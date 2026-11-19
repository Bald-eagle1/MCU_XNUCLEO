/*
 * lcd.h
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  LCD 1602 (HD44780 / LM016L) with PCF8574 I2C Backpack Driver
 *  Compatible with STM32 HAL (I2C1).
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ---- Common PCF8574 I2C Addresses (7-bit left-shifted by 1 for HAL) ---- */
#define LCD_ADDR_PROTEUS        (0x20 << 1)   /* Proteus default PCF8574 (A0=A1=A2=GND, 0x20) -> 0x40 */
#define LCD_ADDR_PCF8574        (0x27 << 1)   /* Standard PCF8574T  (0x27) -> 0x4E */
#define LCD_ADDR_PCF8574A       (0x3F << 1)   /* Standard PCF8574AT (0x3F) -> 0x7E */

/* Default address if not specified */
#define LCD_DEFAULT_ADDR        LCD_ADDR_PROTEUS

/* ---- LCD Dimensions ----------------------------------------------------- */
#define LCD_COLS                16
#define LCD_ROWS                2

/* ---- Public API --------------------------------------------------------- */

/**
 * @brief  Initialise the LCD 1602 module via PCF8574 in 4-bit mode.
 * @param  hi2c   Pointer to HAL I2C handle (e.g. &hi2c1).
 * @param  addr   8-bit I2C address (e.g. LCD_ADDR_PCF8574 or (0x27 << 1)).
 * @return HAL_OK on success, HAL_ERROR on communication failure.
 */
HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c, uint8_t addr);

/**
 * @brief  Clear the entire LCD screen and return cursor to (0, 0).
 */
void LCD_Clear(void);

/**
 * @brief  Move cursor to specific column and row.
 * @param  col  Column index (0 to LCD_COLS - 1, typically 0 to 15).
 * @param  row  Row index (0 for line 1, 1 for line 2).
 */
void LCD_SetCursor(uint8_t col, uint8_t row);

/**
 * @brief  Print a null-terminated string starting at the current cursor position.
 * @param  str  Pointer to string.
 */
void LCD_SendString(const char *str);

/**
 * @brief  Print a single character.
 * @param  ch   Character byte.
 */
void LCD_SendChar(char ch);

/**
 * @brief  Printf-style formatted string output to the LCD.
 * @param  format  Format string (e.g. "Temp: %.1f C", val).
 */
void LCD_Printf(const char *format, ...);

/**
 * @brief  Turn backlight ON or OFF.
 * @param  on   true (1) = ON, false (0) = OFF.
 */
void LCD_SetBacklight(bool on);

/**
 * @brief  Define a custom character in CGRAM (slot 0-7).
 * @param  location  Slot index (0 to 7).
 * @param  charmap   8-byte array defining character bitmap.
 */
void LCD_CreateCustomChar(uint8_t location, const uint8_t charmap[8]);

/**
 * @brief  Send raw command to LCD (internal / advanced use).
 */
void LCD_SendCommand(uint8_t cmd);

/**
 * @brief  Send raw data byte to LCD (internal / advanced use).
 */
void LCD_SendData(uint8_t data);

#endif /* INC_LCD_H_ */
