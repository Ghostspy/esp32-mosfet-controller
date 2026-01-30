; === Your normal end G-code ===
G91
G1 Z10 F3000
G90
G1 X0 Y220 F12000
M106 S0
M104 S0
M140 S0
M84

; === Chamber Control End ===
M118 Cooling down chamber
; Turn off chamber heater, max exhaust fan for cooling
; Wait 2 minutes for cooling, then turn off fan and LEDs