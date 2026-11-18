/*
 * dht20.c
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  DHT20 (AHT20) Temperature & Humidity Sensor Library Implementation
 *  Enhanced for Non-blocking Cooperative Scheduling with Software Timers
 */

#include "dht20.h"

/* ---------- Private Constants -------------------------------------------- */

/* Commands */
#define DHT20_CMD_TRIGGER       0xAC
#define DHT20_CMD_TRIGGER_P1    0x33
#define DHT20_CMD_TRIGGER_P2    0x00
#define DHT20_CMD_SOFTRESET     0xBA
#define DHT20_CMD_INIT_1        0x1B
#define DHT20_CMD_INIT_2        0x1C
#define DHT20_CMD_INIT_3        0x1E

/* Status-byte masks */
#define DHT20_STATUS_BUSY       0x80  /* Bit 7: 1 = measuring/busy */
#define DHT20_STATUS_CAL        0x08  /* Bit 3: 1 = calibrated    */

#define DHT20_DATA_LEN          7     /* 1 status + 5 data + 1 CRC */

/* ---------- Private Helpers ---------------------------------------------- */

/**
 * @brief  CRC-8 calculation (x^8 + x^5 + x^4 + 1, init 0xFF).
 */
static uint8_t DHT20_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief  Send initialization/calibration registers if uncalibrated.
 */
static DHT20_Status DHT20_InitRegisters(DHT20_HandleTypeDef *dht)
{
    uint8_t cmd[3];

    /* Register 0x1B */
    cmd[0] = DHT20_CMD_INIT_1; cmd[1] = 0x00; cmd[2] = 0x00;
    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, cmd, 3, DHT20_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERR_I2C;

    /* Register 0x1C */
    cmd[0] = DHT20_CMD_INIT_2;
    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, cmd, 3, DHT20_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERR_I2C;

    /* Register 0x1E */
    cmd[0] = DHT20_CMD_INIT_3;
    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, cmd, 3, DHT20_TIMEOUT_MS) != HAL_OK)
        return DHT20_ERR_I2C;

    return DHT20_OK;
}

/* ---------- Public API --------------------------------------------------- */

DHT20_Status DHT20_Init(DHT20_HandleTypeDef *dht, I2C_HandleTypeDef *hi2c)
{
    if (dht == NULL || hi2c == NULL)
    {
        return DHT20_ERR_I2C;
    }

    dht->hi2c            = hi2c;
    dht->temperature     = 0.0f;
    dht->humidity        = 0.0f;
    dht->raw_humidity    = 0;
    dht->raw_temperature = 0;
    dht->state           = DHT20_STATE_IDLE;
    dht->data_ready      = 0;
    dht->last_status     = DHT20_OK;

    SoftTimer_Init(&dht->timer);

    /* Guard delay: datasheet specifies >= 100 ms after power-on */
    HAL_Delay(100);

    /* Read status byte */
    uint8_t status = 0;
    if (DHT20_GetStatus(dht, &status) != DHT20_OK)
    {
        dht->state = DHT20_STATE_ERROR;
        dht->last_status = DHT20_ERR_I2C;
        return DHT20_ERR_I2C;
    }

    /* Check calibration bit (Bit 3) */
    if (!(status & DHT20_STATUS_CAL))
    {
        if (DHT20_InitRegisters(dht) != DHT20_OK)
        {
            dht->state = DHT20_STATE_ERROR;
            dht->last_status = DHT20_ERR_I2C;
            return DHT20_ERR_I2C;
        }

        HAL_Delay(10);

        if (DHT20_GetStatus(dht, &status) != DHT20_OK || !(status & DHT20_STATUS_CAL))
        {
            dht->state = DHT20_STATE_ERROR;
            dht->last_status = DHT20_ERR_NOT_INIT;
            return DHT20_ERR_NOT_INIT;
        }
    }

    dht->state = DHT20_STATE_IDLE;
    dht->last_status = DHT20_OK;
    return DHT20_OK;
}

DHT20_Status DHT20_StartMeasurement(DHT20_HandleTypeDef *dht)
{
    if (dht == NULL || dht->hi2c == NULL)
    {
        return DHT20_ERR_I2C;
    }

    uint8_t cmd[3] = {DHT20_CMD_TRIGGER, DHT20_CMD_TRIGGER_P1, DHT20_CMD_TRIGGER_P2};
    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, cmd, 3, DHT20_TIMEOUT_MS) != HAL_OK)
    {
        dht->state = DHT20_STATE_ERROR;
        dht->last_status = DHT20_ERR_I2C;
        return DHT20_ERR_I2C;
    }

    /* Start software timer for non-blocking conversion wait (~80 ms) */
    SoftTimer_Start(&dht->timer, DHT20_MEASURE_DELAY_TICKS, 0);
    dht->state       = DHT20_STATE_TRIGGERED;
    dht->data_ready  = 0;
    dht->last_status = DHT20_WAITING;

    return DHT20_OK;
}

DHT20_Status DHT20_Process(DHT20_HandleTypeDef *dht)
{
    if (dht == NULL || dht->hi2c == NULL)
    {
        return DHT20_ERR_I2C;
    }

    /* Tick the internal software timer */
    SoftTimer_Update(&dht->timer);

    switch (dht->state)
    {
        case DHT20_STATE_IDLE:
            return DHT20_OK;

        case DHT20_STATE_TRIGGERED:
            if (SoftTimer_IsExpired(&dht->timer))
            {
                uint8_t buf[DHT20_DATA_LEN];

                if (HAL_I2C_Master_Receive(dht->hi2c, DHT20_I2C_ADDR, buf, DHT20_DATA_LEN, DHT20_TIMEOUT_MS) != HAL_OK)
                {
                    dht->state = DHT20_STATE_ERROR;
                    dht->last_status = DHT20_ERR_I2C;
                    return DHT20_ERR_I2C;
                }

                /* Check busy bit (Bit 7) */
                if (buf[0] & DHT20_STATUS_BUSY)
                {
                    /* Still busy: wait 1 more tick (10 ms) non-blockingly */
                    SoftTimer_Start(&dht->timer, 1, 0);
                    return DHT20_WAITING;
                }

                /* Validate CRC-8 across the first 6 bytes */
                if (DHT20_CRC8(buf, 6) != buf[6])
                {
                    dht->state = DHT20_STATE_ERROR;
                    dht->last_status = DHT20_ERR_CRC;
                    return DHT20_ERR_CRC;
                }

                /* Parse raw values */
                dht->raw_humidity    = ((uint32_t)buf[1] << 12)
                                     | ((uint32_t)buf[2] << 4)
                                     | ((uint32_t)buf[3] >> 4);

                dht->raw_temperature = (((uint32_t)buf[3] & 0x0F) << 16)
                                     | ((uint32_t)buf[4] << 8)
                                     | ((uint32_t)buf[5]);

                /* Convert to physical units */
                dht->humidity    = ((float)dht->raw_humidity    / 1048576.0f) * 100.0f;
                dht->temperature = ((float)dht->raw_temperature / 1048576.0f) * 200.0f - 50.0f;

                dht->data_ready  = 1;
                dht->state       = DHT20_STATE_READY;
                dht->last_status = DHT20_OK;
                return DHT20_OK;
            }
            return DHT20_WAITING;

        case DHT20_STATE_READY:
            return DHT20_OK;

        case DHT20_STATE_ERROR:
        default:
            return dht->last_status;
    }
}

uint8_t DHT20_HasNewData(DHT20_HandleTypeDef *dht)
{
    if (dht == NULL) return 0;
    if (dht->data_ready)
    {
        dht->data_ready = 0;
        return 1;
    }
    return 0;
}

DHT20_Status DHT20_ReadData_Blocking(DHT20_HandleTypeDef *dht)
{
    if (dht == NULL || dht->hi2c == NULL)
    {
        return DHT20_ERR_I2C;
    }

    uint8_t cmd[3] = {DHT20_CMD_TRIGGER, DHT20_CMD_TRIGGER_P1, DHT20_CMD_TRIGGER_P2};
    uint8_t buf[DHT20_DATA_LEN];

    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, cmd, 3, DHT20_TIMEOUT_MS) != HAL_OK)
    {
        return DHT20_ERR_I2C;
    }

    /* Wait 80 ms typical conversion time */
    HAL_Delay(80);

    for (uint8_t retry = 0; retry < 16; retry++)
    {
        if (HAL_I2C_Master_Receive(dht->hi2c, DHT20_I2C_ADDR, buf, DHT20_DATA_LEN, DHT20_TIMEOUT_MS) != HAL_OK)
        {
            return DHT20_ERR_I2C;
        }

        if (!(buf[0] & DHT20_STATUS_BUSY))
        {
            break;
        }

        if (retry == 15)
        {
            return DHT20_ERR_TIMEOUT;
        }

        HAL_Delay(10);
    }

    if (DHT20_CRC8(buf, 6) != buf[6])
    {
        return DHT20_ERR_CRC;
    }

    dht->raw_humidity    = ((uint32_t)buf[1] << 12)
                         | ((uint32_t)buf[2] << 4)
                         | ((uint32_t)buf[3] >> 4);

    dht->raw_temperature = (((uint32_t)buf[3] & 0x0F) << 16)
                         | ((uint32_t)buf[4] << 8)
                         | ((uint32_t)buf[5]);

    dht->humidity    = ((float)dht->raw_humidity    / 1048576.0f) * 100.0f;
    dht->temperature = ((float)dht->raw_temperature / 1048576.0f) * 200.0f - 50.0f;

    dht->data_ready  = 1;
    dht->state       = DHT20_STATE_READY;
    dht->last_status = DHT20_OK;
    return DHT20_OK;
}

DHT20_Status DHT20_GetStatus(DHT20_HandleTypeDef *dht, uint8_t *status)
{
    if (dht == NULL || dht->hi2c == NULL || status == NULL)
    {
        return DHT20_ERR_I2C;
    }
    if (HAL_I2C_Master_Receive(dht->hi2c, DHT20_I2C_ADDR, status, 1, DHT20_TIMEOUT_MS) != HAL_OK)
    {
        return DHT20_ERR_I2C;
    }
    return DHT20_OK;
}

DHT20_Status DHT20_SoftReset(DHT20_HandleTypeDef *dht)
{
    if (dht == NULL || dht->hi2c == NULL)
    {
        return DHT20_ERR_I2C;
    }
    uint8_t cmd = DHT20_CMD_SOFTRESET;
    if (HAL_I2C_Master_Transmit(dht->hi2c, DHT20_I2C_ADDR, &cmd, 1, DHT20_TIMEOUT_MS) != HAL_OK)
    {
        return DHT20_ERR_I2C;
    }
    HAL_Delay(20);
    return DHT20_OK;
}
