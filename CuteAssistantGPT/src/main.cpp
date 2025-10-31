/*
 * CuteAssistant - AI Voice Assistant with ESP32-S3 Kode Dot
 * ---------------------------------------------------------
 * - Touch screen to talk to the assistant
 * - Animated cute eyes respond to interaction
 * - AI responses shown on screen with typewriter effect
 * - Eyes disappear when showing text responses
 * - GPIO control via voice commands
 */
#include <Arduino.h>
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <vector>
#include <ESP32Servo.h>
#include <Wire.h>
#if defined(CARDPUTER_TARGET) || defined(CORES3_LITE_TARGET)
#include <M5Unified.h>

// Compile-time M5Unified version check per FR-015/A-001
// M5Unified version format: M5UNIFIED_VERSION = 0xMMNNPP (Major.Minor.Patch)
// Required: 0.1.16-0.1.x (0x000116 to 0x0001FF)
#ifndef M5UNIFIED_VERSION
#error "M5UNIFIED_VERSION not defined. Please update M5Unified library."
#endif

#if M5UNIFIED_VERSION < 0x000116
#error "M5Unified version too old. Minimum required: 0.1.16"
#endif

#if M5UNIFIED_VERSION >= 0x000200
#error "M5Unified version 0.2.x+ not supported. Maximum supported: 0.1.x"
#endif

#else
#include <PMIC_BQ25896.h>
#endif

// ==================== API SELECTION ====================
// Uncomment ONE of these to choose API implementation:
// #define USE_REALTIME_API      // WebSocket streaming (ultra-low latency)
#define USE_CHAT_API          // HTTP POST (stable, proven)

// Project libraries
#include <audio_manager/AudioManager.h>
#include <ui_manager/UIManager.h>
#include <led_manager/LEDManager.h>
#include <wifi_manager_lib/WiFiManager.h>

#ifdef USE_REALTIME_API
#include <realtime_client/RealtimeClient.h>
#else
#include <basicgpt_client/BasicGPTClient.h>
#endif

// Generated resources
extern const lv_font_t Inter_30;

// Core managers
DisplayManager display;
AudioManager audioManager;
UIManager uiManager;
LEDManager ledManager;
#if !defined(CARDPUTER_TARGET) && !defined(CORES3_LITE_TARGET)
PMIC_BQ25896 pmic;
#endif

// Configuration
static const uint32_t GUI_LOOP_DELAY_MS = 5;
static const uint32_t WIFI_CHECK_INTERVAL_MS = 15000;
static const uint32_t TOUCH_DEBOUNCE_MS = 200;    // Prevent rapid touches

#ifdef CORES3_LITE_TARGET
// Available GPIO pins for user control on CoreS3-Lite (GPIO 14 excluded - microphone I2S DIN)
static const int AVAILABLE_GPIOS[] = {1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 18};
#define AVAILABLE_GPIO_LIST_STR "1,2,5,6,7,8,9,10,11,12,13,18"
#elif defined(CARDPUTER_TARGET)
// Available GPIO pins for user control on Cardputer base (configurable)
static const int AVAILABLE_GPIOS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
#define AVAILABLE_GPIO_LIST_STR "1,2,3,4,5,6,7,8,9,10"
#else
// Available GPIO pins for user control (from pinout diagram)
static const int AVAILABLE_GPIOS[] = {1, 2, 3, 11, 12, 13, 39, 40, 41, 42};
#define AVAILABLE_GPIO_LIST_STR "1,2,3,11,12,13,39,40,41,42"
#endif
static const int NUM_AVAILABLE_GPIOS = sizeof(AVAILABLE_GPIOS) / sizeof(AVAILABLE_GPIOS[0]);

#if !defined(CARDPUTER_TARGET) && !defined(CORES3_LITE_TARGET)
// ==================== PMIC BQ25896 - 5V BUS CONTROL ====================
static void initPMIC() {
    Serial.println("[PMIC] Inicializando BQ25896 para habilitar bus 5V...");

    // NO llamar a Wire.begin() aquí - ya está inicializado por el display
    // Solo inicializar el objeto PMIC
    pmic.begin();
    delay(200);

    // Habilitar conversión continua de ADC (1 Hz) para refrescar medidas
    pmic.setCONV_RATE(true);
    delay(50);

    // CONFIGURAR BOOST VOLTAGE (5V típico)
    // El registro BOOSTV controla el voltaje de salida del boost
    pmic.setBOOST_LIM(true);  // Set current limit for boost
    delay(50);

    // HABILITAR OTG/BOOST MODE - 5V de salida en el bus
    Serial.println("[PMIC] Habilitando modo OTG/Boost para 5V...");
    pmic.setOTG_CONFIG(true);  // enable OTG/boost (5V out)
    delay(100);  // Dar tiempo para que se estabilice

    // Verificar estado
    auto vstat = pmic.get_VBUS_STAT_reg();
    bool haveUsb = vstat.pg_stat;  // Power Good on VBUS

    Serial.printf("[PMIC] USB=%s, Estado carga=%d\n", haveUsb ? "conectado" : "no conectado", vstat.chrg_stat);
    Serial.println("[PMIC] ✅ Bus de 5V HABILITADO - Modo OTG/Boost activado");
}
#endif

// Servo management (max 10 servos)
#define CUTEASSISTANT_MAX_SERVOS 10
static Servo g_servos[CUTEASSISTANT_MAX_SERVOS];
static int g_servo_pins[CUTEASSISTANT_MAX_SERVOS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// Command execution state machine
enum class CommandState {
    IDLE,
    EXECUTING,
    WAITING
};

struct ActionCommand {
    enum Type {
        GPIO_SET,
        SERVO_SET,
        DELAY,
        UNKNOWN
    } type;
    
    int pin;
    bool state;        // For GPIO: true=HIGH, false=LOW
    int angle;         // For SERVO: 0-180 degrees
    uint32_t delay_ms; // For DELAY command
};

struct CommandSequence {
    std::vector<ActionCommand> commands;
    size_t currentIndex;
    uint32_t waitUntil;
    CommandState state;
    
    CommandSequence() : currentIndex(0), waitUntil(0), state(CommandState::IDLE) {}
};

static CommandSequence g_active_sequence;

// OpenAI Configuration
static const char* OPENAI_HOST = "api.openai.com";
static const int OPENAI_PORT = 443;
static const char* OPENAI_MODEL = "gpt-4o-audio-preview";
// Optimized assistant prompt with structured output
// Optimized assistant prompt with structured output and GPIO control
static const char* SYSTEM_PROMPT =
    "You are CuteAssistant, a friendly AI companion who imagines having a human body to interact with the world. "
    "You communicate in clear, conversational English with warmth and subtle wit. "
    "Keep responses concise (2-3 sentences) and engaging. Never use emojis - keep text clean. "
    "\n\nCRITICAL OUTPUT FORMAT - You MUST ALWAYS respond EXACTLY like this:"
    "\nResponse: [Your conversational reply ONLY - no actions here]"
    "\nActions: [Control commands OR simple action description]"
    "\n\nIMPORTANT RULES:"
    "\n1. The Response line is what the USER SEES on screen - keep it conversational and clean"
    "\n2. The Actions line is for hardware control commands - the user NEVER sees this"
    "\n3. NEVER mix control commands into your Response text"
    "\n4. ALWAYS include both lines, even if Actions is just 'none'"
    "\n\nHARDWARE CONTROL FORMAT:"
    "\nBEGIN;GPIO(pin,state);SERVO(pin,angle);DELAY(ms);END"
    "\nAvailable pins: " AVAILABLE_GPIO_LIST_STR
    "\nGPIO states: ON or OFF"
    "\nServo angles: 0-180 degrees"
    "\n\nEXAMPLES:"
    "\nUser: 'Turn on pin 11'"
    "\nResponse: Sure! I've activated pin 11 for you."
    "\nActions: BEGIN;GPIO(11,ON);END"
    "\n\nUser: 'Move servo on pin 2 to 90 degrees'"
    "\nResponse: Moving the servo to center position!"
    "\nActions: BEGIN;SERVO(2,90);END"
    "\n\nUser: 'Sweep servo on pin 3'"
    "\nResponse: Watch this! I'll sweep the servo for you."
    "\nActions: BEGIN;SERVO(3,0);DELAY(500);SERVO(3,90);DELAY(500);SERVO(3,180);DELAY(500);SERVO(3,90);END"
    "\n\nUser: 'Blink pin 13 and move servo'"
    "\nResponse: I'll do both actions together!"
    "\nActions: BEGIN;GPIO(13,ON);SERVO(2,45);DELAY(1000);GPIO(13,OFF);SERVO(2,135);END"
    "\n\nUser: 'Hello!'"
    "\nResponse: Hey there! How can I help you today?"
    "\nActions: wave_hand_friendly"
    "\n\nREMEMBER:"
    "\n- Response = what user sees (conversational text)"
    "\n- Actions = hardware commands (never shown to user)"
    "\n- You can control GPIOs AND servos in the same sequence"
    "\n- Keep them separate!";
static const uint32_t HTTP_TIMEOUT_MS = 20000;
static const uint32_t MAX_CONVERSATION_HISTORY = 6; // Keep last 3 exchanges (user + assistant)

// Conversational memory structure
struct ConversationMessage {
    String role;    // "user" or "assistant"
    String content;
};

// Audio streaming structures
struct AudioChunk {
    uint8_t* data;
    size_t size;
};

#define MAX_AUDIO_CHUNKS 32
#define CHUNK_POOL_SIZE 8192  // 8KB per chunk

// State management
static TaskHandle_t g_openai_task = nullptr;
static uint32_t g_last_wifi_check_ms = 0;
static uint32_t g_bench_start_ms = 0;
static QueueHandle_t g_led_queue = nullptr; // async LED requests
static QueueHandle_t g_audio_chunk_queue = nullptr; // streaming audio chunks
static uint32_t g_last_touch_ms = 0; // Touch debouncing
static std::vector<ChatMessage> g_conversation_history; // Memory for context
static bool g_streaming_active = false; // Flag to control streaming

// Battery monitoring (CoreS3-Lite/Cardputer only)
#if defined(CORES3_LITE_TARGET) || defined(CARDPUTER_TARGET)
static uint8_t g_battery_percent = 100; // Cached battery percentage
static uint32_t g_last_battery_check_ms = 0; // Last battery check timestamp
static const uint32_t BATTERY_CHECK_INTERVAL_MS = 30000; // 30 seconds per SC-007
static bool g_recording_blocked_low_battery = false; // Block recording when battery <5%
#endif

// Forward declarations
static void openaiTask(void *arg); // OpenAI task declaration

// Structure to hold parsed GPT response
struct ParsedResponse {
    String displayText;  // Text to show on screen
    String action;       // Action command for device
    bool valid;          // Whether parsing was successful
};

// Parse structured GPT response into display text and action
static ParsedResponse parseGPTResponse(const String& rawResponse) {
    ParsedResponse result;
    result.valid = false;
    result.action = "none";
    result.displayText = "";
    
    Serial.println("\n=== PARSING GPT RESPONSE ===");
    Serial.printf("Raw response: '%s'\n", rawResponse.c_str());
    
    String workingText = rawResponse;
    workingText.trim();
    
    // Step 1: Extract GPIO/SERVO commands (BEGIN...END blocks)
    int beginIdx = workingText.indexOf("BEGIN;");
    int endIdx = workingText.indexOf(";END");
    
    if (beginIdx != -1 && endIdx != -1 && endIdx > beginIdx) {
        // Extract the command block
        result.action = workingText.substring(beginIdx, endIdx + 4); // Include ";END"
        result.action.trim();
        
        // Remove it from display text
        String beforeCmd = workingText.substring(0, beginIdx);
        String afterCmd = workingText.substring(endIdx + 4);
        workingText = beforeCmd + afterCmd;
        
        Serial.printf("📦 Extracted command block: '%s'\n", result.action.c_str());
    }
    
    // Step 2: Find "Response:" and "Actions:" markers
    int responseIdx = workingText.indexOf("Response:");
    int actionIdx = workingText.indexOf("Actions:");
    
    Serial.printf("Response marker: %d, Actions marker: %d\n", responseIdx, actionIdx);
    
    // Step 3: Extract response text
    if (responseIdx != -1) {
        int responseStart = responseIdx + 9; // Length of "Response:"
        int responseEnd = (actionIdx != -1) ? actionIdx : workingText.length();
        result.displayText = workingText.substring(responseStart, responseEnd);
    } else {
        // No "Response:" marker - use everything except "Actions:" line
        if (actionIdx != -1) {
            result.displayText = workingText.substring(0, actionIdx);
        } else {
            result.displayText = workingText;
        }
    }
    
    result.displayText.trim();
    
    // Step 4: Extract action from "Actions:" line if present and no BEGIN...END found
    if (actionIdx != -1 && result.action == "none") {
        int actionStart = actionIdx + 8; // Length of "Actions:"
        String actionText = workingText.substring(actionStart);
        actionText.trim();
        
        // Check if it's a line break, then skip it
        int newlinePos = actionText.indexOf('\n');
        if (newlinePos != -1) {
            actionText = actionText.substring(0, newlinePos);
            actionText.trim();
        }
        
        if (actionText.length() > 0 && actionText != "none") {
            result.action = actionText;
        }
    }
    
    // Step 5: Final cleanup - remove any remaining "Actions:" lines from display text
    int actionsLineIdx = result.displayText.indexOf("Actions:");
    if (actionsLineIdx != -1) {
        result.displayText = result.displayText.substring(0, actionsLineIdx);
        result.displayText.trim();
    }
    
    result.valid = true;
    Serial.printf("✅ Display text: '%s'\n", result.displayText.c_str());
    Serial.printf("✅ Action: '%s'\n", result.action.c_str());
    Serial.println("============================\n");
    
    return result;
}

// Execute device action based on GPT command
// Forward declarations
static void openaiTask(void *arg); // OpenAI task declaration

// GPIO Command Parser and Executor
// Format: BEGIN;GPIO(11,ON);DELAY(1000);GPIO(11,OFF);END

static bool isGPIOAvailable(int pin) {
    for (int i = 0; i < NUM_AVAILABLE_GPIOS; i++) {
        if (AVAILABLE_GPIOS[i] == pin) return true;
    }
    return false;
}

// Find servo index for a pin, or -1 if not found
static int findServoIndex(int pin) {
    for (int i = 0; i < CUTEASSISTANT_MAX_SERVOS; i++) {
        if (g_servo_pins[i] == pin) return i;
    }
    return -1;
}

// Attach servo to pin (reuse existing or find empty slot)
static bool attachServo(int pin, int& servoIndex) {
    if (!isGPIOAvailable(pin)) {
        Serial.printf("[SERVO] Error: Pin %d not available\n", pin);
        return false;
    }
    
    // Check if servo already attached to this pin
    servoIndex = findServoIndex(pin);
    if (servoIndex != -1) {
        return true; // Already attached
    }
    
    // Find empty slot
    for (int i = 0; i < CUTEASSISTANT_MAX_SERVOS; i++) {
        if (g_servo_pins[i] == -1) {
            g_servos[i].attach(pin);
            g_servo_pins[i] = pin;
            servoIndex = i;
            Serial.printf("[SERVO] Attached servo to pin %d (slot %d)\n", pin, i);
            return true;
        }
    }
    
    Serial.println("[SERVO] Error: No free servo slots");
    return false;
}

static ActionCommand parseCommand(const String& cmd) {
    ActionCommand action;
    action.type = ActionCommand::UNKNOWN;
    
    String trimmed = cmd;
    trimmed.trim();
    
    // Parse GPIO command: GPIO(pin,state)
    if (trimmed.startsWith("GPIO(")) {
        int openParen = trimmed.indexOf('(');
        int comma = trimmed.indexOf(',');
        int closeParen = trimmed.indexOf(')');
        
        if (openParen != -1 && comma != -1 && closeParen != -1) {
            String pinStr = trimmed.substring(openParen + 1, comma);
            String stateStr = trimmed.substring(comma + 1, closeParen);
            
            pinStr.trim();
            stateStr.trim();
            stateStr.toUpperCase();
            
            action.pin = pinStr.toInt();
            action.state = (stateStr == "ON" || stateStr == "HIGH" || stateStr == "1");
            action.type = ActionCommand::GPIO_SET;
            
            Serial.printf("[CMD] Parsed GPIO: pin=%d, state=%s\n", 
                         action.pin, action.state ? "HIGH" : "LOW");
        }
    }
    // Parse SERVO command: SERVO(pin,angle)
    else if (trimmed.startsWith("SERVO(")) {
        int openParen = trimmed.indexOf('(');
        int comma = trimmed.indexOf(',');
        int closeParen = trimmed.indexOf(')');
        
        if (openParen != -1 && comma != -1 && closeParen != -1) {
            String pinStr = trimmed.substring(openParen + 1, comma);
            String angleStr = trimmed.substring(comma + 1, closeParen);
            
            pinStr.trim();
            angleStr.trim();
            
            action.pin = pinStr.toInt();
            action.angle = angleStr.toInt();
            
            // Clamp angle to valid servo range
            if (action.angle < 0) action.angle = 0;
            if (action.angle > 180) action.angle = 180;
            
            action.type = ActionCommand::SERVO_SET;
            
            Serial.printf("[CMD] Parsed SERVO: pin=%d, angle=%d°\n", 
                         action.pin, action.angle);
        }
    }
    // Parse DELAY command: DELAY(milliseconds)
    else if (trimmed.startsWith("DELAY(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.indexOf(')');
        
        if (openParen != -1 && closeParen != -1) {
            String delayStr = trimmed.substring(openParen + 1, closeParen);
            delayStr.trim();
            
            action.delay_ms = delayStr.toInt();
            action.type = ActionCommand::DELAY;
            
            Serial.printf("[CMD] Parsed DELAY: %u ms\n", action.delay_ms);
        }
    }
    
    return action;
}

static void parseActionSequence(const String& actionStr, CommandSequence& sequence) {
    sequence.commands.clear();
    sequence.currentIndex = 0;
    sequence.waitUntil = 0;
    sequence.state = CommandState::IDLE;
    
    // Check for BEGIN and END markers
    int beginIdx = actionStr.indexOf("BEGIN");
    int endIdx = actionStr.indexOf("END");
    
    if (beginIdx == -1 || endIdx == -1) {
        Serial.println("[CMD] Warning: No BEGIN/END markers found in action sequence");
        return;
    }
    
    // Extract commands between BEGIN and END
    String commandsStr = actionStr.substring(beginIdx + 5, endIdx);
    commandsStr.trim();
    
    Serial.printf("[CMD] Parsing action sequence: %s\n", commandsStr.c_str());
    
    // Split by semicolon
    int lastPos = 0;
    int pos = 0;
    while ((pos = commandsStr.indexOf(';', lastPos)) != -1) {
        String cmd = commandsStr.substring(lastPos, pos);
        cmd.trim();
        
        if (cmd.length() > 0) {
            ActionCommand action = parseCommand(cmd);
            if (action.type != ActionCommand::UNKNOWN) {
                sequence.commands.push_back(action);
            }
        }
        
        lastPos = pos + 1;
    }
    
    // Process last command if no trailing semicolon
    if (lastPos < commandsStr.length()) {
        String cmd = commandsStr.substring(lastPos);
        cmd.trim();
        if (cmd.length() > 0) {
            ActionCommand action = parseCommand(cmd);
            if (action.type != ActionCommand::UNKNOWN) {
                sequence.commands.push_back(action);
            }
        }
    }
    
    Serial.printf("[CMD] Parsed %d commands in sequence\n", sequence.commands.size());
    
    if (sequence.commands.size() > 0) {
        sequence.state = CommandState::EXECUTING;
    }
}

static void executeNextCommand(CommandSequence& sequence) {
    if (sequence.state != CommandState::EXECUTING) return;
    if (sequence.currentIndex >= sequence.commands.size()) {
        Serial.println("[CMD] Sequence completed");
        sequence.state = CommandState::IDLE;
        return;
    }
    
    ActionCommand& cmd = sequence.commands[sequence.currentIndex];
    
    switch (cmd.type) {
        case ActionCommand::GPIO_SET:
            if (isGPIOAvailable(cmd.pin)) {
                pinMode(cmd.pin, OUTPUT);
                digitalWrite(cmd.pin, cmd.state ? HIGH : LOW);
                Serial.printf("[CMD] Executed: GPIO%d = %s\n", 
                             cmd.pin, cmd.state ? "HIGH" : "LOW");
            } else {
                Serial.printf("[CMD] Error: GPIO%d not available\n", cmd.pin);
            }
            sequence.currentIndex++;
            break;
            
        case ActionCommand::SERVO_SET: {
            int servoIdx;
            if (attachServo(cmd.pin, servoIdx)) {
                g_servos[servoIdx].write(cmd.angle);
                Serial.printf("[CMD] Executed: SERVO%d = %d°\n", 
                             cmd.pin, cmd.angle);
            } else {
                Serial.printf("[CMD] Error: Failed to attach servo on pin %d\n", cmd.pin);
            }
            sequence.currentIndex++;
            break;
        }
            
        case ActionCommand::DELAY:
            Serial.printf("[CMD] Executing: DELAY %u ms\n", cmd.delay_ms);
            sequence.waitUntil = millis() + cmd.delay_ms;
            sequence.state = CommandState::WAITING;
            sequence.currentIndex++;
            break;
            
        default:
            Serial.println("[CMD] Error: Unknown command type");
            sequence.currentIndex++;
            break;
    }
}

static void updateCommandSequence() {
    if (g_active_sequence.state == CommandState::IDLE) return;
    
    if (g_active_sequence.state == CommandState::WAITING) {
        if (millis() >= g_active_sequence.waitUntil) {
            g_active_sequence.state = CommandState::EXECUTING;
        } else {
            return; // Still waiting
        }
    }
    
    if (g_active_sequence.state == CommandState::EXECUTING) {
        executeNextCommand(g_active_sequence);
    }
}

// Helper function to normalize Unicode text for font compatibility
static String normalizeTextForDisplay(const String& text) {
    String normalized = text;
    
    // Replace UTF-8 curly quotes with straight quotes
    normalized.replace("\xE2\x80\x99", "'");  // U+2019 RIGHT SINGLE QUOTATION MARK
    normalized.replace("\xE2\x80\x98", "'");  // U+2018 LEFT SINGLE QUOTATION MARK
    normalized.replace("\xE2\x80\x9C", "\""); // U+201C LEFT DOUBLE QUOTATION MARK
    normalized.replace("\xE2\x80\x9D", "\""); // U+201D RIGHT DOUBLE QUOTATION MARK
    normalized.replace("\xE2\x80\x93", "-");  // U+2013 EN DASH
    normalized.replace("\xE2\x80\x94", "-");  // U+2014 EM DASH
    normalized.replace("\xE2\x80\xA6", "..."); // U+2026 HORIZONTAL ELLIPSIS
    
    return normalized;
}

// Helper function to update display immediately
static void updateDisplayNow() {
    display.update();
    uiManager.update();
}

// Helper to request LED state changes from any task, applied in main loop
static void ledRequest(LEDState state) {
    if (!g_led_queue) return;
    uint8_t v = static_cast<uint8_t>(state);
    xQueueSend(g_led_queue, &v, 0);
}

static void ledDrainRequests() {
    if (!g_led_queue) return;
    uint8_t v;
    while (xQueueReceive(g_led_queue, &v, 0) == pdTRUE) {
        ledManager.setState(static_cast<LEDState>(v));
    }
}

// Callback handlers for manager interactions
static void onAudioChunkReady(const uint8_t* data, size_t size) {
    if (!g_streaming_active || !g_audio_chunk_queue) return;
    
    // Allocate chunk memory
    AudioChunk chunk;
    chunk.data = (uint8_t*)malloc(size);
    if (!chunk.data) {
        Serial.println("[Stream] Failed to allocate chunk memory");
        return;
    }
    
    memcpy(chunk.data, data, size);
    chunk.size = size;
    
    // Send to queue (non-blocking)
    if (xQueueSend(g_audio_chunk_queue, &chunk, 0) != pdTRUE) {
        Serial.println("[Stream] Chunk queue full, dropping chunk");
        free(chunk.data);
    } else {
        Serial.printf("[Stream] Chunk queued: %zu bytes (queue has space)\n", size);
    }
}

static void onAudioStateChanged(RecordingState state) {
    switch (state) {
        case RecordingState::Idle:
            // Don't auto-return to Ready - let main loop handle it
            ledRequest(LEDState::Off);
            break;
        case RecordingState::Recording:
            uiManager.postStateChange(UIState::Recording);
            ledRequest(LEDState::Recording);
            break;
        case RecordingState::Saving:
            uiManager.postStateChange(UIState::Processing);
            ledRequest(LEDState::Processing);
            break;
        case RecordingState::Saved:
            // Audio is ready - will be processed by main loop
            g_bench_start_ms = millis();
            break;
        case RecordingState::Error:
            uiManager.postStateChange(UIState::Error);
            ledRequest(LEDState::Error);
            break;
    }
}

// WiFi helper functions
static void wifiEnsureConnected() {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Country/protocol optimization
    wifi_country_t country = {"ES", 1, 13, WIFI_COUNTRY_POLICY_AUTO};
    esp_wifi_set_country(&country);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    WiFi.setHostname("BasicGPT");

    if (WiFi.getMode() != WIFI_MODE_NULL) {
        WiFi.disconnect(true, true);
        delay(100);
    }

    // Use WiFiManager for connection with display updates
    const uint8_t maxAttempts = 2;
    for (uint8_t attempt = 1; attempt <= maxAttempts; ++attempt) {
        Serial.printf("[WiFi] Connecting (attempt %u/%u)...\n", attempt, maxAttempts);
        
        if (wifiManager.connectToWiFi()) {
            Serial.printf("[WiFi] Connected: IP=%s RSSI=%d\n", 
                         WiFi.localIP().toString().c_str(), WiFi.RSSI());
            return;
        }
        
        Serial.println("[WiFi] Connection timeout, resetting...");
        
        esp_wifi_disconnect();
        esp_wifi_stop();
        delay(250);
        esp_wifi_start();
        delay(500U << (attempt - 1));
    }
    
    Serial.println("[WiFi] Connection failed after all attempts");
}

// OpenAI query task - NOW WITH STREAMING SUPPORT
static void openaiTask(void *arg) {
    Serial.println("[OpenAI] Task started - STREAMING MODE");
    ledRequest(LEDState::Processing);
    
    // Validation checks
    if (OPENAI_API_KEY_STR.length() == 0) {
        Serial.println("[OpenAI] Error: No API key found");
        uiManager.postStatus("No API key");
        audioManager.releasePCMBuffer();
        audioManager.resetToIdle();
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        g_streaming_active = false;
        g_openai_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    
    // NOTE: Audio chunks are being streamed in real-time via queue
    // We'll still use the final buffer for the complete request
    
    // Get PCM data SAFELY
    uint8_t* pcmBuffer = audioManager.getPCMBuffer();
    size_t pcmSize = audioManager.getPCMSize();
    
    Serial.printf("[OpenAI] Got PCM buffer: %p, size: %zu\n", pcmBuffer, pcmSize);
    Serial.printf("[OpenAI] Chunks were streamed during recording for pre-processing\n");
    
    if (!pcmBuffer || pcmSize == 0) {
        Serial.println("[OpenAI] Error: No valid audio data");
        audioManager.resetToIdle();
        uiManager.postStatus("No audio");
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        g_streaming_active = false;
        g_openai_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    
    // Stop streaming flag
    g_streaming_active = false;
    
    // Drain any remaining chunks from queue
    AudioChunk chunk;
    int drainedChunks = 0;
    while (xQueueReceive(g_audio_chunk_queue, &chunk, 0) == pdTRUE) {
        free(chunk.data);
        drainedChunks++;
    }
    if (drainedChunks > 0) {
        Serial.printf("[OpenAI] Drained %d remaining chunks from queue\n", drainedChunks);
    }
    
    // Configure OpenAI client
    BasicGPTClient::Config cfg;
    cfg.host = OPENAI_HOST;
    cfg.port = OPENAI_PORT;
    cfg.apiKey = OPENAI_API_KEY_STR.c_str();
    cfg.model = OPENAI_MODEL;
    cfg.systemPrompt = SYSTEM_PROMPT;
    cfg.httpTimeoutMs = HTTP_TIMEOUT_MS;

    wifiEnsureConnected();
    
    String response;
    BasicGPTClient client(cfg);
    
    Serial.println("[OpenAI] Sending audio to OpenAI...");
    Serial.printf("[OpenAI] Conversation history: %d messages\n", g_conversation_history.size());
    
    // Use conversation history for context
    bool success = client.askAudioFromPCMWithHistory(pcmBuffer, pcmSize, 32000, 16, 1, 
                                                     g_conversation_history, response);
    Serial.printf("[OpenAI] Request completed, success: %s\n", success ? "true" : "false");
    
    // Release audio buffer AFTER processing
    Serial.println("[OpenAI] Releasing PCM buffer...");
    audioManager.releasePCMBuffer();
    audioManager.resetToIdle();
    Serial.println("[OpenAI] PCM buffer released and state reset");

    uint32_t benchElapsed = (g_bench_start_ms > 0) ? (millis() - g_bench_start_ms) : 0;
    
    if (success && response.length() > 0) {
        // Parse structured response
        ParsedResponse parsed = parseGPTResponse(response);
        
        if (!parsed.valid || parsed.displayText.length() == 0) {
            Serial.println("[OpenAI] Error: Failed to parse response");
            uiManager.postStatus("Parse error");
            uiManager.postStateChange(UIState::Error);
            ledRequest(LEDState::Error);
            delay(2000);
            uiManager.postStateChange(UIState::Ready);
            g_bench_start_ms = 0;
            g_openai_task = nullptr;
            vTaskDelete(nullptr);
            return;
        }
        
        // Normalize display text for font compatibility
        String displayText = normalizeTextForDisplay(parsed.displayText);
        
        if (benchElapsed > 0) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Response (%.1fs)", benchElapsed / 1000.0f);
            Serial.printf("[Benchmark] Total time: %u ms\n", benchElapsed);
            uiManager.postStatus(msg);
        } else {
            uiManager.postStatus("");
        }
        
        // Add to conversation history (store full response for context)
        ChatMessage userMsg;
        userMsg.role = "user";
        userMsg.content = "[Audio message]";
        g_conversation_history.push_back(userMsg);
        
        ChatMessage assistantMsg;
        assistantMsg.role = "assistant";
        assistantMsg.content = response; // Store full structured response
        g_conversation_history.push_back(assistantMsg);
        
        // Limit history
        while (g_conversation_history.size() > MAX_CONVERSATION_HISTORY) {
            g_conversation_history.erase(g_conversation_history.begin());
        }
        
        Serial.printf("[Memory] Conversation history: %d messages\n", g_conversation_history.size());
        
        // Show ONLY display text on screen
        Serial.printf("[CuteAssistant] Displaying: %s\n", displayText.c_str());
        Serial.printf("[CuteAssistant] Physical Action: %s\n", parsed.action.c_str());
        
        uiManager.postStateChange(UIState::ShowingResponse);
        uiManager.postResponse(displayText.c_str());
        
        ledRequest(LEDState::Off);
        
        // Parse and execute GPIO command sequence if present
        if (parsed.action.length() > 0 && parsed.action != "none") {
            Serial.println("=== PHYSICAL ACTION ===");
            Serial.printf("ACTION: %s\n", parsed.action.c_str());
            Serial.println("=======================");
            
            // Wait 1 second to let text start appearing on screen first
            delay(1000);
            
            // Check if action contains GPIO commands
            if (parsed.action.indexOf("BEGIN") != -1 && parsed.action.indexOf("END") != -1) {
                Serial.println("[GPIO] Starting command sequence execution...");
                parseActionSequence(parsed.action, g_active_sequence);
            } else {
                Serial.println("[ACTION] Human-like action (no GPIO control)");
            }
        }
        
        // UIManager will automatically return to Ready after 2s (handled in UIManager update loop)
        // No blocking delay needed - user can interact immediately when ready
        
    } else {
        char msg[80];
        if (benchElapsed > 0) {
            snprintf(msg, sizeof(msg), "Error (%.1fs)", benchElapsed / 1000.0f);
        } else {
            snprintf(msg, sizeof(msg), "Assistant Error");
        }
        Serial.printf("[OpenAI] Query failed. Time=%u ms\n", benchElapsed);
        uiManager.postStatus(msg);
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        
        delay(2000);
        uiManager.postStateChange(UIState::Ready);
    }
    
    g_bench_start_ms = 0;
    g_openai_task = nullptr;
    vTaskDelete(nullptr);
}

// Battery monitoring for M5Unified devices (CoreS3-Lite, Cardputer)
#if defined(CORES3_LITE_TARGET) || defined(CARDPUTER_TARGET)
static void updateBatteryStatus() {
    uint32_t now = millis();
    if (now - g_last_battery_check_ms < BATTERY_CHECK_INTERVAL_MS) {
        return; // Too soon to check again
    }
    g_last_battery_check_ms = now;

    // Get battery level via M5Unified API
    g_battery_percent = M5.Power.getBatteryLevel();

    Serial.printf("[Power] Battery: %u%%\n", (unsigned)g_battery_percent);

    // Check critical threshold (<5% per clarification FR-011a)
    bool is_charging = M5.Power.isCharging();
    bool was_blocked = g_recording_blocked_low_battery;

    if (g_battery_percent < 5 && !is_charging) {
        g_recording_blocked_low_battery = true;
        if (!was_blocked) {
            Serial.println("[Power] Battery critical (<5%), recording blocked");
            uiManager.postStatus("Battery <5%");
        }
    } else {
        if (was_blocked && is_charging) {
            Serial.println("[Power] Charging detected, recording enabled");
        }
        g_recording_blocked_low_battery = false;
    }

    // Update UI with battery percentage (T032 implementation)
    // UIManager will display this value in top-right corner
    uiManager.setBatteryPercent(g_battery_percent);
}
#endif

void setup() {
    Serial.begin(115200);
    Serial.println("CuteAssistant starting...");

#if !defined(CARDPUTER_TARGET) && !defined(CORES3_LITE_TARGET)
    // Initialize I2C bus FIRST (shared by display, touch, PMIC) - Kode Dot only
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);
    delay(100);

    // Initialize PMIC early to enable 5V bus for servos/peripherals
    initPMIC();
#endif

    // Initialize display and UI so user sees something immediately
    if (!display.init()) {
        Serial.println("Error: Display initialization failed");
        while (1) delay(1000);
    }

    // Initialize UI manager immediately after display
    if (!uiManager.init()) {
        Serial.println("Error: UI manager initialization failed");
        while (1) delay(1000);
    }
    
    // Show initial status immediately
    uiManager.postStateChange(UIState::Connecting);
    
    // Force display update so user sees the message
    updateDisplayNow();
    delay(100); // Brief pause to ensure rendering

    // Initialize LED manager
    LEDConfig ledConfig;
    ledConfig.pin = NEO_PIXEL_PIN;
    ledConfig.count = NEO_PIXEL_COUNT;
    ledConfig.brightness = 51; // ~20%
    
    if (!ledManager.init(ledConfig)) {
        Serial.println("Warning: LED manager initialization failed - continuing without LEDs");
        uiManager.postStatus("Starting...");
    } else {
        uiManager.postStatus("Starting...");
        ledManager.setBrightness(128);      // ~50% brightness
        ledManager.setState(LEDState::Processing); // Blue while system boots/connects
    }

    // Initialize audio manager
    updateDisplayNow();
    delay(50);
    
    AudioConfig audioConfig;
    audioConfig.sampleRate = 32000;
    audioConfig.bitsPerSample = 16;
    audioConfig.numChannels = 1;
    audioConfig.maxRecordingMs = 30000; // 30 seconds max, but user can release earlier
    audioConfig.sckPin = MIC_I2S_SCK;
    audioConfig.wsPin = MIC_I2S_WS;
    audioConfig.dinPin = MIC_I2S_DIN;
    
    audioManager.setStateCallback(onAudioStateChanged);
    audioManager.setChunkCallback(onAudioChunkReady);
    if (!audioManager.init(audioConfig)) {
        Serial.println("Error: Audio manager initialization failed");
        uiManager.postStatus("Audio initialization failed");
        uiManager.postStateChange(UIState::Error);
        while (1) delay(1000);
    }

    // Create LED request queue
    g_led_queue = xQueueCreate(8, sizeof(uint8_t));
    
    // Create audio chunk streaming queue
    g_audio_chunk_queue = xQueueCreate(MAX_AUDIO_CHUNKS, sizeof(AudioChunk));
    if (!g_audio_chunk_queue) {
        Serial.println("[Setup] Error: Failed to create audio chunk queue");
    } else {
        Serial.println("[Setup] Audio streaming queue created successfully");
    }

    // Initialize primary input control
#if defined(CARDPUTER_TARGET) || defined(CORES3_LITE_TARGET)
    Serial.println("[Setup] Using M5 buttons for input control");
#else
    pinMode(BUTTON_TOP, INPUT_PULLUP);
    Serial.println("[Setup] Button TOP configured on GPIO 0");
#endif

    // NOW initialize WiFi and API credentials
    updateDisplayNow();
    delay(50);
    
    // Set up progress callback for WiFi connection
    wifiManager.setProgressCallback([](const char* message) {
        // Keep "Connecting..." state
    });
    
    bool wifiOk = wifiManager.loadCredentialsFromSD();
    bool apiOk = wifiManager.loadApiKeysFromSD();
    
    if (!wifiOk) {
        Serial.println("[WiFi] Could not load credentials from SD");
    }
    if (!apiOk) {
        Serial.println("[API] Could not load OpenAI key from SD (/apis.txt)");
    }
    
    // Connect to WiFi
    updateDisplayNow();
    delay(100);
    
    wifiEnsureConnected();
    
    if (WiFi.status() == WL_CONNECTED) {
        uiManager.postStateChange(UIState::Ready);
        Serial.println("[Setup] CuteAssistant ready!");
    } else {
        uiManager.postStatus("WiFi failed");
        uiManager.postStateChange(UIState::Error);
    }
    
    if (OPENAI_API_KEY_STR.length() == 0) {
        Serial.println("[API] Warning: No API key found, OpenAI queries will fail");
        uiManager.postStatus("No API key");
    }
}

void loop() {
    // Update all managers
    display.update();
    uiManager.update();
    audioManager.service();
    ledDrainRequests();

    // Update GPIO command sequence state machine
    updateCommandSequence();

#if defined(CORES3_LITE_TARGET) || defined(CARDPUTER_TARGET)
    // Update battery status every 30 seconds (T031)
    updateBatteryStatus();
#endif
    
    // Touch/Button handling for recording: press to start, release to stop
    static bool wasTouching = false;
    static bool wasButtonPressed = false;
    bool isTouching = false;
#if defined(CARDPUTER_TARGET) || defined(CORES3_LITE_TARGET)
    bool isButtonPressed = M5.BtnA.isPressed();
#else
    bool isButtonPressed = false;
#endif
    
    // Check current touch state
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    if (indev) {
        lv_indev_state_t state = lv_indev_get_state(indev);
        isTouching = (state == LV_INDEV_STATE_PRESSED);
    }

#if !defined(CARDPUTER_TARGET) && !defined(CORES3_LITE_TARGET)
    // Check top button state (active LOW with pull-up)
    isButtonPressed = (digitalRead(BUTTON_TOP) == LOW);
#endif
    
    // Combined input: either touch or button
    bool isInputActive = isTouching || isButtonPressed;
    bool wasInputActive = wasTouching || wasButtonPressed;
    
    // Detect input press edge (touch or button) - start recording
    if (isInputActive && !wasInputActive &&
        audioManager.getState() == RecordingState::Idle &&
        g_openai_task == nullptr &&
        millis() - g_last_touch_ms > TOUCH_DEBOUNCE_MS) {

        g_last_touch_ms = millis();

#if defined(CORES3_LITE_TARGET) || defined(CARDPUTER_TARGET)
        // Check battery level before allowing recording (T034)
        if (g_recording_blocked_low_battery) {
            Serial.println("[Power] Recording blocked: Battery critical (<5%)");
            uiManager.postStatus("Charge battery");
            uiManager.postStateChange(UIState::Error);
            goto skip_recording; // Skip to end of this block
        }
#endif

        // Clear chunk queue before starting
        AudioChunk chunk;
        while (xQueueReceive(g_audio_chunk_queue, &chunk, 0) == pdTRUE) {
            free(chunk.data);
        }

        // Enable streaming
        g_streaming_active = true;

        // Start recording
        uiManager.setResponse("");
        if (audioManager.startRecording()) {
            uiManager.postStatus("Listening...");
            if (isButtonPressed) {
#if defined(CARDPUTER_TARGET) || defined(CORES3_LITE_TARGET)
                Serial.println("[Input] Recording started by M5 button - STREAMING ENABLED");
#else
                Serial.println("[Input] Recording started by TOP button - STREAMING ENABLED");
#endif
            } else {
                Serial.println("[Input] Recording started by touch - STREAMING ENABLED");
            }
        } else {
            uiManager.postStatus("Record failed");
            uiManager.postStateChange(UIState::Error);
            g_streaming_active = false;
        }

#if defined(CORES3_LITE_TARGET) || defined(CARDPUTER_TARGET)
skip_recording: // Label for battery check goto
        (void)0; // No-op statement for label
#endif
    }

    // Detect input release edge (touch or button) - stop recording
    if (!isInputActive && wasInputActive && 
        audioManager.getState() == RecordingState::Recording) {
        audioManager.stopRecording();
        uiManager.postStatus("Processing...");
        Serial.println("[Input] Recording stopped - streaming will continue until task starts");
    }
    
    wasTouching = isTouching;
    wasButtonPressed = isButtonPressed;

    // Handle audio ready for OpenAI processing (with safety delay)
    static uint32_t audioReadyTime = 0;
    if (audioManager.getState() == RecordingState::Saved && 
        g_openai_task == nullptr && 
        audioManager.getPCMBuffer() != nullptr && 
        audioManager.getPCMSize() > 0) {
        
        if (audioReadyTime == 0) {
            audioReadyTime = millis();
        } else if (millis() - audioReadyTime > 100) { // 100ms safety delay
            // Start OpenAI query task and immediately update UI
            Serial.printf("[Main] Creating OpenAI task for %zu bytes of audio\n", audioManager.getPCMSize());
            uiManager.postStateChange(UIState::Processing);
            
            BaseType_t result = xTaskCreatePinnedToCore(
                openaiTask, 
                "openai_query", 
                12288, 
                nullptr, 
                1, 
                &g_openai_task, 
                1
            );
            
            if (result != pdPASS) {
                Serial.println("[Main] Failed to create OpenAI task");
                audioManager.resetToIdle();
                uiManager.postStatus("Failed to create OpenAI task");
                uiManager.postStateChange(UIState::Error);
                ledManager.setState(LEDState::Error);
            }
            audioReadyTime = 0;
        }
    } else if (audioManager.getState() != RecordingState::Saved) {
        audioReadyTime = 0;
    }

    // WiFi connectivity watchdog
    if (millis() - g_last_wifi_check_ms > WIFI_CHECK_INTERVAL_MS) {
        g_last_wifi_check_ms = millis();
        if (WiFi.status() != WL_CONNECTED) {
            uiManager.postStatus("Reconnecting WiFi...");
            wifiEnsureConnected();
        }
    }

    delay(GUI_LOOP_DELAY_MS);
}