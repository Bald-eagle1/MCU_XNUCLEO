/*
 * lcd.c
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  LCD 1602 (HD44780 / LM016L) with PCF8574 I2C Backpack Driver Implementation
 */

#include "lcd.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ---- PCF8574 Pin Mapping Bitmasks --------------------------------------- */
#define LCD_PIN_RS          (1 << 0)    /* Bit 0: Register Select (0=Cmd, 1=Data) */
#define LCD_PIN_RW          (1 << 1)    /* Bit 1: Read/Write (0=Write, 1=Read)    */
#define LCD_PIN_EN          (1 << 2)    /* Bit 2: Enable Pulse                    */
#define LCD_PIN_BL          (1 << 3)    /* Bit 3: Backlight LED (1=ON, 0=OFF)     */

#define LCD_I2C_TIMEOUT     50          /* ms */

/* ---- Private State ------------------------------------------------------ */
static I2C_HandleTypeDef *lcd_hi2c = NULL;
static uint8_t            lcd_addr = LCD_DEFAULT_ADDR;
static uint8_t            lcd_bl_state = LCD_PIN_BL; /* Default Backlight ON */

/* ---- Private Helpers ---------------------------------------------------- */

/**
 * @brief  Transmit 1 byte to the PCF8574 expander over I2C.
 */
static void LCD_I2C_Write(uint8_t data)
{
    if (lcd_hi2c == NULL) return;
    uint8_t tx_data = data | lcd_bl_state;
    HAL_I2C_Master_Transmit(lcd_hi2c, lcd_addr, &tx_data, 1, LCD_I2C_TIMEOUT);
}

/**
 * @brief  Pulse the Enable pin to latch the 4-bit nibble into the LCD.
 */
static void LCD_PulseEnable(uint8_t data)
{
    LCD_I2C_Write(data | LCD_PIN_EN);   /* EN = 1 */
    /* Small delay for HD44780 enable pulse width (min 450 ns) */
    for (volatile int i = 0; i < 20; i++) { __NOP(); }
    LCD_I2C_Write(data & ~LCD_PIN_EN);  /* EN = 0 */
}

/**
 * @brief  Send 4 bits (upper nibble of data) with RS control.
 */
static void LCD_Write4Bits(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | (rs ? LCD_PIN_RS : 0);
    LCD_PulseEnable(data);
}

/**
 * @brief  Send full 8-bit byte split into high nibble then low nibble.
 */
static void LCD_WriteByte(uint8_t value, uint8_t rs)
{
    LCD_Write4Bits(value & 0xF0, rs);         /* High nibble */
    LCD_Write4Bits((value << 4) & 0xF0, rs);  /* Low nibble  */
}

/* ---- Public API --------------------------------------------------------- */

void LCD_SendCommand(uint8_t cmd)
{
    LCD_WriteByte(cmd, 0);
    if (cmd == 0x01 || cmd == 0x02)
    {
        HAL_Delay(2); /* Clear Display and Return Home require ~1.5 - 2 ms */
    }
}

void LCD_SendData(uint8_t data)
{
    LCD_WriteByte(data, 1);
}

HAL_StatusTypeDef LCD_Init(I2C_HandleTypeDef *hi2c, uint8_t addr)
{
    if (hi2c == NULL) return HAL_ERROR;

    lcd_hi2c = hi2c;
    lcd_addr = addr;

    /* Check if the device acknowledges at this I2C address */
    if (HAL_I2C_IsDeviceReady(lcd_hi2c, lcd_addr, 2, LCD_I2C_TIMEOUT) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /* 1. Wait > 50 ms after VDD rises */
    HAL_Delay(50);

    /* 2. HD44780 4-bit initialization sequence per datasheet */
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(5);

    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1);

    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1);

    /* 3. Switch interface to 4-bit mode */
    LCD_Write4Bits(0x20, 0);
    HAL_Delay(1);

    /* 4. Function Set: 4-bit mode, 2 lines, 5x8 font (0x28) */
    LCD_SendCommand(0x28);

    /* 5. Display Control: Display ON, Cursor OFF, Blink OFF (0x0C) */
    LCD_SendCommand(0x0C);

    /* 6. Clear display (0x01) */
    LCD_SendCommand(0x01);
    HAL_Delay(2);

    /* 7. Entry mode set: Increment cursor, no display shift (0x06) */
    LCD_SendCommand(0x06);

    return HAL_OK;
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t col, uint8_t row)
{
    if (col >= LCD_COLS) col = LCD_COLS - 1;
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;

    /* Row 0 starts at 0x80, Row 1 starts at 0xC0 */
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_SendCommand(address);
}

void LCD_SendString(const char *str)
{
    if (str == NULL) return;
    while (*str)
    {
        LCD_SendData((uint8_t)*str++);
    }
}

void LCD_SendChar(char ch)
{
    LCD_SendData((uint8_t)ch);
}

void LCD_Printf(const char *format, ...)
{
    char buf[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    LCD_SendString(buf);
}

void LCD_SetBacklight(bool on)
{
    lcd_bl_state = on ? LCD_PIN_BL : 0;
    LCD_I2C_Write(0x00); /* Update backpack immediately */
}

void LCD_CreateCustomChar(uint8_t location, const uint8_t charmap[8])
{
    location &= 0x07; /* Only 8 CGRAM locations (0-7) */
    LCD_SendCommand(0x40 | (location << 3));
    for (int i = 0; i < 8; i++)
    {
        LCD_SendData(charmap[i]);
    }
    LCD_SendCommand(0x80); /* Return to DDRAM */
}
