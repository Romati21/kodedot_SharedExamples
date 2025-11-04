# Sensor Integration Implementation Notes

## Simplified Function Calling Approach

Instead of full OpenAI Function Calling API (which requires complex JSON parsing and multi-round requests), we'll use a **structured response format** similar to GPIO commands.

### Why Simplified Approach?
1. **Memory constraints**: ESP32 has limited RAM for complex JSON parsing
2. **Simpler implementation**: Single request-response cycle
3. **Faster response**: No multiple API calls needed
4. **Consistent with existing GPIO format**: Uses same `BEGIN;...;END` pattern

### Response Format

GPT will output sensor read commands in the response:

```
Response: Let me check the temperature for you!
Actions: BEGIN;READ_SENSOR(bme688,temperature);END
```

After executing the sensor read, we'll make a **second request** with the result:

```
Previous context: User asked about temperature
Sensor result: temperature=23.5°C
Please provide a natural response to the user.
```

GPT final response:
```
The current temperature is 23.5 degrees Celsius. It's quite comfortable!
```

### Command Format

**Sensor Read Commands**:
- `READ_SENSOR(bme688,temperature)` - Read BME688 temperature
- `READ_SENSOR(bme688,humidity)` - Read BME688 humidity
- `READ_SENSOR(bme688,pressure)` - Read BME688 pressure
- `READ_SENSOR(bme688,gas)` - Read BME688 gas resistance
- `READ_SENSOR(bme688,all)` - Read all BME688 values
- `READ_SENSOR(adxl345,x)` - Read X-axis acceleration
- `READ_SENSOR(adxl345,y)` - Read Y-axis acceleration
- `READ_SENSOR(adxl345,z)` - Read Z-axis acceleration
- `READ_SENSOR(adxl345,all)` - Read all axes
- `READ_SENSOR(hall,detect)` - Check magnetic field
- `READ_SENSOR(rfid,uid)` - Read RFID card UID

### System Prompt Update

Add to existing system prompt:

```
SENSOR READING FORMAT:
READ_SENSOR(device,parameter)

Available sensors:
- bme688: temperature, humidity, pressure, gas, all
- adxl345: x, y, z, all (acceleration in g)
- hall: detect (magnetic field)
- rfid: uid (card UID)

EXAMPLES:
User: "What's the temperature?"
Response: Let me check the temperature sensor!
Actions: BEGIN;READ_SENSOR(bme688,temperature);END

User: "Is there a magnet nearby?"
Response: I'll check with the magnetic sensor.
Actions: BEGIN;READ_SENSOR(hall,detect);END

User: "How tilted am I?"
Response: Let me read the accelerometer!
Actions: BEGIN;READ_SENSOR(adxl345,all);END
```

### Implementation Flow

```cpp
// 1. User speaks: "What's the temperature?"
String userInput = transcription;

// 2. Send to OpenAI
String gptResponse = "";
gptClient.askAudioFromPCM(..., gptResponse);

// 3. Parse response
//    Response: "Let me check the temperature sensor!"
//    Actions: "BEGIN;READ_SENSOR(bme688,temperature);END"

// 4. Execute sensor read
if (actions.contains("READ_SENSOR")) {
    SensorResult result = executeSensorRead(actions);

    // 5. Second request with result
    String contextPrompt = "User asked: '" + userInput + "'\n"
                          "Sensor reading: " + result.toString() + "\n"
                          "Provide a natural spoken response.";

    String finalResponse = "";
    gptClient.askText(contextPrompt, finalResponse);

    // 6. Display final response
    uiManager.postResponse(finalResponse);
}
```

### Advantages
- ✅ Works with existing BasicGPTClient (no major rewrite)
- ✅ Similar to GPIO command pattern
- ✅ Low memory footprint
- ✅ Single sensor library integration point
- ✅ Easy to add new sensors

### Disadvantages
- ❌ Two API calls per sensor query (costs 2x credits)
- ❌ Slower than native Function Calling
- ❌ Relies on GPT following format instructions

### Alternative: Direct Voice Response

For even simpler implementation, we can have GPT respond with sensor value placeholder:

```
User: "What's the temperature?"
GPT: "The temperature is [SENSOR:bme688:temperature] degrees."
```

Device replaces `[SENSOR:bme688:temperature]` with actual value before TTS.

**This is the most efficient approach!**

## Recommended Implementation: Placeholder Method

### Pros
- ✅ Only 1 API call
- ✅ Minimal changes to BasicGPTClient
- ✅ Fast response time
- ✅ Low cost

### System Prompt Addition
```
When user asks about sensor readings, respond with placeholder syntax:
[SENSOR:device:parameter]

Examples:
User: "What's the temperature?"
You: "The current temperature is [SENSOR:bme688:temperature] degrees Celsius."

User: "Am I tilted?"
You: "You're tilted at [SENSOR:adxl345:x] g on the X axis."

User: "Is there a card?"
You: "Let me check... [SENSOR:rfid:uid]. That's your card!"
```

### Processing
```cpp
String response = gptResponseText;

// Find and replace all [SENSOR:x:y] placeholders
while (response.indexOf("[SENSOR:") >= 0) {
    int start = response.indexOf("[SENSOR:");
    int end = response.indexOf("]", start);
    String placeholder = response.substring(start + 8, end); // Skip "[SENSOR:"

    // Parse device:parameter
    int colon = placeholder.indexOf(":");
    String device = placeholder.substring(0, colon);
    String param = placeholder.substring(colon + 1);

    // Read sensor
    String value = sensorManager.readSensor(device, param);

    // Replace placeholder
    response.replace("[SENSOR:" + placeholder + "]", value);
}

uiManager.postResponse(response);
```

## Decision: Use Placeholder Method

Implementing the placeholder method for sensor readings as it's:
- Most efficient (1 API call)
- Simplest to implement
- Lowest memory usage
- Compatible with existing code
