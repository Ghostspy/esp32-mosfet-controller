# ESP32 MOSFET Chamber Controller for 3D Printing

Remote WiFi-controlled chamber heater, fan, and LED controller using ESP32 with 4-channel MOSFET module and DS18B20 temperature sensor.

## Hardware Requirements

### Main Components
- **ESP32 Development Board** (ESP32-WROOM-32 or similar)
- **4-Channel MOSFET Module** (DC 5-60V)
  - Example: Yaregelun 4-Channel MOS Switch Module
  - Must support PWM control
  - Recommended: IRF520 or similar N-channel MOSFETs
- **DS18B20 Temperature Sensor** (waterproof recommended)
- **24V Power Supply** (adequate amperage for your loads)
- **4.7kΩ Resistor** (for DS18B20 pull-up)

### Controlled Devices (24V DC)
- Chamber heater (silicone heater pad or similar)
- Exhaust fan (24V DC fan)
- LED strip (24V DC LEDs)
- Spare channel available

### Tools & Accessories
- Wire (appropriate gauge for current draw)
- Soldering iron and solder
- Heat shrink tubing
- Multimeter (for testing)
- Breadboard (for prototyping, optional)

## Wiring Diagram

### Power Distribution
```
24V Power Supply
├── (+) → MOSFET Module VIN
├── (+) → Heater (+), Fan (+), LED (+)
└── (-) → Common Ground

ESP32 3.3V
├── (+) → DS18B20 VCC (Red wire)
└── (+) → 4.7kΩ resistor → DS18B20 Data (Yellow wire)
```

### Pin Connections

#### ESP32 to MOSFET Module
```
ESP32          MOSFET Module
GPIO 25    →   Channel 1 (Heater)
GPIO 26    →   Channel 2 (Fan)
GPIO 27    →   Channel 3 (LEDs)
GPIO 14    →   Channel 4 (Spare)
GND        →   GND
```

#### DS18B20 Temperature Sensor
```
DS18B20        ESP32
Red (VCC)  →   3.3V
Black (GND)→   GND
Yellow (Data)→ GPIO 4 (with 4.7kΩ pullup to 3.3V)
```

#### Load Connections
```
MOSFET Ch1 OUT → Heater (-)    | Heater (+) → 24V+
MOSFET Ch2 OUT → Fan (-)       | Fan (+) → 24V+
MOSFET Ch3 OUT → LED (-)       | LED (+) → 24V+
```

**IMPORTANT:** All grounds must be connected together (ESP32 GND, MOSFET GND, 24V PSU GND)

## Software Installation

### Required Arduino Libraries

Install via Arduino IDE Library Manager:
1. **OneWire** by Paul Stoffregen
2. **DallasTemperature** by Miles Burton
3. **ArduinoJson** by Benoit Blanchon (v6.x)
4. **PubSubClient** by Nick O'Leary (for MQTT)

### Installation Steps

1. **Install Arduino IDE**
   - Download from https://www.arduino.cc/en/software
   - Version 2.0+ recommended

2. **Install ESP32 Board Support**
```
   File → Preferences → Additional Board Manager URLs:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   
   Tools → Board → Boards Manager → Search "ESP32" → Install
```

3. **Install Libraries**
```
   Sketch → Include Library → Manage Libraries
   Search and install each required library listed above
```

4. **Configure Code**
   Edit these lines in the sketch:
```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* mqtt_server = "192.168.1.100";  // Optional
```

5. **Upload Code**
```
   Tools → Board → ESP32 Dev Module
   Tools → Port → Select your COM port
   Sketch → Upload
```

6. **Find ESP32 IP Address**
   - Open Serial Monitor (115200 baud)
   - Look for "IP Address: 192.168.1.XXX"
   - Save this IP for later use

## Configuration

### WiFi Setup
```cpp
const char* ssid = "YourNetworkName";
const char* password = "YourPassword";
```

### MQTT Setup (Optional)
```cpp
const char* mqtt_server = "192.168.1.100";  // Your MQTT broker IP
const int mqtt_port = 1883;
const char* mqtt_user = "username";         // Optional
const char* mqtt_pass = "password";         // Optional
```

### PID Tuning
Default values work for most heaters, but you can tune:
```cpp
float Kp = 50.0;   // Proportional gain
float Ki = 0.5;    // Integral gain
float Kd = 25.0;   // Derivative gain
```

**Tuning Guide:**
- **Kp too high:** Oscillation, overshoot
- **Kp too low:** Slow response, never reaches target
- **Ki too high:** Overshoot, instability
- **Kd too high:** Noisy, erratic behavior

Adjust via web interface or G-code: `M301 P50 I0.5 D25`

### Safety Limits
```cpp
#define MAX_CHAMBER_TEMP 80.0     // Maximum safe temperature (°C)
#define HEATER_TIMEOUT 300000     // 5 minutes timeout (ms)
```

## Usage

### Web Interface

Access at: `http://192.168.1.XXX/`

Features:
- **Real-time temperature display** (updates every 2 seconds)
- **Chamber temperature control** (slider and manual input)
- **Fan speed control** (0-100% with PWM)
- **LED brightness control** (0-100% with PWM)
- **Emergency stop button**
- **Mobile-responsive design**

### HTTP API

#### Get Status
```bash
curl http://192.168.1.XXX/api/status
```
Response:
```json
{
  "temperature": 45.2,
  "target": 50.0,
  "heating": true,
  "fan": 128,
  "led": 255,
  "emergency": false
}
```

#### Set Chamber Temperature
```bash
curl -X POST http://192.168.1.XXX/api/chamber \
  -H "Content-Type: application/json" \
  -d '{"temperature": 50}'
```

#### Set Fan Speed (0-255)
```bash
curl -X POST http://192.168.1.XXX/api/fan \
  -H "Content-Type: application/json" \
  -d '{"speed": 128}'
```

#### Set LED Brightness (0-255)
```bash
curl -X POST http://192.168.1.XXX/api/led \
  -H "Content-Type: application/json" \
  -d '{"brightness": 191}'
```

#### Emergency Stop
```bash
curl -X POST http://192.168.1.XXX/api/emergency
```

### G-code Commands

Send via HTTP POST to `/gcode` endpoint:
```bash
curl -X POST http://192.168.1.XXX/gcode -d "M141 S50"
```

#### Supported G-codes
```gcode
M141 S[temp]    # Set chamber temperature (°C)
M141 S0         # Turn off chamber heater
M106 S[speed]   # Set fan speed (0-255)
M107            # Turn off fan
M150 S[bright]  # Set LED brightness (0-255)
M105            # Get temperature report
M112            # Emergency stop
M301 P[p] I[i] D[d]  # Set PID values
```

### MQTT Topics

#### Subscribe (Control)
```
printer/chamber/set    # Set chamber temp (payload: temperature value)
printer/fan/set        # Set fan speed (payload: 0-255)
printer/led/set        # Set LED brightness (payload: 0-255)
```

#### Publish (Status)
```
printer/chamber/temperature  # Current temperature
printer/chamber/target       # Target temperature
printer/chamber/heating      # Heating state (true/false)
printer/fan/speed           # Fan speed (0-255)
printer/led/brightness      # LED brightness (0-255)
printer/chamber/emergency   # Emergency stop event
```

#### MQTT Examples
```bash
# Set chamber temperature
mosquitto_pub -h 192.168.1.100 -t "printer/chamber/set" -m "50"

# Subscribe to all status updates
mosquitto_sub -h 192.168.1.100 -t "printer/#"
```

## OrcaSlicer Integration

### Method 1: Post-Processing Script

**chamber_control.py:**
```python
#!/usr/bin/env python3
import sys
import requests

ESP32_IP = "192.168.1.XXX"

def send_command(gcode):
    try:
        requests.post(f"http://{ESP32_IP}/gcode", data=gcode, timeout=2)
    except:
        pass

def process_gcode(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Detect filament type
    filament_type = "PLA"
    chamber_temp = 0
    
    for line in lines[:100]:
        if "ABS" in line.upper() or "ASA" in line.upper():
            chamber_temp = 50
        elif "PETG" in line.upper():
            chamber_temp = 35
    
    # Send preheat commands
    if chamber_temp > 0:
        send_command(f"M141 S{chamber_temp}")
    send_command("M150 S255")  # LEDs on
    send_command("M106 S77")   # Fan 30%
    
    print(f"Chamber preheated to {chamber_temp}°C")

if __name__ == "__main__":
    process_gcode(sys.argv[1])
```

**OrcaSlicer Setup:**
1. Save script to `/path/to/chamber_control.py`
2. Make executable: `chmod +x chamber_control.py`
3. OrcaSlicer → Edit → Preferences → Post-processing scripts
4. Add: `/path/to/chamber_control.py`

### Method 2: Manual Control

Create desktop shortcuts or scripts:

**Linux/Mac:**
```bash
#!/bin/bash
# preheat_abs.sh
curl -X POST http://192.168.1.XXX/gcode -d "M141 S50"
curl -X POST http://192.168.1.XXX/gcode -d "M150 S255"
curl -X POST http://192.168.1.XXX/gcode -d "M106 S128"
echo "Chamber preheating for ABS..."
```

**Windows:**
```batch
@echo off
REM preheat_abs.bat
curl -X POST http://192.168.1.XXX/gcode -d "M141 S50"
curl -X POST http://192.168.1.XXX/gcode -d "M150 S255"
curl -X POST http://192.168.1.XXX/gcode -d "M106 S128"
echo Chamber preheating for ABS...
pause
```

### Recommended Chamber Temperatures

| Filament | Chamber Temp | Notes |
|----------|-------------|-------|
| PLA | 0°C (off) | No chamber heating |
| PETG | 30-35°C | Improves layer adhesion |
| ABS/ASA | 50-60°C | Critical for warp prevention |
| Nylon/PA | 55-70°C | Reduces moisture issues |
| PC | 60-70°C | High temp needed |
| TPU | 0-30°C | Depends on hardness |

## Home Assistant Integration

**configuration.yaml:**
```yaml
mqtt:
  sensor:
    - name: "Chamber Temperature"
      state_topic: "printer/chamber/temperature"
      unit_of_measurement: "°C"
      device_class: temperature
    
    - name: "Chamber Target Temperature"
      state_topic: "printer/chamber/target"
      unit_of_measurement: "°C"
  
  number:
    - name: "Chamber Temperature Setpoint"
      command_topic: "printer/chamber/set"
      state_topic: "printer/chamber/target"
      min: 0
      max: 80
      step: 1
      unit_of_measurement: "°C"
      mode: slider
  
  fan:
    - name: "Chamber Fan"
      command_topic: "printer/fan/set"
      state_topic: "printer/fan/speed"
      payload_off: "0"
      payload_on: "255"
      speed_range_min: 0
      speed_range_max: 255
  
  light:
    - name: "Chamber Lights"
      command_topic: "printer/led/set"
      state_topic: "printer/led/brightness"
      brightness_scale: 255
```

## Troubleshooting

### Temperature Sensor Issues

**Problem:** "DS18B20 sensor not found"
- **Check wiring:** VCC to 3.3V, GND to GND, Data to GPIO4
- **Verify pull-up resistor:** 4.7kΩ between Data and VCC
- **Test sensor:** Try different GPIO pin
- **Multiple sensors:** Each needs unique address

**Problem:** Temperature reading -127°C or DEVICE_DISCONNECTED
- **Bad connection:** Resolder connections
- **Wrong sensor:** Ensure DS18B20, not DHT22 or similar
- **Long wires:** Use shielded cable for runs >3 meters

### WiFi Connection Issues

**Problem:** "WiFi connection failed"
- **Check credentials:** Verify SSID and password
- **Signal strength:** Move ESP32 closer to router
- **2.4GHz only:** ESP32 doesn't support 5GHz WiFi
- **Static IP:** Consider setting static IP in router

**Problem:** IP address keeps changing
- **Set static DHCP:** Reserve IP in router settings
- **Or use mDNS:** Access via `http://chamber.local/` (requires mDNS support)

### Heating Issues

**Problem:** Heater not turning on
- **Check voltage:** Verify 24V at MOSFET output with multimeter
- **MOSFET failure:** Test MOSFET with different load
- **Current limit:** Ensure heater doesn't exceed MOSFET rating
- **Common ground:** All grounds must be connected

**Problem:** Temperature oscillates wildly
- **PID tuning:** Reduce Kp value (try 30 instead of 50)
- **Sensor placement:** Move sensor closer to heater
- **Thermal mass:** Small heaters oscillate more

**Problem:** Heater timeout error
- **Increase timeout:** Change `HEATER_TIMEOUT` to 600000 (10 min)
- **Insufficient power:** Check heater wattage vs power supply
- **Poor insulation:** Seal chamber better

### Web Interface Issues

**Problem:** Cannot access web interface
- **Ping test:** `ping 192.168.1.XXX`
- **Firewall:** Check Windows/Mac firewall settings
- **Different subnet:** Ensure PC and ESP32 on same network
- **Port 80 blocked:** Some routers block port 80

**Problem:** Web page loads but controls don't work
- **JavaScript errors:** Check browser console (F12)
- **JSON parsing:** Update ArduinoJson library to latest
- **API timeout:** Increase timeout in code

### MOSFET Module Issues

**Problem:** MOSFET stays on all the time
- **Stuck gate:** Replace MOSFET
- **Wrong wiring:** Verify ESP32 GPIO to MOSFET signal pins
- **Logic level:** Some MOSFETs need 12V gate drive (use logic-level MOSFETs)

**Problem:** MOSFET gets very hot
- **Undersized MOSFET:** Use higher current rating
- **No heatsink:** Add heatsink to MOSFET
- **PWM frequency:** Try lower frequency (reduce `PWM_FREQ`)

## Safety Warnings

⚠️ **ELECTRICAL SAFETY**
- Never work on live circuits
- Use proper wire gauge for current ratings
- Install fuses/circuit breakers
- Keep electronics away from heat sources

⚠️ **FIRE SAFETY**
- Do not exceed MAX_CHAMBER_TEMP (80°C default)
- Always use thermal runaway protection
- Never leave heater unattended
- Install smoke detector near printer
- Keep fire extinguisher accessible

⚠️ **THERMAL SAFETY**
- Heaters and enclosure surfaces get HOT
- Allow cooldown before handling
- Use high-temp rated wire (>105°C)
- Ensure proper ventilation

⚠️ **TESTING**
- Test without printer first
- Monitor first few heating cycles
- Verify emergency stop works
- Check temperature sensor accuracy

## Specifications

### Electrical Ratings
- **Input Voltage:** 5V (ESP32), 24V (loads)
- **MOSFET Module:** 5-60V DC, 5A per channel (typical)
- **PWM Frequency:** 25kHz (adjustable)
- **PWM Resolution:** 8-bit (0-255)

### Temperature Control
- **Sensor:** DS18B20 (-55°C to +125°C)
- **Resolution:** 0.0625°C (12-bit)
- **Update Rate:** 1 second
- **Control Method:** PID
- **Safety Limit:** 80°C (configurable)

### Network
- **WiFi:** 2.4GHz 802.11 b/g/n
- **Web Server:** HTTP on port 80
- **MQTT:** Port 1883 (configurable)
- **Update Rate:** 2 seconds (status)

## Current Consumption

Approximate current draw @ 24V:
- Silicone heater (100W): ~4.2A
- 24V fan (5W): ~0.2A
- LED strip (12W): ~0.5A
- **Total:** ~5A (requires 24V 6A+ power supply)

**ESP32 draws ~200mA @ 5V** (can be powered from USB during development)

## License

This project is released under MIT License. Use at your own risk.

## Credits

- ESP32 Arduino Core by Espressif
- DallasTemperature library by Miles Burton
- OneWire library by Paul Stoffregen
- ArduinoJson library by Benoit Blanchon

## Support

For issues or questions:
1. Check troubleshooting section above
2. Verify all wiring connections
3. Test components individually
4. Check Serial Monitor output (115200 baud)

## Version History

- **v1.0** - Initial release with PID control, web interface, MQTT support