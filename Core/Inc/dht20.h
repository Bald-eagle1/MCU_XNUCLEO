/*
 * dht20.h
 *
 *  Created on: Nov 17, 2026
 *      Author: no
 *
 *  DHT20 (AHT20) Temperature & Humidity Sensor Library
 *  Communication: I2C (7-bit address 0x38)
 *
 *  Enhanced for Cooperative Scheduler & Software Timers:
 *  - Non-blocking state-machine operation: does NOT block other tasks
 *  - Uses Software Timer for the ~80 ms measurement delay
 *  - Retains blocking helper for standalone testing
 */

#ifndef INC_DHT20_H_
#define INC_DHT20_H_

#include "stm32f1xx_hal.h"
#include "software_timer.h"
#include <stdint.h>

/* ---------- Constants ---------------------------------------------------- */

#define DHT20_I2C_ADDR              (0x38 << 1)   /* 7-bit addr left-shifted for HAL */
#define DHT20_TIMEOUT_MS            50            /* I2C timeout for quick non-blocking HAL calls */
#define DHT20_MEASURE_DELAY_TICKS   MS_TO_TICKS(80) /* ~80 ms conversion time (8 ticks @ 10ms) */

/* ---------- Status Codes ------------------------------------------------- */

typedef enum {
    DHT20_OK            = 0x00,
    DHT20_ERR_I2C       = 0x01,   /* I2C bus error                           */
    DHT20_ERR_BUSY      = 0x02,   /* Sensor still converting                 */
    DHT20_ERR_CRC       = 0x03,   /* CRC-8 mismatch                          */
    DHT20_ERR_NOT_INIT  = 0x04,   /* Status register says not calibrated     */
    DHT20_ERR_TIMEOUT   = 0x05,   /* Conversion did not complete             */
    DHT20_WAITING       = 0x06    /* Measurement in progress (non-blocking)  */
} DHT20_Status;

/* ---------- FSM States (Non-blocking operation) -------------------------- */

typedef enum {
    DHT20_STATE_IDLE = 0,         /* Ready to start a new measurement        */
    DHT20_STATE_TRIGGERED,        /* Trigger command sent, waiting timer     */
    DHT20_STATE_READY,            /* Fresh data available                    */
    DHT20_STATE_ERROR             /* Communication or CRC error              */
} DHT20_FSM_State;

/* ---------- Handle Structure --------------------------------------------- */

typedef struct {
    I2C_HandleTypeDef *hi2c;            /* HAL I2C handle pointer            */
    float              temperature;     /* Temperature in °C                 */
    float              humidity;        /* Relative humidity in %RH          */
    uint32_t           raw_humidity;    /* 20-bit raw humidity               */
    uint32_t           raw_temperature; /* 20-bit raw temperature            */
    DHT20_FSM_State    state;           /* Current FSM state                 */
    SoftTimer_TypeDef  timer;           /* Software timer for conversion wait*/
    uint8_t            data_ready;      /* 1 when fresh reading is available */
    DHT20_Status       last_status;     /* Status of the last operation      */
} DHT20_HandleTypeDef;

/* ---------- Public API --------------------------------------------------- */

/**
 * @brief  Initialise the DHT20 sensor and its internal software timer.
 *         Verifies device presence, checks calibration, and sends init registers.
 * @param  dht   Pointer to DHT20 handle.
 * @param  hi2c  Pointer to configured HAL I2C handle (e.g. &hi2c1).
 * @return DHT20_OK on success, or error code.
 */
DHT20_Status DHT20_Init(DHT20_HandleTypeDef *dht, I2C_HandleTypeDef *hi2c);

/* -------------------------------------------------- */
/* ---- Non-blocking / Cooperative Scheduler API ---- */
/* -------------------------------------------------- */

/**
 * @brief  Trigger a new measurement non-blockingly.
 *         Sends command 0xAC 0x33 0x00 and starts the software timer (~80 ms).
 *         Returns immediately without halting the CPU.
 * @param  dht   Pointer to DHT20 handle.
 * @return DHT20_OK if command transmitted, or error code.
 */
DHT20_Status DHT20_StartMeasurement(DHT20_HandleTypeDef *dht);

/**
 * @brief  Periodic process function for cooperative scheduler / timer.
 *         Call regularly (e.g. every 10 ms or 20 ms).
 *         When the ~80 ms conversion timer expires, it reads the 7-byte
 *         result, validates CRC, and computes temperature & humidity.
 * @param  dht   Pointer to DHT20 handle.
 * @return DHT20_OK if fresh data parsed, DHT20_WAITING while converting,
 *         or error code.
 */
DHT20_Status DHT20_Process(DHT20_HandleTypeDef *dht);

/**
 * @brief  Check whether a fresh reading is available.
 *         Automatically resets the flag upon return.
 * @param  dht   Pointer to DHT20 handle.
 * @return 1 if new data is ready, 0 otherwise.
 */
uint8_t DHT20_HasNewData(DHT20_HandleTypeDef *dht);

/* ------------------------------------------- */
/* ---- Standalone / Blocking Testing API ---- */
/* ------------------------------------------- */

/**
 * @brief  Synchronous blocking measurement (waits ~80 ms via HAL_Delay).
 *         Suitable for initial hardware testing before starting scheduler.
 * @param  dht   Pointer to DHT20 handle.
 * @return DHT20_OK on success, or error code.
 */
DHT20_Status DHT20_ReadData_Blocking(DHT20_HandleTypeDef *dht);

/**
 * @brief  Read the 1-byte raw status register.
 */
DHT20_Status DHT20_GetStatus(DHT20_HandleTypeDef *dht, uint8_t *status);

/**
 * @brief  Send software reset command (0xBA).
 */
DHT20_Status DHT20_SoftReset(DHT20_HandleTypeDef *dht);

#endif /* INC_DHT20_H_ */
