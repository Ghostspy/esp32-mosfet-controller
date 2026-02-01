/*
 * ESP32-S3 MOSFET Chamber Controller for 3D Printing
 * 
 * Hardware:
 * - ESP32-S3 N16R8 DevKitC-1 Development Board
 * - 4-Channel MOSFET Module (DC 5-36V)
 * - DS18B20 Temperature Sensor
 * - 24V Chamber Heater
 * - 24V Internal Purifier Fan (HEPA/Carbon filter)
 * - 24V LED Lights
 * 
 * Features:
 * - WiFi web interface with real-time updates
 * - PID temperature control
 * - Automatic purifier fan management
 * - MQTT support for home automation
 * - G-code command interface
 * - Filter runtime tracking
 * - Emergency stop with safety features
 * 
 * Version: 2.1 - ESP32-S3 Compatible
 * Author: Chamber Controller Project
 */

#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============== CONFIGURATION ==============

// WiFi Configuration
const char* ssid = "gh-iot";
const char* password = "littleorangemen";

// MQTT Configuration (optional - comment out if not using)
const char* mqtt_server = "192.168.1.179";
const int mqtt_port = 1883;
const char* mqtt_user = "ghost";
const char* mqtt_pass = "L0g!tech";
const char* mqtt_client_id = "esp32_chamber";

// Pin Definitions - MOSFET Module (ESP32-S3 safe GPIOs)
#define CHAMBER_HEATER_PIN 5  // Channel 1 - 24V Chamber Heater
#define PURIFIER_FAN_PIN 6    // Channel 2 - 24V Internal Purifier Fan
#define LED_PIN 7             // Channel 3 - 24V LED Strip
#define SPARE_PIN 8           // Channel 4 - Available for future use

// DS18B20 Temperature Sensor
#define ONE_WIRE_BUS 4        // Data pin (needs 4.7kΩ pullup to 3.3V)

// PWM Configuration
#define PWM_FREQ 1000       // 20kHz for silent operation (within module's 0-20kHz range)
#define PWM_RESOLUTION 8      // 8-bit (0-255)

// Safety Limits
#define MAX_CHAMBER_TEMP 60.0  // Maximum safe temperature (°C)
#define MIN_TEMP_READING -20.0 // Sensor fault detection
#define HEATER_TIMEOUT 600000  // 5 minutes to reach temperature

// PID Constants (tune for your heater)
float Kp = 50.0;  // Proportional gain
float Ki = 0.5;   // Integral gain
float Kd = 25.0;  // Derivative gain

// ============== HARDWARE SETUP ==============
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress chamberThermometer;

WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ============== STATE VARIABLES ==============
// Temperature Control
float chamberTargetTemp = 0;
float chamberCurrentTemp = 0;
bool chamberHeaterEnabled = false;

// PID Variables
float integral = 0;
float lastError = 0;
unsigned long lastPIDTime = 0;

// Purifier Fan Control
int purifierSpeed = 0;         // 0-255
bool autoPurifierControl = true;
bool purifierEnabled = false;

// LED Control
int ledBrightness = 0;         // 0-255

// Safety
unsigned long heatingStartTime = 0;
bool heatingTimeout = false;
bool emergencyStopActive = false;

// Filter Runtime Tracking (in seconds)
unsigned long filterRuntime = 0;
unsigned long lastFilterUpdate = 0;
#define FILTER_REPLACEMENT_HOURS 500  // Alert after 500 hours

// ============== SETUP ==============

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32-S3 MOSFET Chamber Controller ===");
  Serial.println("Version 2.1 - Bentobox Fan Edition (ESP32-S3)");
  
  setupPWM();
  setupTemperatureSensor();
  setupWiFi();
  setupWebServer();
  setupMQTT();
  
  Serial.println("\n=== System Ready ===");
  Serial.printf("Web Interface: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("API Endpoint: http://%s/api\n", WiFi.localIP().toString().c_str());
  Serial.println("Purifier: Auto mode enabled");
}

void setupPWM() {
  // ESP32-S3 Arduino Core 3.x API - attach pins directly
  // ledcAttach(pin, frequency, resolution) - returns true on success
  
  if (!ledcAttach(CHAMBER_HEATER_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ERROR: Failed to attach PWM to CHAMBER_HEATER_PIN");
  }
  if (!ledcAttach(PURIFIER_FAN_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ERROR: Failed to attach PWM to PURIFIER_FAN_PIN");
  }
  if (!ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ERROR: Failed to attach PWM to LED_PIN");
  }
  if (!ledcAttach(SPARE_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ERROR: Failed to attach PWM to SPARE_PIN");
  }
  
  // Initialize all outputs to OFF (use pin numbers, not channel numbers!)
  ledcWrite(CHAMBER_HEATER_PIN, 0);
  ledcWrite(PURIFIER_FAN_PIN, 0);
  ledcWrite(LED_PIN, 0);
  ledcWrite(SPARE_PIN, 0);
  
  Serial.println("PWM channels initialized on GPIO 5, 6, 7, 8");
}

void setupTemperatureSensor() {
  sensors.begin();
  if (!sensors.getAddress(chamberThermometer, 0)) {
    Serial.println("WARNING: DS18B20 sensor not found!");
    Serial.println("Check wiring: VCC->3.3V, GND->GND, Data->GPIO4 (with 4.7kΩ pullup)");
  } else {
    sensors.setResolution(chamberThermometer, 12);
    Serial.println("DS18B20 temperature sensor initialized");
  }
}

void setupWiFi() {
  Serial.printf("Connecting to WiFi: %s ", ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println(" FAILED!");
    Serial.println("Check SSID and password, then restart");
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/status", HTTP_GET, handleAPIStatus);
  server.on("/api/chamber", HTTP_POST, handleAPIChamber);
  server.on("/api/purifier", HTTP_POST, handleAPIPurifier);
  server.on("/api/led", HTTP_POST, handleAPILed);
  server.on("/api/emergency", HTTP_POST, handleAPIEmergency);
  server.on("/api/filter/reset", HTTP_POST, handleAPIFilterReset);
  server.on("/gcode", HTTP_POST, handleGcode);
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void setupMQTT() {
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
  reconnectMQTT();
}

// ============== MAIN LOOP ==============

void loop() {
  server.handleClient();
  
  if (mqtt.connected()) {
    mqtt.loop();
  } else {
    reconnectMQTT();
  }
  
  updateChamberHeater();
  updatePurifierFan();
  updateFilterRuntime();
  publishMQTTStatus();
  
  delay(100);
}

// ============== TEMPERATURE CONTROL ==============

float readChamberTemp() {
  sensors.requestTemperatures();
  float temp = sensors.getTempC(chamberThermometer);
  
  if (temp == DEVICE_DISCONNECTED_C || temp < MIN_TEMP_READING) {
    Serial.println("ERROR: Temperature sensor fault!");
    emergencyStop();
    return 0;
  }
  
  return temp;
}

void setChamberTemp(float temp) {
  chamberTargetTemp = constrain(temp, 0, MAX_CHAMBER_TEMP);
  chamberHeaterEnabled = (chamberTargetTemp > 0);
  emergencyStopActive = false;
  
  if (chamberHeaterEnabled) {
    heatingStartTime = millis();
    heatingTimeout = false;
    integral = 0;
    lastError = 0;
    lastPIDTime = millis();
  } else {
    ledcWrite(CHAMBER_HEATER_PIN, 0);  // Turn off heater
  }
  
  Serial.printf("Chamber target set to %.1f°C\n", chamberTargetTemp);
}

void updateChamberHeater() {
  // Read temperature every second
  static unsigned long lastTempRead = 0;
  if (millis() - lastTempRead > 1000) {
    chamberCurrentTemp = readChamberTemp();
    lastTempRead = millis();
  }
  
  // Safety: Check for overtemperature
  if (chamberCurrentTemp > MAX_CHAMBER_TEMP) {
    Serial.println("ERROR: Chamber overtemperature!");
    emergencyStop();
    return;
  }
  
  // Safety: Check for heating timeout
  if (chamberHeaterEnabled && !heatingTimeout) {
    if (millis() - heatingStartTime > HEATER_TIMEOUT) {
      if (chamberCurrentTemp < chamberTargetTemp - 5) {
        Serial.println("ERROR: Heating timeout - target temperature not reached");
        heatingTimeout = true;
        emergencyStop();
        return;
      }
    }
  }
  
  // If heater disabled or in emergency state, turn off
  if (!chamberHeaterEnabled || heatingTimeout || emergencyStopActive) {
    ledcWrite(CHAMBER_HEATER_PIN, 0);
    return;
  }
  
  // PID Control
  unsigned long now = millis();
  float dt = (now - lastPIDTime) / 1000.0;
  if (dt < 0.1) return; // Don't update too frequently
  lastPIDTime = now;
  
  float error = chamberTargetTemp - chamberCurrentTemp;
  
  // Proportional
  float P = Kp * error;
  
  // Integral with anti-windup
  integral += error * dt;
  integral = constrain(integral, -100, 100);
  float I = Ki * integral;
  
  // Derivative
  float derivative = (error - lastError) / dt;
  lastError = error;
  float D = Kd * derivative;
  
  // Calculate output
  float output = P + I + D;
  output = constrain(output, 0, 255);
  
  // Apply to heater
  ledcWrite(CHAMBER_HEATER_PIN, (int)output);
}

// ============== PURIFIER FAN CONTROL ==============

void updatePurifierFan() {
  if (autoPurifierControl) {
    if (chamberHeaterEnabled) {
      // Chamber is actively heating or maintaining temperature
      if (chamberCurrentTemp < chamberTargetTemp - 10) {
        // Initial heating phase - medium speed for heat distribution
        setPurifier(128);  // 50% - distribute heat without excessive cooling
      } 
      else if (chamberCurrentTemp < chamberTargetTemp - 2) {
        // Approaching target - increase for better filtration
        setPurifier(192);  // 75%
      }
      else {
        // At target or maintaining - full filtration during printing
        setPurifier(255);  // 100%
      }
      purifierEnabled = true;
    }
    else if (!chamberHeaterEnabled && chamberCurrentTemp > 40) {
      // Cooling down but still warm - continue filtering
      setPurifier(192);  // 75%
      purifierEnabled = true;
    }
    else {
      // Cool enough - can turn off purifier
      setPurifier(0);
      purifierEnabled = false;
    }
  }
  // If manual mode, purifierSpeed is already set by setPurifier() calls from API
}

void setPurifier(int speed) {
  if (!emergencyStopActive) {
    purifierSpeed = constrain(speed, 0, 255);
    ledcWrite(PURIFIER_FAN_PIN, purifierSpeed);
  }
}

void updateFilterRuntime() {
  if (purifierSpeed > 0) {
    unsigned long now = millis();
    if (lastFilterUpdate > 0) {
      filterRuntime += (now - lastFilterUpdate) / 1000;  // Add seconds
    }
    lastFilterUpdate = now;
    
    // Check if filter replacement needed
    static unsigned long lastWarning = 0;
    if (filterRuntime > FILTER_REPLACEMENT_HOURS * 3600) {
      if (millis() - lastWarning > 3600000) {  // Warn once per hour
        Serial.println("WARNING: Filter replacement recommended!");
        Serial.printf("Runtime: %.1f hours\n", filterRuntime / 3600.0);
        lastWarning = millis();
      }
    }
  } else {
    lastFilterUpdate = 0;
  }
}

// ============== LED CONTROL ==============

void setLED(int brightness) {
  ledBrightness = constrain(brightness, 0, 255);
  ledcWrite(LED_PIN, ledBrightness);
  Serial.printf("LED brightness: %d (%.1f%%)\n", ledBrightness, (ledBrightness/255.0)*100);
}

// ============== EMERGENCY STOP ==============

void emergencyStop() {
  emergencyStopActive = true;
  chamberHeaterEnabled = false;
  chamberTargetTemp = 0;
  
  ledcWrite(CHAMBER_HEATER_PIN, 0);  // Heater OFF
  setPurifier(255);                   // Purifier FULL for cooling
  
  integral = 0;
  lastError = 0;
  
  if (mqtt.connected()) {
    mqtt.publish("printer/chamber/emergency", "EMERGENCY STOP ACTIVATED");
  }
  
  Serial.println("!!! EMERGENCY STOP - Heater OFF, Purifier FULL !!!");
}

// ============== MQTT ==============

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();
  
  if (!mqtt.connected()) {
    Serial.print("Connecting to MQTT... ");
    if (mqtt.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      mqtt.subscribe("printer/chamber/set");
      mqtt.subscribe("printer/purifier/set");
      mqtt.subscribe("printer/purifier/auto");
      mqtt.subscribe("printer/led/set");
      publishMQTTStatus();
    } else {
      Serial.printf("failed, rc=%d\n", mqtt.state());
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.printf("MQTT [%s]: %s\n", topic, message.c_str());
  
  if (strcmp(topic, "printer/chamber/set") == 0) {
    float temp = message.toFloat();
    setChamberTemp(temp);
  }
  else if (strcmp(topic, "printer/purifier/set") == 0) {
    int speed = message.toInt();
    autoPurifierControl = false;  // Switch to manual mode
    setPurifier(speed);
  }
  else if (strcmp(topic, "printer/purifier/auto") == 0) {
    autoPurifierControl = (message == "1" || message.equalsIgnoreCase("true"));
    Serial.printf("Purifier auto mode: %s\n", autoPurifierControl ? "ON" : "OFF");
  }
  else if (strcmp(topic, "printer/led/set") == 0) {
    int brightness = message.toInt();
    setLED(brightness);
  }
}

void publishMQTTStatus() {
  static unsigned long lastPublish = 0;
  if (millis() - lastPublish < 2000) return;
  lastPublish = millis();
  
  if (mqtt.connected()) {
    char temp[10];
    dtostrf(chamberCurrentTemp, 4, 2, temp);
    mqtt.publish("printer/chamber/temperature", temp);
    
    char target[10];
    dtostrf(chamberTargetTemp, 4, 2, target);
    mqtt.publish("printer/chamber/target", target);
    
    mqtt.publish("printer/chamber/heating", chamberHeaterEnabled ? "true" : "false");
    mqtt.publish("printer/purifier/speed", String(purifierSpeed).c_str());
    mqtt.publish("printer/purifier/enabled", purifierEnabled ? "true" : "false");
    mqtt.publish("printer/purifier/auto", autoPurifierControl ? "true" : "false");
    mqtt.publish("printer/led/brightness", String(ledBrightness).c_str());
    
    char runtime[10];
    dtostrf(filterRuntime / 3600.0, 4, 1, runtime);
    mqtt.publish("printer/filter/runtime_hours", runtime);
  }
}

// ============== WEB INTERFACE ==============

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Enclosure Controller</title>
  <style>
    * { box-sizing: border-box; }
    body { 
      font-family: 'Segoe UI', Arial, sans-serif; 
      margin: 0; 
      padding: 20px; 
      background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
      color: #fff; 
    }
    .container { max-width: 700px; margin: 0 auto; }
    .header {
      text-align: center;
      margin-bottom: 30px;
      padding: 20px;
      background: rgba(76, 175, 80, 0.1);
      border-radius: 10px;
      border: 2px solid #4CAF50;
    }
    h1 { 
      color: #4CAF50; 
      margin: 0;
      font-size: 28px;
    }
    .subtitle {
      color: #888;
      margin-top: 5px;
      font-size: 14px;
    }
    .card { 
      background: #2a2a2a; 
      padding: 25px; 
      margin: 15px 0; 
      border-radius: 12px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .card h2 { 
      color: #4CAF50; 
      margin-top: 0;
      font-size: 20px;
      border-bottom: 2px solid #333;
      padding-bottom: 10px;
    }
    .temp { 
      font-size: 56px; 
      font-weight: bold; 
      color: #4CAF50;
      text-align: center;
      margin: 20px 0;
      text-shadow: 0 0 20px rgba(76, 175, 80, 0.5);
    }
    .slider { 
      width: 100%; 
      height: 8px;
      border-radius: 5px;
      background: #555;
      outline: none;
      -webkit-appearance: none;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
      box-shadow: 0 0 10px rgba(76, 175, 80, 0.5);
    }
    .slider::-moz-range-thumb {
      width: 24px;
      height: 24px;
      border-radius: 50%;
      background: #4CAF50;
      cursor: pointer;
      border: none;
    }
    button { 
      background: #4CAF50; 
      color: white; 
      border: none; 
      padding: 12px 24px; 
      border-radius: 6px; 
      cursor: pointer; 
      font-size: 15px;
      margin: 5px;
      transition: all 0.3s;
      font-weight: 500;
    }
    button:hover { 
      background: #45a049;
      transform: translateY(-2px);
      box-shadow: 0 4px 8px rgba(76, 175, 80, 0.4);
    }
    .emergency { 
      background: #f44336;
      font-size: 16px;
      padding: 15px 30px;
    }
    .emergency:hover { 
      background: #da190b;
      box-shadow: 0 4px 8px rgba(244, 67, 54, 0.4);
    }
    input[type="number"] { 
      width: 100px; 
      padding: 10px; 
      font-size: 18px; 
      background: #3a3a3a;
      color: #fff;
      border: 2px solid #555;
      border-radius: 6px;
      text-align: center;
    }
    .status { 
      font-size: 14px; 
      color: #888; 
      margin-top: 10px;
      padding: 10px;
      background: #1a1a1a;
      border-radius: 6px;
    }
    .indicator {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      margin-right: 5px;
      box-shadow: 0 0 5px currentColor;
    }
    .on { background-color: #4CAF50; }
    .off { background-color: #555; }
    .toggle {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
      vertical-align: middle;
    }
    .toggle input { opacity: 0; width: 0; height: 0; }
    .toggle-slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #555;
      transition: .4s;
      border-radius: 34px;
    }
    .toggle-slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .4s;
      border-radius: 50%;
    }
    input:checked + .toggle-slider { background-color: #4CAF50; }
    input:checked + .toggle-slider:before { transform: translateX(26px); }
    .input-group {
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
      margin: 15px 0;
    }
    .warning {
      background: rgba(255, 152, 0, 0.2);
      border: 2px solid #FF9800;
      padding: 10px;
      border-radius: 6px;
      margin-top: 10px;
      color: #FFB74D;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Enclosure Controller</h1>
      <div class="subtitle">ESP32-S3 MOSFET • Bentobox Fan Edition</div>
    </div>
    
    <div class="card">
      <h2>🌡️ Chamber Temperature</h2>
      <div class="temp" id="currentTemp">--°C</div>
      <div class="status">
        Target: <span id="targetTemp">--</span>°C | 
        <span class="indicator" id="heaterInd"></span>
        Heater: <span id="heaterStatus">--</span>
      </div>
      <div class="input-group">
        <input type="number" id="tempInput" value="50" min="0" max="60" step="1">
        <button onclick="setChamber()">Set Temperature</button>
        <button onclick="setChamber(0)">Turn Off</button>
      </div>
    </div>
    
    <div class="card">
      <h2>💨 Air Purifier Fan</h2>
      <input type="range" min="0" max="255" value="128" class="slider" id="purifierSlider" oninput="updatePurifier(this.value)">
      <div class="status">
        Speed: <span id="purifierSpeed">50</span>% | 
        <span class="indicator" id="purifierInd"></span>
        Status: <span id="purifierStatus">AUTO</span>
      </div>
      <div style="margin-top: 15px;">
        <label class="toggle">
          <input type="checkbox" id="autoToggle" checked onchange="toggleAuto()">
          <span class="toggle-slider"></span>
        </label>
        <span style="margin-left: 10px; vertical-align: middle;">
          Auto Mode (runs with heater for filtration)
        </span>
      </div>
      <div class="status" style="margin-top: 10px;">
        Filter Runtime: <span id="filterHours">0</span> hours
        <span id="filterWarning" class="warning" style="display: none;">
          ⚠️ Filter replacement recommended!
        </span>
      </div>
    </div>
    
    <div class="card">
      <h2>💡 LED Lights</h2>
      <input type="range" min="0" max="255" value="0" class="slider" id="ledSlider" oninput="updateLED(this.value)">
      <div class="status">Brightness: <span id="ledBright">0</span>%</div>
    </div>
    
    <div class="card" style="text-align: center;">
      <button class="emergency" onclick="emergencyStop()">🛑 EMERGENCY STOP</button>
      <div class="status" style="margin-top: 10px;">
        Stops heater immediately, runs purifier at full speed
      </div>
    </div>
  </div>

  <script>
    function updateStatus() {
      fetch('/api/status')
        .then(r => r.json())
        .then(data => {
          // Temperature
          document.getElementById('currentTemp').innerText = data.temperature.toFixed(1) + '°C';
          document.getElementById('targetTemp').innerText = data.target.toFixed(1);
          
          // Heater status
          const heating = data.heating;
          document.getElementById('heaterStatus').innerText = heating ? 'ON' : 'OFF';
          document.getElementById('heaterInd').className = 'indicator ' + (heating ? 'on' : 'off');
          
          // Purifier status
          const purifierPct = Math.round(data.purifier * 100 / 255);
          document.getElementById('purifierSpeed').innerText = purifierPct;
          document.getElementById('purifierInd').className = 'indicator ' + (data.purifier > 0 ? 'on' : 'off');
          
          if (!data.purifier_auto) {
            document.getElementById('purifierSlider').value = data.purifier;
          }
          
          const autoMode = data.purifier_auto;
          document.getElementById('autoToggle').checked = autoMode;
          document.getElementById('purifierStatus').innerText = autoMode ? 'AUTO' : 'MANUAL';
          
          // LED status
          document.getElementById('ledSlider').value = data.led;
          document.getElementById('ledBright').innerText = Math.round(data.led * 100 / 255);
          
          // Filter runtime
          const hours = data.filter_hours || 0;
          document.getElementById('filterHours').innerText = hours.toFixed(1);
          
          // Show warning if filter needs replacement
          const warning = document.getElementById('filterWarning');
          if (hours > 500) {
            warning.style.display = 'block';
          } else {
            warning.style.display = 'none';
          }
        })
        .catch(err => console.error('Status update failed:', err));
    }
    
    function setChamber(temp) {
      const t = temp !== undefined ? temp : document.getElementById('tempInput').value;
      fetch('/api/chamber', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({temperature: parseFloat(t)})
      }).then(() => updateStatus());
    }
    
    function updatePurifier(value) {
      document.getElementById('purifierSpeed').innerText = Math.round(value * 100 / 255);
      
      // Switch to manual mode when slider moved
      document.getElementById('autoToggle').checked = false;
      
      fetch('/api/purifier', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({speed: parseInt(value), auto: false})
      });
    }
    
    function toggleAuto() {
      const auto = document.getElementById('autoToggle').checked;
      fetch('/api/purifier', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({auto: auto})
      }).then(() => updateStatus());
    }
    
    function updateLED(value) {
      document.getElementById('ledBright').innerText = Math.round(value * 100 / 255);
      fetch('/api/led', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({brightness: parseInt(value)})
      });
    }
    
    function emergencyStop() {
      if (confirm('Execute emergency stop?\n\nThis will:\n- Turn OFF heater immediately\n- Run purifier at FULL speed\n- Require manual reset')) {
        fetch('/api/emergency', {method: 'POST'}).then(() => updateStatus());
      }
    }
    
    // Update every 2 seconds
    setInterval(updateStatus, 2000);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

// ============== API HANDLERS ==============

void handleAPIStatus() {
  StaticJsonDocument<384> doc;
  doc["temperature"] = chamberCurrentTemp;
  doc["target"] = chamberTargetTemp;
  doc["heating"] = chamberHeaterEnabled;
  doc["purifier"] = purifierSpeed;
  doc["purifier_auto"] = autoPurifierControl;
  doc["led"] = ledBrightness;
  doc["emergency"] = emergencyStopActive;
  doc["filter_hours"] = filterRuntime / 3600.0;
  
  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

void handleAPIChamber() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    float temp = doc["temperature"];
    setChamberTemp(temp);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
  }
}

void handleAPIPurifier() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("auto")) {
      autoPurifierControl = doc["auto"];
      Serial.printf("Purifier auto mode: %s\n", autoPurifierControl ? "ON" : "OFF");
    }
    
    if (doc.containsKey("speed")) {
      int speed = doc["speed"];
      autoPurifierControl = false;  // Manual control
      setPurifier(speed);
      Serial.printf("Purifier manual speed: %d (%.1f%%)\n", speed, (speed/255.0)*100);
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
  }
}

void handleAPILed() {
  if (server.hasArg("plain")) {
    StaticJsonDocument<128> doc;
    deserializeJson(doc, server.arg("plain"));
    int brightness = doc["brightness"];
    setLED(brightness);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"invalid request\"}");
  }
}

void handleAPIEmergency() {
  emergencyStop();
  server.send(200, "application/json", "{\"status\":\"emergency stop activated\"}");
}

void handleAPIFilterReset() {
  filterRuntime = 0;
  Serial.println("Filter runtime counter reset to 0");
  server.send(200, "application/json", "{\"status\":\"filter runtime reset\"}");
}

// ============== G-CODE HANDLER ==============

void handleGcode() {
  if (server.hasArg("plain")) {
    String gcode = server.arg("plain");
    gcode.toUpperCase();
    gcode.trim();
    processGcode(gcode);
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "error: no gcode provided");
  }
}

void processGcode(String gcode) {
  Serial.printf("G-code: %s\n", gcode.c_str());
  
  // M141 - Set Chamber Temperature
  if (gcode.startsWith("M141")) {
    float temp = parseValue(gcode, 'S');
    setChamberTemp(temp);
  }
  // M106 - Set Purifier Speed
  else if (gcode.startsWith("M106")) {
    int speed = parseValue(gcode, 'S');
    if (speed == 0) speed = 255;  // M106 with no S parameter = full speed
    autoPurifierControl = false;
    setPurifier(speed);
  }
  // M107 - Purifier Off
  else if (gcode.startsWith("M107")) {
    autoPurifierControl = false;
    setPurifier(0);
  }
  // M150 - Set LED Brightness
  else if (gcode.startsWith("M150")) {
    int brightness = parseValue(gcode, 'S');
    setLED(brightness);
  }
  // M105 - Get Temperature
  else if (gcode.startsWith("M105")) {
    Serial.printf("ok T:%.1f /%.1f\n", chamberCurrentTemp, chamberTargetTemp);
  }
  // M112 - Emergency Stop
  else if (gcode.startsWith("M112")) {
    emergencyStop();
  }
  // M301 - Set PID Values
  else if (gcode.startsWith("M301")) {
    if (gcode.indexOf('P') != -1) Kp = parseValue(gcode, 'P');
    if (gcode.indexOf('I') != -1) Ki = parseValue(gcode, 'I');
    if (gcode.indexOf('D') != -1) Kd = parseValue(gcode, 'D');
    Serial.printf("PID values: P=%.2f I=%.2f D=%.2f\n", Kp, Ki, Kd);
  }
}

float parseValue(String gcode, char param) {
  int index = gcode.indexOf(param);
  if (index == -1) return 0;
  
  String value = "";
  for (int i = index + 1; i < gcode.length(); i++) {
    char c = gcode.charAt(i);
    if (c == ' ' || c == '\t') break;
    if (isDigit(c) || c == '.' || c == '-') value += c;
  }
  return value.toFloat();
}
