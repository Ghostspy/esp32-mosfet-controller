# Create a script: preheat_chamber.sh
#!/bin/bash
curl -X POST http://192.168.1.XXX/api/chamber -H "Content-Type: application/json" -d '{"temperature": 50}'
curl -X POST http://192.168.1.XXX/api/led -H "Content-Type: application/json" -d '{"brightness": 255}'
curl -X POST http://192.168.1.XXX/api/fan -H "Content-Type: application/json" -d '{"speed": 77}'
echo "Chamber preheating to 50°C"