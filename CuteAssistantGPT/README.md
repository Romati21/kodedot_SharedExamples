# CuteAssistantGPT 🤖

AI-powered voice assistant for ESP32-S3 devices with cute animated eyes, GPIO control, and sensor integration.

![CoreS3-Lite](https://img.shields.io/badge/M5Stack-CoreS3--Lite-blue)
![ESP32](https://img.shields.io/badge/ESP32-S3-green)
![OpenAI](https://img.shields.io/badge/OpenAI-GPT--4o--audio-orange)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## ✨ Features

### 🎤 Voice Interaction
- **Touch-to-talk**: Hold button to record, release to send
- **OpenAI GPT-4o-audio-preview**: Advanced speech recognition & natural conversation
- **Multi-language support**: Responds in English (expandable to other languages)
- **Real-time streaming**: Low-latency audio processing

### 👀 Animated UI
- **Cute animated eyes**: Blink, move, and react to state changes
- **State-based colors**:
  - ⚪ White: Ready (idle animation)
  - 🔴 Red: Recording
  - 🟣 Purple: Processing (thinking animation)
  - 🔵 Cyan: Connecting to WiFi
  - 🔴 Red (pulsing): Error
- **Typewriter text animation**: Smooth character-by-character display
- **GPT logo**: Top-right corner with animation during processing

### 🔌 GPIO Control
- **Voice-controlled pins**: "Turn on pin 5", "Blink pin 11"
- **Servo motor support**: "Move servo on pin 2 to 90 degrees"
- **Sequence commands**: "Blink pin 13 and move servo"
- **12 available pins** on CoreS3-Lite: 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 18

### 🌡️ Sensor Integration
- **BME688 Environmental Sensor**:
  - Temperature, humidity, pressure, air quality
  - "What's the temperature?" → "23.5°C"
- **ADXL345 Accelerometer**:
  - 3-axis acceleration, tilt detection
  - "Am I tilted?" → "You're tilted at 0.8g"
- **Hall Effect Sensor**:
  - Magnetic field detection
  - "Is there a magnet?" → "Yes, detected!"
- **RFID Reader**:
  - NFC/RFID card UID reading
  - "Read my card" → "UID: 04:A3:2C:BA"

### 🔋 Battery Management (CoreS3-Lite)
- **Battery percentage display**: Top-right corner
- **Color-coded indicator**: White (>50%), Yellow (20-50%), Red (<20%)
- **Critical threshold**: Recording blocked at <5% (unless charging)
- **Charging detection**: Auto-enables recording when plugged in

### 📡 WiFi Management
- **Multi-network support**: Up to 3 WiFi networks
- **Auto-reconnect**: Automatic retry with exponential backoff
- **SD card configuration**: Store credentials securely
- **RSSI monitoring**: Signal strength tracking

---

## 🎯 Supported Devices

| Device | Display | Touch | Audio | Status |
|--------|---------|-------|-------|--------|
| **M5Stack CoreS3-Lite** | 320x240 IPS | FT6336U | ES7210 | ✅ Full Support |
| M5Stack Cardputer | 240x135 IPS | Buttons | PDM Mic | ⚠️ Experimental |
| Kode Dot (Custom) | 410x502 QSPI | CST816S | SPH0645 | ✅ Original Target |

**This guide focuses on M5Stack CoreS3-Lite.**

---

## 🚀 Quick Start

### Prerequisites
- M5Stack CoreS3-Lite
- microSD card (FAT32 formatted)
- OpenAI API key with $5+ balance
- WiFi 2.4GHz network

### 1. Get OpenAI API Key (5 minutes)

1. Sign up: https://platform.openai.com/signup
2. Add payment method and fund account ($5-20 recommended)
3. Create API key: https://platform.openai.com/api-keys
4. Copy your key: `sk-proj-XXXXXXXXXX...`

**Cost**: ~$0.03-0.05 per voice query (100 queries ≈ $3-5)

### 2. Configure SD Card (2 minutes)

Create `wifi.txt` in SD card root:
```txt
NAME=Home WiFi
SSID=YourNetworkName
PASSWORD=YourPassword
---
```

Create `apis.txt` in SD card root:
```txt
OPENAI=sk-proj-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

Insert SD card into CoreS3-Lite (slot on back).

### 3. Flash Firmware (3 minutes)

Install [Visual Studio Code](https://code.visualstudio.com/) + **PlatformIO IDE** extension.

```bash
git clone https://github.com/Romati21/kodedot_SharedExamples.git
cd kodedot_SharedExamples/CuteAssistantGPT
```

In VS Code:
1. **File → Open Folder** → select `CuteAssistantGPT`
2. Connect CoreS3-Lite via USB-C
3. Click **Upload button** (→) at bottom
4. Select **env:cores3_lite**
5. Wait for compilation & upload (~2-3 minutes)

### 4. Use It!

1. Eyes appear on screen 👀
2. Wait for WiFi connection (~10 seconds)
3. **Hold button G** (below screen) → speak → **release**
4. Watch eyes turn red (recording) → purple (processing) → see response!

**Example**:
```
You: "Hello! What is your name?"
AI: "Hi there! I'm CuteAssistant, your friendly AI companion!"
```

📖 **Detailed Setup**: [docs/SETUP_CORES3_LITE.md](docs/SETUP_CORES3_LITE.md)

---

## 💬 Example Commands

### Conversation
```
"Hello! Tell me a joke"
"What's 42 times 137?"
"Sing me a lullaby"
"Tell me a fun fact"
```

### GPIO Control
```
"Turn on pin 5"
"Turn off pin 11"
"Blink pin 13"
"Move servo on pin 2 to 90 degrees"
"Sweep servo on pin 3"
```

### Sensor Readings
```
"What's the temperature?"
"Check humidity"
"Am I tilted?"
"Is there a magnet nearby?"
"Read my RFID card"
```

---

## 🏗️ Architecture

### Hardware Stack
```
ESP32-S3 (Dual-core Xtensa @ 240MHz)
├── 16MB Flash
├── 8MB PSRAM
├── ES7210 Digital Microphone (I2S)
├── 320x240 IPS Display (16-bit RGB565)
├── FT6336U Capacitive Touch
├── AXP2101 Battery Management
└── I2C Sensors (BME688, ADXL345, RFID)
```

### Software Stack
```
Arduino Framework (ESP-IDF core)
├── M5Unified 0.1.16 (hardware abstraction)
├── LVGL 9.0.0 (UI framework)
├── OpenAI GPT-4o-audio-preview API
├── FreeRTOS (dual-core task management)
├── ArduinoJson 7.0 (JSON parsing)
└── Custom Libraries:
    ├── AudioManager (I2S recording)
    ├── UIManager (LVGL UI + animations)
    ├── SensorManager (I2C sensor hub)
    ├── BasicGPTClient (OpenAI HTTP client)
    └── WiFiManager (network management)
```

### Project Structure
```
CuteAssistantGPT/
├── src/
│   └── main.cpp              # Main application logic
├── lib/
│   ├── audio_manager/        # I2S microphone driver
│   ├── ui_manager/           # LVGL UI + cute eyes animation
│   ├── sensor_manager/       # Unified sensor interface
│   ├── basicgpt_client/      # OpenAI API HTTP client
│   ├── wifi_manager_lib/     # WiFi + SD card config
│   ├── led_manager/          # NeoPixel LED control
│   └── kodedot_bsp/          # Hardware abstraction layer
├── boards/
│   └── cores3_lite.json      # PlatformIO board definition
├── docs/
│   ├── SETUP_CORES3_LITE.md  # Detailed setup guide
│   └── QUICK_START.md        # Fast 10-minute setup
├── specs/
│   ├── 001-cores3-lite-port/ # CoreS3 porting specification
│   └── 002-sensor-integration/ # Sensor integration spec
└── platformio.ini            # Build configuration
```

---

## 🎨 UI Design Standards

### Typography
- **Minimum font size**: 14px (readability on small screens)
- **Primary font**: Inter 30 (body text)
- **System font**: Montserrat 14 (indicators)
- **Line spacing**: ≥1.2x font height

### Touch Targets
- **Minimum tap area**: 40x40px
- **Spacing**: Adequate padding between elements

### Screen Adaptations

**CoreS3-Lite (320x240)**:
- Eyes: 80x80px circles
- Text area: 300x50px fixed at bottom (y=180)
- Text truncation: 100 chars max with "..."
- Battery indicator: Top-right below GPT logo

**Kode Dot (410x502)**:
- Eyes: 140x100px ovals
- Text area: 370x402px dynamic centered
- No truncation (full screen width)

---

## 🧩 Sensor Placeholder System

Instead of complex OpenAI Function Calling API, we use an efficient **placeholder method**:

### How It Works

1. **User asks**: "What's the temperature?"
2. **GPT responds**: "The temperature is [SENSOR:bme688:temperature] degrees."
3. **Device reads sensor**: BME688 → 23.5°C
4. **Replaces placeholder**: "[SENSOR:bme688:temperature]" → "23.5"
5. **Displays**: "The temperature is 23.5 degrees."

### Advantages
- ✅ **Single API call** (faster & cheaper)
- ✅ **Low memory usage** (no complex JSON parsing)
- ✅ **Simple implementation** (works with existing client)
- ✅ **Extensible** (easy to add new sensors)

### Placeholder Format
```
[SENSOR:device:parameter]

Examples:
[SENSOR:bme688:temperature]  → "23.5"
[SENSOR:adxl345:x]           → "0.82"
[SENSOR:hall:detect]         → "detected"
[SENSOR:rfid:uid]            → "04:A3:2C:BA"
```

---

## 🔧 Configuration

### WiFi Configuration (`/wifi.txt` on SD card)
```txt
# Primary network
NAME=Home WiFi
SSID=MyHomeNetwork
PASSWORD=SecurePassword123
---

# Backup network
NAME=Mobile Hotspot
SSID=iPhone_Hotspot
PASSWORD=HotspotPass
---

# Office network
NAME=Work WiFi
SSID=OfficeNet
PASSWORD=WorkPass456
END
```

### API Keys (`/apis.txt` on SD card)
```txt
# OpenAI API Key
OPENAI=sk-proj-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
```

### System Prompt Customization

Edit `SYSTEM_PROMPT` in `src/main.cpp` to change assistant personality:
```cpp
static const char* SYSTEM_PROMPT =
    "You are CuteAssistant, a friendly AI companion..."
    // Customize behavior here
```

---

## 📊 Performance Metrics

### Memory Usage
- **Flash**: ~1.5MB (16MB available)
- **SRAM**: ~180KB (512KB available)
- **PSRAM**: ~2MB for LVGL buffers (8MB available)

### Battery Life (CoreS3-Lite)
- **Idle (WiFi connected)**: ~6 hours
- **Active conversation**: ~4 hours
- **Recording**: ~3.5 hours
- **Charging time**: ~2 hours (USB-C 5V/2A)

### API Costs (OpenAI GPT-4o-audio-preview)
- **Per query**: $0.03-0.05 (varies with audio length)
- **100 queries**: ~$3-5
- **Daily usage (10-20 queries)**: ~$1-2/day

---

## 🛠️ Development

### Build Commands
```bash
# Compile for CoreS3-Lite
pio run -e cores3_lite

# Upload firmware
pio run -e cores3_lite -t upload

# Serial monitor
pio device monitor -b 115200

# Clean build
pio run -t clean

# Size analysis
pio run -e cores3_lite -t size
```

### Adding New Sensors

1. Create sensor class:
```cpp
// lib/sensor_manager/include/sensor_manager/NewSensor.h
class NewSensor {
public:
    bool begin(TwoWire* wire);
    bool readValue(float& value);
};
```

2. Register in SensorManager:
```cpp
// SensorManager.cpp
if (scanI2CAddress(0xXX)) {
    newSensor_ = new NewSensor();
    if (newSensor_->begin(&Wire)) {
        newSensorAvailable_ = true;
    }
}
```

3. Add placeholder handler:
```cpp
// readSensorByString()
else if (deviceLower == "newsensor") {
    float value;
    if (newSensor_->readValue(value)) {
        return String(value, 2);
    }
}
```

4. Update system prompt with new sensor examples

---

## 📖 Documentation

- **Setup Guide**: [docs/SETUP_CORES3_LITE.md](docs/SETUP_CORES3_LITE.md)
- **Quick Start**: [docs/QUICK_START.md](docs/QUICK_START.md)
- **CoreS3 Port Spec**: [specs/001-cores3-lite-port/](specs/001-cores3-lite-port/)
- **Sensor Integration**: [specs/002-sensor-integration/](specs/002-sensor-integration/)
- **BSP Documentation**: [lib/kodedot_bsp/README.md](lib/kodedot_bsp/README.md)
- **UI Manager**: [lib/ui_manager/README.md](lib/ui_manager/README.md)
- **Sensor Manager**: [lib/sensor_manager/README.md](lib/sensor_manager/README.md)

---

## 🤝 Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 🐛 Troubleshooting

### WiFi Issues
- Use **2.4GHz WiFi only** (CoreS3 doesn't support 5GHz)
- Check `wifi.txt` format (no spaces around `=`)
- Ensure SD card is FAT32 formatted

### OpenAI Errors
- **401**: Invalid API key → check `apis.txt`
- **429**: Rate limit → wait 1 minute or upgrade tier
- **Insufficient quota**: Add funds to OpenAI account

### Sensor Detection
- Verify I2C connections: SDA=GPIO11, SCL=GPIO12
- Check sensor addresses: BME688=0x77, ADXL345=0x53
- Try sensors individually to rule out conflicts

📚 **Full troubleshooting**: [docs/SETUP_CORES3_LITE.md#troubleshooting](docs/SETUP_CORES3_LITE.md#устранение-проблем)

---

## 📜 License

MIT License - see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- **M5Stack** for CoreS3 hardware & M5Unified library
- **OpenAI** for GPT-4o-audio-preview API
- **Adafruit** for sensor libraries
- **LVGL** team for amazing UI framework
- **PlatformIO** for build system

---

## 📞 Support

- **Issues**: https://github.com/Romati21/kodedot_SharedExamples/issues
- **Discussions**: https://github.com/Romati21/kodedot_SharedExamples/discussions
- **Email**: roman@kodedot.com (if provided)

---

## 🎉 Enjoy CuteAssistantGPT!

**Star ⭐ this repo** if you find it useful!

Made with ❤️ by the KodeDot community
