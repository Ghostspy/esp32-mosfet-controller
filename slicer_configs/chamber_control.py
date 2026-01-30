#!/usr/bin/env python3
import sys
import requests
import re

# Your ESP32 IP address
ESP32_IP = "192.168.1.XXX"  # Change this!

def send_chamber_command(gcode):
    """Send G-code command to ESP32 chamber controller"""
    try:
        response = requests.post(
            f"http://{ESP32_IP}/gcode",
            data=gcode,
            timeout=2
        )
        return response.status_code == 200
    except Exception as e:
        print(f"Warning: Could not reach chamber controller: {e}")
        return False

def process_gcode_file(filename):
    """Process G-code file and inject chamber control commands"""
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    output_lines = []
    filament_type = "PLA"  # Default
    chamber_temp = 0
    
    # Detect filament type from comments
    for line in lines[:100]:  # Check first 100 lines
        if "filament_type" in line.lower():
            if "ABS" in line.upper() or "ASA" in line.upper():
                filament_type = "ABS"
                chamber_temp = 50
            elif "PETG" in line.upper():
                chamber_temp = 35
            elif "NYLON" in line.upper() or "PA" in line.upper():
                chamber_temp = 55
    
    # Track if we've inserted start commands
    start_inserted = False
    
    for i, line in enumerate(lines):
        # Insert chamber commands after bed/nozzle heating starts
        if not start_inserted and ("M140" in line or "M190" in line or "M104" in line or "M109" in line):
            output_lines.append("; === ESP32 Chamber Controller ===\n")
            output_lines.append(f"; Detected filament: {filament_type}\n")
            
            # Send commands to ESP32
            if chamber_temp > 0:
                output_lines.append(f"; Setting chamber to {chamber_temp}°C\n")
                send_chamber_command(f"M141 S{chamber_temp}")
            
            # Turn on LEDs
            output_lines.append("; Turning on chamber LEDs\n")
            send_chamber_command("M150 S255")
            
            # Start exhaust fan at 30%
            output_lines.append("; Starting exhaust fan (30%)\n")
            send_chamber_command("M106 S77")
            
            output_lines.append("; ===================================\n")
            start_inserted = True
        
        output_lines.append(line)
        
        # Insert end commands at print end
        if "M84" in line or ("End of" in line and "code" in line.lower()):
            output_lines.append("\n; === Chamber Cooldown ===\n")
            output_lines.append("; Turning off chamber heater\n")
            send_chamber_command("M141 S0")
            
            output_lines.append("; Setting exhaust fan to 100% for cooling\n")
            send_chamber_command("M106 S255")
            
            output_lines.append("; Chamber will cool for 2 minutes, then turn off automatically\n")
            output_lines.append("; (Manual: visit http://{} to control)\n".format(ESP32_IP))
    
    # Write modified G-code
    with open(filename, 'w') as f:
        f.writelines(output_lines)
    
    print(f"Chamber control commands added. Filament: {filament_type}, Chamber: {chamber_temp}°C")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: chamber_control.py <gcode_file>")
        sys.exit(1)
    
    gcode_file = sys.argv[1]
    process_gcode_file(gcode_file)