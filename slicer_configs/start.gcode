; === Chamber Control Start ===
M118 Starting chamber preheat
; Use curl to set chamber temperature
M118 Sending chamber preheat command...

; Set chamber to 45°C (adjust as needed for your filament)
; You'll need to enable "Allow shell commands" in OrcaSlicer preferences

; Chamber preheat
{if filament_type[0]=="ABS" or filament_type[0]=="ASA"}
  M118 ABS/ASA detected - preheating chamber to 50°C
{endif}
{if filament_type[0]=="PLA"}
  M118 PLA detected - chamber heater off
{endif}

; Turn on LEDs
M118 Turning on chamber LEDs

; Start exhaust fan at low speed
M118 Starting exhaust fan

; === Your normal start G-code continues here ===
G28 ; home all axes
; ... rest of your start code