# Set chamber to 50°C
curl -X POST http://192.168.1.XXX/api/chamber \
  -H "Content-Type: application/json" \
  -d '{"temperature": 50}'

# Set fan to 50%
curl -X POST http://192.168.1.XXX/api/fan \
  -H "Content-Type: application/json" \
  -d '{"speed": 128}'

# Set LED brightness to 75%
curl -X POST http://192.168.1.XXX/api/led \
  -H "Content-Type: application/json" \
  -d '{"brightness": 191}'

# Get status
curl http://192.168.1.XXX/api/status