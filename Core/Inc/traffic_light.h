/*
 * traffic_light.h
 *
 *  Module: Two-Road Traffic Light Driver via Dual 74S139 Decoders + PNP Drivers
 *
 *  Hardware Mapping (XNUCLEO-F103RB + Proteus Schematic):
 *      Road 1 (U1:A - 74S139, E=GND):
 *          A = TL1_A (PB3), B = TL1_B (PB5)
 *          Y0 = NC (OFF), Y1 = RED_1 (Q3), Y2 = YELLOW_1 (Q2), Y3 = GREEN_1 (Q1)
 *
 *      Road 2 (U1:B - 74S139, E=GND):
 *          A = TL2_A (PB4), B = TL2_B (PB10)
 *          Y0 = NC (OFF), Y1 = RED_2 (Q6), Y2 = YELLOW_2 (Q5), Y3 = GREEN_2 (Q4)
 *
 *  74S139 Active-LOW Output Truth Table (drives PNP bases):
 *      B=0, A=0 -> Y0=LOW (NC)       -> All LEDs OFF
 *      B=0, A=1 -> Y1=LOW (RED_x)    -> RED ON
 *      B=1, A=0 -> Y2=LOW (YELLOW_x) -> AMBER/YELLOW ON
 *      B=1, A=1 -> Y3=LOW (GREEN_x)  -> GREEN ON
 *
 *  Course: CO3009 - Microprocessors & Microcontrollers
 */

#ifndef INC_TRAFFIC_LIGHT_H_
#define INC_TRAFFIC_LIGHT_H_

#include "main.h"
#include <stdint.h>

/* ---- Constants ---------------------------------------------------------- */

#define TRAFFIC_ROAD_1      1
#define TRAFFIC_ROAD_2      2

/* ---- Data Types --------------------------------------------------------- */

/* Enum values match 2-bit (B:A) 74S139 decoder select code directly */
typedef enum {
    LIGHT_OFF   = 0,    /* B=0, A=0 -> Y0 (NC)       -> OFF           */
    LIGHT_RED   = 1,    /* B=0, A=1 -> Y1 (RED_x)    -> RED           */
    LIGHT_AMBER = 2,    /* B=1, A=0 -> Y2 (YELLOW_x) -> AMBER/YELLOW  */
    LIGHT_GREEN = 3     /* B=1, A=1 -> Y3 (GREEN_x)  -> GREEN         */
} TrafficLightColor_t;

/* ---- Public API --------------------------------------------------------- */

/**
 * @brief  Initialise both traffic light modules to a safe OFF state.
 */
void traffic_init(void);

/**
 * @brief  Set the colour of a single traffic light road.
 * @param  road   TRAFFIC_ROAD_1 (1) or TRAFFIC_ROAD_2 (2).
 * @param  color  LIGHT_OFF, LIGHT_RED, LIGHT_AMBER, or LIGHT_GREEN.
 */
void traffic_set(uint8_t road, TrafficLightColor_t color);

/**
 * @brief  Set both roads simultaneously with a safety interlock.
 *         - Prevents both roads from being GREEN at the same time (forces RED/RED).
 *         - Orders GPIO updates so the non-GREEN road is updated first to prevent
 *           even a brief overlap of GREEN+GREEN during state transitions.
 * @param  road1_color  Target colour for Road 1.
 * @param  road2_color  Target colour for Road 2.
 */
void traffic_set_both(TrafficLightColor_t road1_color, TrafficLightColor_t road2_color);

/**
 * @brief  Get the current colour state of a road.
 * @param  road   TRAFFIC_ROAD_1 (1) or TRAFFIC_ROAD_2 (2).
 * @return Current TrafficLightColor_t (LIGHT_OFF if invalid road).
 */
TrafficLightColor_t traffic_get(uint8_t road);

/**
 * @brief  Toggle a road between the specified colour and LIGHT_OFF (for blinking).
 * @param  road   TRAFFIC_ROAD_1 (1) or TRAFFIC_ROAD_2 (2).
 * @param  color  Colour to blink (e.g. LIGHT_AMBER).
 */
void traffic_toggle(uint8_t road, TrafficLightColor_t color);

/**
 * @brief  Turn off all traffic lights on both roads.
 */
void traffic_off_all(void);

/**
 * @brief  Return a short 3-character string for LCD display ("OFF", "RED", "AMB", "GRN").
 */
const char *traffic_color_str(TrafficLightColor_t color);

#endif /* INC_TRAFFIC_LIGHT_H_ */
