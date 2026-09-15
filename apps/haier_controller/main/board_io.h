#ifndef BOARD_IO_H
#define BOARD_IO_H

#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "board_lvgl.h"


#define I2C_MASTER_SCL_IO           BOARD_TOUCH_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           BOARD_TOUCH_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0       /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BOARD_IO_LCD_H_RES              BOARD_LCD_WIDTH
#define BOARD_IO_LCD_V_RES              BOARD_LCD_HEIGHT

#if ESP_PANEL_USE_1024_600_LCD
    #define BOARD_IO_LCD_PIXEL_CLOCK_HZ      (21 * 1000 * 1000)
#else
    #define BOARD_IO_LCD_PIXEL_CLOCK_HZ      (16 * 1000 * 1000)
#endif

#define EXAMPLE_LCD_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_BIT_PER_PIXEL       (16)
#define EXAMPLE_RGB_DATA_WIDTH          (16)
#define BOARD_IO_RGB_BOUNCE_BUFFER_SIZE  (BOARD_IO_LCD_H_RES * CONFIG_EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT)
#define EXAMPLE_LCD_IO_RGB_DISP         (-1)             // -1 if not used
#define EXAMPLE_LCD_IO_RGB_VSYNC        (GPIO_NUM_3)
#define EXAMPLE_LCD_IO_RGB_HSYNC        (GPIO_NUM_46)
#define EXAMPLE_LCD_IO_RGB_DE           (GPIO_NUM_5)
#define EXAMPLE_LCD_IO_RGB_PCLK         (GPIO_NUM_7)
#define EXAMPLE_LCD_IO_RGB_DATA0        (GPIO_NUM_14)
#define EXAMPLE_LCD_IO_RGB_DATA1        (GPIO_NUM_38)
#define EXAMPLE_LCD_IO_RGB_DATA2        (GPIO_NUM_18)
#define EXAMPLE_LCD_IO_RGB_DATA3        (GPIO_NUM_17)
#define EXAMPLE_LCD_IO_RGB_DATA4        (GPIO_NUM_10)
#define EXAMPLE_LCD_IO_RGB_DATA5        (GPIO_NUM_39)
#define EXAMPLE_LCD_IO_RGB_DATA6        (GPIO_NUM_0)
#define EXAMPLE_LCD_IO_RGB_DATA7        (GPIO_NUM_45)
#define EXAMPLE_LCD_IO_RGB_DATA8        (GPIO_NUM_48)
#define EXAMPLE_LCD_IO_RGB_DATA9        (GPIO_NUM_47)
#define EXAMPLE_LCD_IO_RGB_DATA10       (GPIO_NUM_21)
#define EXAMPLE_LCD_IO_RGB_DATA11       (GPIO_NUM_1)
#define EXAMPLE_LCD_IO_RGB_DATA12       (GPIO_NUM_2)
#define EXAMPLE_LCD_IO_RGB_DATA13       (GPIO_NUM_42)
#define EXAMPLE_LCD_IO_RGB_DATA14       (GPIO_NUM_41)
#define EXAMPLE_LCD_IO_RGB_DATA15       (GPIO_NUM_40)

#define EXAMPLE_LCD_IO_RST              (-1)             // -1 if not used
#define EXAMPLE_PIN_NUM_BK_LIGHT        (-1)    // -1 if not used
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL   (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL  !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

#define EXAMPLE_PIN_NUM_TOUCH_RST       (GPIO_NUM_NC)   // Reset is handled by the CH422G sequence
#define EXAMPLE_PIN_NUM_TOUCH_INT       (GPIO_NUM_NC)   // Poll GT911 status without an interrupt line

static const char *BOARD_IO_TAG = "board_io";

esp_err_t board_io_init(void);
i2c_master_bus_handle_t board_io_i2c_bus(void);
esp_err_t board_io_touch_reset(void);

esp_err_t board_io_backlight_on(void);
esp_err_t board_io_backlight_off(void);


#endif