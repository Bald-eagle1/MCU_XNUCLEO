/*
 * button.c
 *
 *  Module: Button driver with 3-sample debouncing, edge detection, and 1s long-press
 *  Hardware: 4 push buttons with external 10k pull-up resistors (Active-LOW)
 *  Course: CO3009 - Microprocessors & Microcontrollers
 */

#include "button.h"

/* ---- Private Constants -------------------------------------------------- */

#define BUTTON_IS_PRESSED       GPIO_PIN_RESET  /* Pulled to GND when pressed   */
#define BUTTON_IS_RELEASED      GPIO_PIN_SET    /* Pulled to +5V via 10k when idle */
#define TICKS_FOR_1S            (LONG_PRESS_DURATION_MS / BUTTON_READ_PERIOD_MS)

/* ---- Hardware Pin Mapping (from main.h / schematic) --------------------- */

static GPIO_TypeDef *const buttonPort[NO_OF_BUTTONS] = {
    BU1_A_GPIO_Port,
    BU1_B_GPIO_Port,
    BU2_A_GPIO_Port,
    BU2_B_GPIO_Port
};

static const uint16_t buttonPin[NO_OF_BUTTONS] = {
    BU1_A_Pin,
    BU1_B_Pin,
    BU2_A_Pin,
    BU2_B_Pin
};

/* ---- Private State ------------------------------------------------------ */

/* 3-stage debounce shift registers, initialised to RELEASED to avoid boot glitches */
static GPIO_PinState debounceReg0[NO_OF_BUTTONS] = {
    BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED
};
static GPIO_PinState debounceReg1[NO_OF_BUTTONS] = {
    BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED
};
static GPIO_PinState debounceReg2[NO_OF_BUTTONS] = {
    BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED
};
static volatile GPIO_PinState stableState[NO_OF_BUTTONS] = {
    BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED, BUTTON_IS_RELEASED
};

/* Event flags and long-press counters */
static volatile uint8_t  flagForButtonPressed[NO_OF_BUTTONS] = {0};
static volatile uint8_t  flagForButtonPress1s[NO_OF_BUTTONS] = {0};
static uint16_t counterForButtonPress1s[NO_OF_BUTTONS]       = {0};

/* ---- Public API --------------------------------------------------------- */

void button_init(void)
{
    for (uint8_t i = 0; i < NO_OF_BUTTONS; i++)
    {
        debounceReg0[i]            = BUTTON_IS_RELEASED;
        debounceReg1[i]            = BUTTON_IS_RELEASED;
        debounceReg2[i]            = BUTTON_IS_RELEASED;
        stableState[i]             = BUTTON_IS_RELEASED;
        flagForButtonPressed[i]    = 0;
        flagForButtonPress1s[i]    = 0;
        counterForButtonPress1s[i] = 0;
    }
}

void button_read(void)
{
    for (uint8_t i = 0; i < NO_OF_BUTTONS; i++)
    {
        /* Shift 3-sample debounce window (30 ms stability window @ 10 ms tick) */
        debounceReg2[i] = debounceReg1[i];
        debounceReg1[i] = debounceReg0[i];
        debounceReg0[i] = HAL_GPIO_ReadPin(buttonPort[i], buttonPin[i]);

        /* Accept new state only when 3 consecutive readings match */
        if ((debounceReg0[i] == debounceReg1[i]) && (debounceReg1[i] == debounceReg2[i]))
        {
            if (stableState[i] != debounceReg0[i])
            {
                stableState[i] = debounceReg0[i];

                if (stableState[i] == BUTTON_IS_PRESSED)
                {
                    /* Falling edge (Released -> Pressed): latch single-press event */
                    flagForButtonPressed[i]    = 1;
                    counterForButtonPress1s[i] = 0;
                }
                else
                {
                    /* Rising edge (Pressed -> Released): reset long-press state */
                    counterForButtonPress1s[i] = 0;
                    flagForButtonPress1s[i]    = 0;
                }
            }
            else if (stableState[i] == BUTTON_IS_PRESSED)
            {
                /* Button remains stably held: count up to 1 second */
                if (counterForButtonPress1s[i] < TICKS_FOR_1S)
                {
                    counterForButtonPress1s[i]++;
                    if (counterForButtonPress1s[i] == TICKS_FOR_1S)
                    {
                        flagForButtonPress1s[i] = 1;
                    }
                }
            }
        }
    }
}

uint8_t isButtonPressed(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0;
    }

    if (flagForButtonPressed[index] == 1)
    {
        flagForButtonPressed[index] = 0;
        return 1;
    }

    return 0;
}

uint8_t isButtonPressed1s(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0;
    }

    if (flagForButtonPress1s[index] == 1)
    {
        flagForButtonPress1s[index] = 0;
        return 1;
    }

    return 0;
}

uint8_t isButtonHeld(uint8_t index)
{
    if (index >= NO_OF_BUTTONS)
    {
        return 0;
    }

    return (stableState[index] == BUTTON_IS_PRESSED) ? 1 : 0;
}

void button_clear_flags(void)
{
    for (uint8_t i = 0; i < NO_OF_BUTTONS; i++)
    {
        flagForButtonPressed[i] = 0;
        flagForButtonPress1s[i] = 0;
    }
}
