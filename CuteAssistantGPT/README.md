# CuteAssistant for Kode Dot and Cardputer

CuteAssistant transforms the Kode Dot ESP32-S3 board (and the M5Stack Cardputer variant) into a self-contained voice assistant. The firmware combines touch interaction, live UI feedback, on-device audio capture, cloud speech-to-text reasoning, and hardware automation so the device can respond conversationally while controlling external electronics.

## Key Capabilities

- **Tap-to-talk experience** – the capacitive touch display wakes the assistant, begins recording through the onboard microphones, and shows animated eyes or prompts while listening.
- **Local audio pipeline** – the `AudioManager` configures I2S recording, captures 32 kHz mono audio, and normalises samples before handing them off for upload.
- **Network orchestration** – the `WiFiManager` establishes Wi-Fi connectivity, checks link health on an interval, and blocks request attempts until the network is ready.
- **Cloud AI integration** – choose between the streaming `RealtimeClient` (WebSocket) or the simpler `BasicGPTClient` (HTTP) for sending audio clips to the OpenAI API and receiving text replies.
- **Rich visual feedback** – the `DisplayManager` drives the LVGL UI while the `UIManager` animates eyes, typewriter text, status banners, and connection states.
- **Voice-driven hardware control** – responses are parsed into conversational text plus an action block so the assistant can toggle GPIO pins, sweep servos, or schedule delays in reaction to voice commands.
- **Ambient cues** – the `LEDManager` uses NeoPixel indicators to signal when the assistant is idle, recording, thinking, or performing an action.
- **Power management (Kode Dot build)** – the optional BQ25896 PMIC driver enables 5 V boost mode and monitors USB power status on the base board.

## Project Layout

```
├── src/
│   ├── main.cpp              # Firmware entry point and runtime loop
│   ├── fonts/, images/       # LVGL resources for the UI
│   └── lv_conf.h             # LVGL configuration overrides
├── lib/
│   ├── audio_manager/        # I2S capture and PCM processing helper
│   ├── basicgpt_client/      # HTTPS client for chat completions
│   ├── ui_manager/           # LVGL state machine and messaging
│   ├── led_manager/          # NeoPixel colour/state controller
│   ├── wifi_manager_lib/     # Wi-Fi provisioning and reconnection
│   └── kodedot_bsp/          # Board support package helpers
├── boards/                   # PlatformIO board descriptors
├── extra_scripts/            # Build helpers (serial auto-detect, binary rename)
├── include/                  # Shared headers
└── platformio.ini            # Build environments and dependencies
```

## Build Targets

Two PlatformIO environments are provided:

- `env:kode_dot` (default): targets the Kode Dot ESP32-S3 board with the custom display stack, PMIC boost logic, and NeoPixel status LED.
- `env:cardputer`: inherits the Kode Dot configuration but enables the `CARDPUTER_TARGET` flag and links against `M5Unified` so the firmware runs on the Cardputer form factor.

Switch environments from the command line:

```bash
# Build for the default Kode Dot hardware
pio run

# Build and upload to a Cardputer
pio run -e cardputer -t upload
```

## Getting Started

1. **Install dependencies** – follow the [PlatformIO installation guide](https://platformio.org/install) and ensure the `platformio` command is available in your shell.
2. **Clone the repository** – pull the project to your development machine and change into the `CuteAssistantGPT` directory.
3. **Configure secrets** – create a header such as `include/secrets.h` that defines your Wi-Fi credentials and OpenAI API key, then include it from `src/main.cpp` (the template deliberately omits sensitive data from version control).
4. **Select the hardware target** – edit `platformio.ini` or pass `-e kode_dot` / `-e cardputer` when running PlatformIO to match your board.
5. **Build and upload** – connect the device over USB-C and use `pio run -t upload` for your chosen environment. The serial monitor (`pio device monitor`) shows log output, including connectivity status and assistant responses.
6. **Verify audio levels** – if speech detection is unreliable, adjust the gain in `lib/audio_manager/audio_manager.cpp` and confirm the microphone wiring matches the board definition.

These steps bring the assistant online with default behaviours. From here you can customise prompts, UI themes, and action handlers to tailor the experience to your hardware setup.

## Runtime Flow

1. **Startup** – the display, audio codec, LEDs, and (for Kode Dot) the PMIC are initialised. The UI enters an idle state showing animated eyes.
2. **Wi-Fi provisioning** – stored credentials are applied and connectivity is checked on a timer (`WIFI_CHECK_INTERVAL_MS`). The UI reflects connection status.
3. **User interaction** – a touch event (debounced with `TOUCH_DEBOUNCE_MS`) triggers recording. The assistant plays earcons through the speaker and LEDs shift to a recording colour.
4. **Audio capture** – PCM chunks are queued for the background task, which either streams frames to the realtime WebSocket client or buffers them into a WAV file for the chat completion HTTP client.
5. **Inference and parsing** – the cloud response is parsed by `parseGPTResponse`, splitting the human-readable `Response:` from the structured `Actions:` block. Any `BEGIN;...;END` commands are converted into servo and GPIO instructions.
6. **Feedback and control** – the UI presents the reply with a typewriter effect, LEDs animate, and the command sequencer drives servos or toggles pins as requested.

## Configuration Highlights

- **API selection** – toggle between the streaming and HTTP workflows by editing the macros near the top of `src/main.cpp`:
  ```cpp
  // #define USE_REALTIME_API
  #define USE_CHAT_API
  ```
  Only one option should remain uncommented.
- **Actionable pins** – adjust the `AVAILABLE_GPIOS` array in `main.cpp` to match external peripherals. Separate defaults are supplied for the Cardputer base and the Kode Dot carrier board.
- **System prompt** – the `SYSTEM_PROMPT` constant defines the assistant personality and the command grammar expected from the cloud response. Customise it to fit your own branding or command set.
- **Timeouts and history** – constants such as `HTTP_TIMEOUT_MS`, `MAX_CONVERSATION_HISTORY`, and the GUI loop delay are collocated near the top of `main.cpp` for quick tuning.
- **Secrets management** – store your OpenAI API key securely (for example in a header excluded from version control) and inject it into the runtime configuration before making requests.

## Extending CuteAssistant

- **Add new hardware actions** – extend the `ActionCommand` enum and update `executeCommand` logic (in `src/main.cpp`) to handle additional peripherals like relays or RGB strips.
- **Custom UI themes** – modify assets in `src/fonts` and `src/images`, or extend the `UIManager` to add new panels, notifications, or animations.
- **Alternative backends** – implement another client under `lib/` that conforms to the same task interface (queue audio chunks, return parsed replies) and swap it in using the API macros.
- **Offline modes** – leverage the conversation history data structures to add cached replies or fallback behaviours when the network drops.

## Requirements

- PlatformIO 6.x with the Espressif32 platform
- An ESP32-S3 board with PSRAM (Kode Dot or Cardputer)
- I2S microphones and speaker connected per the board support package
- Internet connectivity for OpenAI API access

With these components in place you can build a friendly desk companion that speaks, listens, and interacts with the physical world through simple voice commands.
