#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <kodedot/pin_config.h>

#ifdef CARDPUTER_TARGET
#include <M5Unified.h>
#else
#include <Arduino_GFX_Library.h>
#include <bb_captouch.h>
#endif

/**
 * @brief High-level manager that initializes and wires up the display, LVGL, and input.
 */
class DisplayManager {
private:
#ifdef CARDPUTER_TARGET
    lv_display_t *display;
    lv_color_t *buf;
    lv_color_t *buf2;
    uint32_t last_tick_ms;
#else
    // Hardware interfaces
    Arduino_DataBus *bus;
    Arduino_CO5300 *gfx;
    BBCapTouch bbct;

    // LVGL display and draw buffers
    lv_display_t *display;
    lv_color_t *buf;
    lv_color_t *buf2;
    uint32_t last_tick_ms;
#endif

    // Static callbacks required by LVGL v9
    static void disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    static void touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data);

    // Singleton-like back-reference used by static callbacks
    static DisplayManager* instance;

public:
    DisplayManager();
    ~DisplayManager();

    /**
     * @brief Fully initialize display, LVGL, and input.
     * @return true on success, false otherwise
     */
    bool init();

#ifndef CARDPUTER_TARGET
    /**
     * @brief Get the underlying Arduino_GFX panel instance.
     */
    Arduino_CO5300* getGfx() { return gfx; }

    /**
     * @brief Get the touch controller instance.
     */
    BBCapTouch* getTouch() { return &bbct; }
#endif

    /**
     * @brief Pump LVGL timers and tick. Call frequently in loop().
     */
    void update();

    /**
     * @brief Set backlight brightness.
     * @param brightness Range 0-255
     */
    void setBrightness(uint8_t brightness);

    /**
     * @brief Get current brightness percentage.
     * @return Brightness percentage (0-100)
     */
    uint8_t getBrightnessPercentage();

    /**
     * @brief Read current touch coordinates.
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @return true if there is an active touch
     */
    bool getTouchCoordinates(int16_t &x, int16_t &y);
};

