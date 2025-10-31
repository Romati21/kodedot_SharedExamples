# UI Manager Library

Library for managing LVGL user interface with thread-safe messaging and multi-device UI scaling.

## Features

- LVGL UI management with multi-device support
- Thread-safe message posting via FreeRTOS queues
- State-based visual feedback (eyes, status badges, typewriter animation)
- Custom brand styling (Kode Dot dark theme)
- Adaptive UI scaling for different screen sizes
- Battery status display (CoreS3-Lite/Cardputer)

## Supported Screen Resolutions

### Kode Dot (410x502)
- **Layout**: Vertical portrait orientation
- **Eyes**: 140x100px ovals with 30px spacing
- **Text Area**: Dynamic sizing (370x402px, centered)
- **Design**: Full-featured UI with ample space

### M5Stack CoreS3-Lite (320x240)
- **Layout**: Horizontal landscape orientation
- **Eyes**: 80x80px circles with 20px spacing (scaled down from Kode Dot)
- **Text Area**: Fixed 300x50px at bottom (y=180)
- **Text Truncation**: Max 100 characters with "..." overflow
- **Battery Display**: Top-right corner below GPT logo
- **Design Focus**: Compact, essential information only

### M5Stack Cardputer (240x135)
- **Layout**: Compact horizontal orientation
- **Eyes**: Further scaled for small screen
- **Text Area**: Minimal text display
- **Design Focus**: Keyboard-primary interaction

## UI Scaling Strategy

The library uses conditional compilation to adapt UI elements per device:

```cpp
#ifdef CORES3_LITE_TARGET
    // Small screen optimizations
    const lv_coord_t eyeWidth = 80;   // Scaled down eyes
    lv_obj_set_size(responseTextArea_, 300, 50);  // Fixed size text
    lv_obj_set_pos(responseTextArea_, 10, 180);   // Bottom placement
#else
    // Large screen (Kode Dot)
    const lv_coord_t eyeWidth = 140;  // Full-size eyes
    lv_obj_set_size(responseTextArea_, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_center(responseTextArea_);  // Centered placement
#endif
```

**Key Adaptations for 320x240**:
1. **Eye Size**: Reduced from 140x100px to 80x80px circular (43% smaller)
2. **Text Area**: Fixed position vs. dynamic centering
3. **Text Overflow**: Truncated to 100 chars max with ellipsis
4. **Line Spacing**: Maintained 1.2x font height for readability
5. **Battery Indicator**: Added for portable devices

## UI Design Standards

Per clarification in project specification, all UI elements follow these standards:

### Typography
- **Minimum Font Size**: ≥14px for readability on small screens
- **Primary Font**: Inter 30 (body text and responses)
- **System Font**: Montserrat 14 (battery, status indicators)
- **Line Spacing**: ≥1.2x font height (e.g., 30px font → 36px line spacing)

### Touch Targets
- **Minimum Tap Area**: ≥40x40px for reliable touch interaction
- **Spacing**: Adequate padding between interactive elements

### Text Handling
- **Long Text**: Wrap with `LV_LABEL_LONG_WRAP` mode
- **Small Screens**: Truncate with ellipsis when necessary (e.g., 100 char limit on CoreS3-Lite)
- **Typewriter Animation**: Configurable speed (default 40ms/char)

### Color Coding
- **Battery Status**:
  - Red: <20% (critical)
  - Yellow: 20-49% (warning)
  - White: ≥50% (normal)
- **Background**: Black (#000000)
- **Text**: Light beige (#FFFAF5)

## Usage

```cpp
#include <ui_manager/UIManager.h>

UIManager uiManager;

void setup() {
    uiManager.init();
    uiManager.postStatus("Hello World");
    uiManager.postStateChange(UIState::Ready);

    // Battery display (CoreS3-Lite/Cardputer only)
    uiManager.setBatteryPercent(85);

    // Adjust typewriter speed
    uiManager.setTypewriterSpeed(60);  // 60ms per character
}

void loop() {
    uiManager.update();  // Process message queue
}
```

## State Machine

The UIManager operates with the following states:

- **Connecting**: Initial WiFi connection phase (cyan eyes)
- **Ready**: Waiting for user input (white eyes, idle animation)
- **Recording**: User is speaking (red eyes)
- **Processing**: AI thinking animation (purple eyes moving)
- **ShowingResponse**: Text response displayed (eyes hidden, text visible)
- **Error**: Error state (red indication)

State transitions are thread-safe via `postStateChange()` method.
