# ESP32 Relay Chamber Controller for 3D Printing

Remote WiFi-controlled chamber heater, fan, and LED controller using ESP32 with 4-channel relay module and DS18B20 temperature sensor.

## Hardware Requirements

### Main Components
- **ESP32 Relay Board** (AC 90-250V with built-in relays)
  - Example: ESP32-WROOM-32E 4-Way Relay Module
  - Built-in AC/DC power supply
  - 4x Relay outputs (NO/NC/COM)
- **DS18B20 Temperature Sensor** (waterproof recommended)
- **4.7kΩ Resistor** (for DS18B20 pull-up)
- **24V DC Power Supply** (for loads) OR use AC power through relays

### Controlled Devices
Option A: **24V DC Devices** (switched by relays)
- Chamber heater (24V silicone heater)
- Exhaust fan (24V DC fan)
- LED strip (24V DC LEDs)
- Spare relay available

Option B: **AC-Powered Devices** (110/220V through relays)
- Chamber heater (AC heater)
- Exhaust fan (AC fan)
- LED lights (AC bulbs/strips)

### Tools & Accessories
- Wire (14-18 AWG for AC, appropriate gauge for DC)
- Screwdriver (for terminal blocks)
- Multimeter (for testing)
- Heat shrink tubing
- Cable ties

## Relay Module Overview

### Typical Pin Layout
```
ESP32 Relay Board Layout:
┌─────────────────────────┐
│  AC Input: L, N, GND    │  ← AC 90-250V input
│  ┌───┐ ┌───┐ ┌───┐ ┌───┐│
│  │ 1 │ │ 2 │ │ 3 │ │ 4 ││  ← 4x Relays
│  └───┘ └───┘ └───┘ └───┘│
│  NO NC COM (x4)         │  ← Relay terminals
│  [ESP32-WROOM-32E]      │
│  [USB-C] [Programming]  │
└─────────────────────────┘
```

### Relay Terminal Configuration
Each relay has 3 terminals:
- **NO (Normally Open):** Closed when relay activated
- **NC (Normally Closed):** Open when relay activated  
- **COM (Common):** Connect to power source

**For this application, use NO terminals** (power flows when relay is ON)

## Wiring Diagrams

### Option A: 24V DC Devices (Recommended)
```
AC Outlet → ESP32 Board (powers ESP32)

24V Power Supply
├── (+) → All device (+) terminals
│         ├─ Heater (+)
│         ├─ Fan (+)
│         └─ LED (+)
└── (-) → Common Ground

Relay Connections:
COM 1 → 24V (+)    NO 1 → Heater (-)
COM 2 → 24V (+)    NO 2 → Fan (-)
COM 3 → 24V (+)    NO 3 → LED (-)
COM 4 → (spare)    NO 4 → (spare)
```

### Option B: AC Devices (Use with Caution)
```
⚠️ DANGER: HIGH VOLTAGE - Lethal if mishandled!

AC Outlet (L, N, GND)
├── L (Hot) → 
│   ├─ Relay 1 COM → NO 1 → Heater L
│   ├─ Relay 2 COM → NO 2 → Fan L
│   └─ Relay 3 COM → NO 3 → LED L
└── N (Neutral) → 
    ├─ Heater N
    ├─ Fan N
    └─ LED N

⚠️ ALL AC WIRING MUST BE IN GROUNDED METAL ENCLOSURE
⚠️ USE PROPER WIRE GAUGE (14 AWG minimum for 15A)
⚠️ FOLLOW LOCAL ELECTRICAL CODES
```

### DS18B20 Temperature Sensor
```
DS18B20        ESP32
Red (VCC)  →   3.3V
Black (GND)→   GND
Yellow (Data)→ GPIO 4
               └── 4.7kΩ resistor → 3.3V (pullup)
```

## Relay Pin Configuration

**IMPORTANT:** Verify your board's GPIO assignments!

### Common Configuration A (Most boards)
```cpp
#define RELAY_1 16  // Heater
#define RELAY_2 17  // Fan
#define RELAY_3 18  // LEDs
#define RELAY_4 19  // Spare
```

### Alternative Configuration B
```cpp
#define RELAY_1 13
#define RELAY_2 12
#define RELAY_3 14
#define RELAY_4 27
```

**Check your board's documentation or silkscreen labels!**

## Relay Logic

Most Chinese relay boards are **ACTIVE LOW:**
- `digitalWrite(pin, LOW)` = Relay ON (contacts closed)
- `digitalWrite(pin, HIGH)` = Relay OFF (contacts open)

If your board is **ACTIVE HIGH**, change in code:
```cpp
#define RELAY_ON HIGH
#define RELAY_OFF LOW
```

**Test first:** Upload code, observe relay behavior, adjust if needed.

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
   Search and install each required library
```

4. **Configure Code**
   Edit these lines:
```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   const char* mqtt_server = "192.168.1.100";  // Optional
```

5. **Verify Relay Pin Configuration**
   Check your board and update pin definitions if needed

6. **Upload Code**
```
   Tools → Board → ESP32 Dev Module
   Tools → Port → Select your COM port
   Sketch → Upload
```

7. **Test Relays**
   - Open Serial Monitor (115200 baud)
   - Observe relay clicking sounds
   - Verify correct ON/OFF behavior
   - Adjust RELAY_ON/RELAY_OFF if needed

## Configuration

### WiFi Setup
```cpp
const char* ssid = "YourNetworkName";
const char* password = "YourPassword";
```

### MQTT Setup (Optional)
```cpp
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;
const char* mqtt_user = "username";
const char* mqtt_pass = "password";
```

### Temperature Control Modes

The system supports two control modes:

#### 1. Bang-Bang Control (Default)
Simple ON/OFF control with hysteresis:
- Heater turns ON when temp < target - 2°C
- Heater turns OFF when temp > target + 2°C
- **Pros:** Simple, reliable, works with any heater
- **Cons:** ±2°C temperature swing, more relay cycling
```cpp
String controlMode = "bang-bang";
#define TEMP_HYSTERESIS 2.0  // Adjust swing range
```

#### 2. Time-Proportional Control
PID-based power modulation using relay cycling:
- Calculates power needed (0-100%)
- Cycles relay over 10-second periods
- Example: 50% power = 5 sec ON, 5 sec OFF
- **Pros:** Smoother temperature, less overshoot
- **Cons:** More relay wear, more complex
```cpp
String controlMode = "time-proportional";
#define CYCLE_TIME 10000  // 10 second cycles

// PID tuning
float Kp = 10.0;
float Ki = 0.1;
float Kd = 5.0;
```

Change mode via web interface or API.

### Safety Limits
```cpp
#define MAX_CHAMBER_TEMP 80.0     // Maximum safe temperature (°C)
#define HEATER_TIMEOUT 300000     // 5 minutes (ms)
```

## Usage

### Web Interface

Access at: `http://192.168.1.XXX/`

Features:
- **Real-time temperature display**
- **Chamber temperature control** with target setting
- **Fan ON/OFF toggle** with visual indicator
- **LED ON/OFF toggle** with visual indicator
- **Control mode selector** (Bang-Bang vs Time-Proportional)
- **Emergency stop button**
- **Live relay status indicators**
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
  "heater_state": true,
  "fan": true,
  "led": false,
  "emergency": false,
  "mode": "bang-bang"
}
```

#### Set Chamber Temperature
```bash
curl -X POST http://192.168.1.XXX/api/chamber \
  -H "Content-Type: application/json" \
  -d '{"temperature": 50}'
```

#### Toggle Fan
```bash
curl -X POST http://192.168.1.XXX/api/fan \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

#### Toggle LED
```bash
curl -X POST http://192.168.1.XXX/api/led \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

#### Change Control Mode
```bash
curl -X POST http://192.168.1.XXX/api/mode \
  -H "Content-Type: application/json" \
  -d '{"mode": "time-proportional"}'
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
M106            # Turn on fan
M107            # Turn off fan
M150 S[value]   # Turn on LED (any value >0)
M150 S0         # Turn off LED
M112            # Emergency stop
```

**Note:** Fan and LED are binary (ON/OFF) with relays - speed/brightness values ignored

### MQTT Topics

#### Subscribe (Control)
```
printer/chamber/set    # Set chamber temp (payload: number)
printer/fan/set        # Set fan state (payload: "1"/"0" or "on"/"off")
printer/led/set        # Set LED state (payload: "1"/"0" or "on"/"off")
```

#### Publish (Status)
```
printer/chamber/temperature  # Current temperature
printer/chamber/target       # Target temperature
printer/chamber/heating      # Heating enabled (true/false)
printer/heater/state        # Heater relay state (ON/OFF)
printer/fan/state           # Fan relay state (ON/OFF)
printer/led/state           # LED relay state (ON/OFF)
printer/chamber/emergency   # Emergency events
```

#### MQTT Examples
```bash
# Set chamber to 50°C
mosquitto_pub -h 192.168.1.100 -t "printer/chamber/set" -m "50"

# Turn on fan
mosquitto_pub -h 192.168.1.100 -t "printer/fan/set" -m "1"

# Subscribe to status
mosquitto_sub -h 192.168.1.100 -t "printer/#"
```

## OrcaSlicer Integration

### Python Post-Processing Script

**chamber_control_relay.py:**
```python
#!/usr/bin/env python3
import sys
import requests

ESP32_IP = "192.168.1.XXX"  # Change this!

def send_command(endpoint, data):
    try:
        requests.post(
            f"http://{ESP32_IP}{endpoint}",
            json=data,
            timeout=2
        )
        return True
    except Exception as e:
        print(f"Warning: {e}")
        return False

def process_gcode(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Detect filament type
    chamber_temp = 0
    for line in lines[:100]:
        if "ABS" in line.upper() or "ASA" in line.upper():
            chamber_temp = 50
        elif "PETG" in line.upper():
            chamber_temp = 35
        elif "NYLON" in line.upper():
            chamber_temp = 55
    
    # Send preheat commands
    print("Sending chamber preheat commands...")
    
    if chamber_temp > 0:
        send_command("/api/chamber", {"temperature": chamber_temp})
        print(f"  Chamber: {chamber_temp}°C")
    
    send_command("/api/led", {"state": True})
    print("  LEDs: ON")
    
    send_command("/api/fan", {"state": False})
    print("  Fan: OFF (will turn on at end)")
    
    print(f"Chamber controller configured for {chamber_temp}°C")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: chamber_control_relay.py <gcode_file>")
        sys.exit(1)
    
    process_gcode(sys.argv[1])
```

**OrcaSlicer Setup:**
1. Save script to `/path/to/chamber_control_relay.py`
2. Make executable: `chmod +x chamber_control_relay.py`
3. OrcaSlicer → Edit → Preferences → Post-processing scripts
4. Add: `/path/to/chamber_control_relay.py`

### Manual Control Scripts

**preheat_abs.sh (Linux/Mac):**
```bash
#!/bin/bash
ESP32_IP="192.168.1.XXX"

echo "Preheating chamber for ABS..."
curl -X POST http://$ESP32_IP/api/chamber -H "Content-Type: application/json" -d '{"temperature": 50}'
curl -X POST http://$ESP32_IP/api/led -H "Content-Type: application/json" -d '{"state": true}'
echo "Chamber heating to 50°C, LEDs ON"
```

**preheat_abs.bat (Windows):**
```batch
@echo off
set ESP32_IP=192.168.1.XXX

echo Preheating chamber for ABS...
curl -X POST http://%ESP32_IP%/api/chamber -H "Content-Type: application/json" -d "{\"temperature\": 50}"
curl -X POST http://%ESP32_IP%/api/led -H "Content-Type: application/json" -d "{\"state\": true}"
echo Chamber heating to 50°C, LEDs ON
pause
```

### Recommended Chamber Temperatures

| Filament | Chamber Temp | Fan | Notes |
|----------|-------------|-----|-------|
| PLA | 0°C (off) | ON | Active cooling helpful |
| PETG | 30-35°C | OFF | Mild heat improves adhesion |
| ABS/ASA | 50-60°C | OFF | Critical for warp prevention |
| Nylon/PA | 55-70°C | OFF | High heat reduces moisture |
| PC | 60-70°C | OFF | Highest temps needed |
| TPU | 0-30°C | ON | Flexible, less heat needed |

## Home Assistant Integration

**configuration.yaml:**
```yaml
mqtt:
  sensor:
    - name: "Chamber Temperature"
      state_topic: "printer/chamber/temperature"
      unit_of_measurement: "°C"
      device_class: temperature
    
    - name: "Chamber Target"
      state_topic: "printer/chamber/target"
      unit_of_measurement: "°C"
  
  climate:
    - name: "3D Printer Chamber"
      mode_command_topic: "printer/chamber/mode"
      temperature_command_topic: "printer/chamber/set"
      temperature_state_topic: "printer/chamber/temperature"
      current_temperature_topic: "printer/chamber/temperature"
      min_temp: 0
      max_temp: 80
      temp_step: 1
      modes:
        - "off"
        - "heat"
  
  switch:
    - name: "Chamber Fan"
      command_topic: "printer/fan/set"
      state_topic: "printer/fan/state"
      payload_on: "1"
      payload_off: "0"
    
    - name: "Chamber Lights"
      command_topic: "printer/led/set"
      state_topic: "printer/led/state"
      payload_on: "1"
      payload_off: "0"
```

## Troubleshooting

### Relay Issues

**Problem:** Relay clicks but device doesn't turn on
- **Check wiring:** Verify COM, NO, NC connections
- **Test continuity:** Use multimeter in ohm mode
- **Voltage test:** Measure voltage at NO terminal when relay is ON
- **Load problem:** Test relay with different device (light bulb)

**Problem:** Relay doesn't click at all
- **Pin configuration:** Verify GPIO pins match your board
- **Active LOW/HIGH:** Try reversing RELAY_ON/RELAY_OFF
- **Board fault:** Test relay manually (short signal pin to GND)

**Problem:** All relays turn on together
- **Code error:** Check pin definitions don't overlap
- **Power issue:** Insufficient power to ESP32
- **Board defect:** May need replacement

**Problem:** Relay constantly cycles on/off rapidly
- **Temperature oscillation:** Use bang-bang mode, increase hysteresis
- **Sensor noise:** Move sensor away from electrical noise
- **Code loop:** Check for infinite loop conditions

### Temperature Sensor Issues

**Problem:** "DS18B20 sensor not found"
- **Check wiring:** VCC→3.3V, GND→GND, Data→GPIO4
- **Pullup resistor:** Must have 4.7kΩ between Data and VCC
- **Wrong sensor:** Ensure DS18B20, not DHT22
- **Sensor fault:** Try different sensor

**Problem:** Temperature reading -127°C
- **Bad connection:** Check all solder joints
- **Long wires:** Use shielded cable for >3m runs
- **Power issue:** Sensor not getting 3.3V

**Problem:** Temperature inaccurate
- **Calibration:** Test in ice water (0°C) and boiling water (100°C)
- **Sensor placement:** Must be inside chamber, not on wall
- **Air flow:** Ensure sensor reads chamber air, not just heater

### Heating Control Issues

**Problem:** Temperature overshoots target by >5°C
- **Switch to bang-bang:** More predictable for slow heaters
- **Increase hysteresis:** Change from 2°C to 3-4°C
- **Reduce heater power:** Use lower wattage heater
- **PID tuning:** Reduce Kp value

**Problem:** Temperature never reaches target
- **Insufficient heater:** Need higher wattage
- **Poor insulation:** Seal chamber better
- **Timeout too short:** Increase HEATER_TIMEOUT
- **Sensor placement:** Sensor too far from heater

**Problem:** Temperature swings ±5-10°C
- **Normal for bang-bang:** Expected with hysteresis
- **Switch to time-proportional:** Smoother control
- **Increase cycle time:** Change from 10s to 20s
- **Better insulation:** Reduces heat loss

### WiFi/Network Issues

**Problem:** Cannot find ESP32 IP address
- **Serial monitor:** Check for IP at startup (115200 baud)
- **Router admin:** Look for new device in DHCP list
- **Static IP:** Set static DHCP reservation in router
- **mDNS:** Try `http://chamber.local/` (may not work on all networks)

**Problem:** Web interface doesn't load
- **Firewall:** Disable temporarily to test
- **Same network:** Ensure PC and ESP32 on same subnet
- **Ping test:** `ping 192.168.1.XXX`
- **Port 80:** Some routers block HTTP

### Safety & Emergency Issues

**Problem:** Emergency stop not working
- **Test button:** Click emergency button on web interface
- **MQTT test:** Send emergency command via MQTT
- **Code issue:** Verify emergencyStop() function
- **Relay stuck:** May need manual intervention

**Problem:** Heater won't turn off
- **Relay fault:** Relay contacts welded shut (replace board)
- **Code stuck:** Upload code again
- **AC wiring:** IMMEDIATELY disconnect power if using AC
- **Emergency:** Unplug entire system

## Safety Warnings

⚠️ **CRITICAL ELECTRICAL SAFETY**

### AC Wiring (90-250V)
- **LETHAL VOLTAGE** - Can kill instantly
- **Only qualified electricians** should work with AC
- **All AC wiring** must be in grounded metal enclosure
- **Use proper wire gauge** (14 AWG minimum for 15A)
- **Follow local electrical codes** - Required by law
- **Install GFCI protection** - Prevents electrocution
- **Never work on live circuits** - Always disconnect power
- **Double-check all connections** - One mistake can be fatal

### DC Wiring (24V)
- **Low voltage but high current** - Can cause fires
- **Use proper wire gauge** for current draw
- **Install inline fuses** - Critical for fire prevention
- **Avoid short circuits** - Can melt wires

### Fire Prevention
- ⚠️ **Never leave heater unattended**
- ⚠️ **Do NOT exceed 80°C** without proper materials
- ⚠️ **Install smoke detector** near printer
- ⚠️ **Keep fire extinguisher** accessible (ABC type)
- ⚠️ **Test emergency stop** regularly
- ⚠️ **Ensure proper ventilation** - Prevent heat buildup
- ⚠️ **Use thermal fuse** on heater as backup

### Thermal Safety
- Heaters get **extremely hot** (>100°C surface temp)
- Allow **full cooldown** before handling (30+ minutes)
- Use **high-temp wire** (silicone insulation, >200°C rated)
- **Mount securely** - Prevent heater contact with plastic
- **Monitor first runs** - Stay nearby for first 5-10 heating cycles

### Relay Specifications
- **DO NOT EXCEED relay ratings** (typically 10A @ 250VAC)
- **Inductive loads** (motors, fans) need derating (use 50% of rating)
- **Resistive loads** (heaters) can use full rating
- **Check datasheet** for your specific relay board

### Testing Procedure
1. **Visual inspection** - Check all connections before power
2. **Low voltage test** - Test with 12V/24V first if possible
3. **Monitor temperature** - Use external thermometer first few runs
4. **Verify emergency stop** - Test before leaving unattended
5. **Check for hot spots** - Use IR thermometer on connections
6. **Smell check** - Any burning smell = immediate shutdown

## Relay Lifespan & Maintenance

### Expected Relay Life
- **Mechanical cycles:** 100,000+ (manufacturer rating)
- **Electrical cycles:** 10,000-100,000 (depends on load)
- **Time-proportional mode:** ~10-20 cycles/min = wears faster
- **Bang-bang mode:** ~2-6 cycles/min = longer life

### Maintenance Schedule
- **Weekly:** Listen for relay click sounds (should be crisp)
- **Monthly:** Check relay operation with multimeter
- **Every 6 months:** Inspect terminals for burning/arcing
- **Yearly:** Consider replacing high-cycle relays

### Signs of Relay Failure
- **Intermittent operation** - Contacts bouncing
- **Won't turn on** - Contacts oxidized
- **Won't turn off** - Contacts welded (DANGER!)
- **Buzzing sound** - Coil issue
- **Burning smell** - Overheated, replace immediately

### Extending Relay Life
- Use **bang-bang mode** for less critical control
- Add **RC snubber** for inductive loads
- Don't exceed **50% of relay rating**
- Keep relay board **cool and dust-free**

## Specifications

### Electrical Ratings (Typical)
- **Input Power:** AC 90-250V (built-in power supply)
- **Relay Rating:** 10A @ 250VAC, 10A @ 30VDC
- **Contact Type:** SPDT (Single Pole, Double Throw)
- **Control Voltage:** 3.3V (ESP32 GPIO)

### Temperature Control
- **Sensor:** DS18B20 (-55°C to +125°C)
- **Resolution:** 0.0625°C (12-bit)
- **Update Rate:** 1 second
- **Control Modes:** Bang-Bang, Time-Proportional
- **Safety Limit:** 80°C (configurable)
- **Hysteresis:** 2°C (adjustable)

### Network
- **WiFi:** 2.4GHz 802.11 b/g/n
- **Web Server:** HTTP on port 80
- **MQTT:** Port 1883 (configurable)
- **Update Rate:** 2 seconds (status)

### Control Performance

**Bang-Bang Mode:**
- Temperature stability: ±2-4°C
- Overshoot: <5°C
- Time to temp: 10-30 min (depends on heater)
- Relay cycles: 2-6 per minute

**Time-Proportional Mode:**
- Temperature stability: ±1-2°C
- Overshoot: <2°C
- Time to temp: 10-30 min
- Relay cycles: 10-20 per minute (more wear)

## Power Consumption

### AC Power (if using AC loads)
- ESP32 board: ~2W (built-in power supply)
- Chamber heater: 100-200W (typical)
- Fan: 5-20W
- LED: 10-30W
- **Total:** ~120-250W

### DC Power (if using 24V DC loads)
- 24V heater (100W): ~4.2A
- 24V fan (5W): ~0.2A
- 24V LED (12W): ~0.5A
- **Total 24V:** ~5A (requires 24V 6A+ PSU)
- **ESP32:** Powered separately from AC

## Comparison: Relay vs MOSFET

| Feature | Relay Version | MOSFET Version |
|---------|--------------|----------------|
| **Control** | ON/OFF only | PWM (0-100%) |
| **Switching Speed** | Slow (~10ms) | Fast (<1ms) |
| **Noise** | Audible clicking | Silent |
| **Lifespan** | Limited cycles | Unlimited |
| **Isolation** | Galvanically isolated | Not isolated |
| **AC Capability** | Yes (10A typical) | No (DC only) |
| **Complexity** | Simpler wiring | More complex |
| **Cost** | Higher | Lower |
| **Best For** | AC loads, simple control | DC loads, precise control |

## License

This project is released under MIT License. Use at your own risk.

**DISCLAIMER:** Working with electricity, especially AC power, is dangerous and potentially lethal. This project is provided for educational purposes. The author assumes no liability for damage, injury, or death resulting from use of this information. Always follow local electrical codes and consult a licensed electrician for AC installations.

## Credits

- ESP32 Arduino Core by Espressif
- DallasTemperature library by Miles Burton
- OneWire library by Paul Stoffregen
- ArduinoJson library by Benoit Blanchon

## Support

For issues or questions:
1. Check troubleshooting section
2. Verify all wiring (use multimeter)
3. Test relays individually
4. Check Serial Monitor (115200 baud)
5. Ensure correct RELAY_ON/RELAY_OFF polarity

## Version History

- **v1.0** - Initial release with dual control modes, web interface, MQTT support

---

⚠️ **FINAL WARNING:** This device controls heating elements capable of causing fires and severe burns. Never leave unattended. Always use proper safety measures including smoke detectors, fire extinguishers, and thermal protection. When working with AC power, hire a licensed electrician unless you are qualified to work with potentially lethal voltages.