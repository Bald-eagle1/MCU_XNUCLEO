/*
 * button.h
 *
 *  Module: Button driver with 3-sample debouncing, edge detection, and 1s long-press
 *  Hardware (XNUCLEO-F103RB + Proteus Schematic):
 *      Button 0 (BU1_A): PA8  (Arduino D7)  - External 10k pull-up, Active-LOW
 *      Button 1 (BU1_B): PA9  (Arduino D8)  - External 10k pull-up, Active-LOW
 *      Button 2 (BU2_A): PC7  (Arduino D9)  - External 10k pull-up, Active-LOW
 *      Button 3 (BU2_B): PB6  (Arduino D10) - External 10k pull-up, Active-LOW
 *  Course: CO3009 - Microprocessors & Microcontrollers
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include "main.h"
#include <stdint.h>

/* ---- Configuration ------------------------------------------------------ */

#define NO_OF_BUTTONS           4

/* Button indices matching schematic labels */
#define BTN_1                   0   /* BU1_A (PA8, D7)  */
#define BTN_2                   1   /* BU1_B (PA9, D8)  */
#define BTN_3                   2   /* BU2_A (PC7, D9)  */
#define BTN_4                   3   /* BU2_B (PB6, D10) */

/* Functional aliases for FSM layers */
#define BTN_MODE                BTN_1
#define BTN_SELECT              BTN_2
#define BTN_INC                 BTN_3
#define BTN_CONFIRM             BTN_4

/* Timing parameters (based on 10 ms scheduler tick) */
#define BUTTON_READ_PERIOD_MS   10
#define LONG_PRESS_DURATION_MS  1000

/* ---- Public API --------------------------------------------------------- */

/**
 * @brief  Initialise all button debounce buffers, flags, and counters.
 */
void button_init(void);

/**
 * @brief  Sample all buttons, perform 3-stage debouncing, and update event flags.
 *         Register as a periodic task running every 10 ms in the scheduler.
 */
void button_read(void);

/**
 * @brief  Check whether a single press (falling edge) occurred on a button.
 * @param  index  Button index (0 to NO_OF_BUTTONS - 1).
 * @return 1 if pressed since last check (flag auto-cleared), 0 otherwise.
 */
uint8_t isButtonPressed(uint8_t index);

/**
 * @brief  Check whether a button has been held down continuously for >= 1 s.
 * @param  index  Button index (0 to NO_OF_BUTTONS - 1).
 * @return 1 if 1s long-press occurred (flag auto-cleared), 0 otherwise.
 */
uint8_t isButtonPressed1s(uint8_t index);

/**
 * @brief  Query the current stable (debounced) level of a button.
 * @param  index  Button index (0 to NO_OF_BUTTONS - 1).
 * @return 1 while physically held down, 0 while released.
 */
uint8_t isButtonHeld(uint8_t index);

/**
 * @brief  Clear all pending press and long-press flags (useful on FSM mode switch).
 */
void button_clear_flags(void);

#endif /* INC_BUTTON_H_ */
