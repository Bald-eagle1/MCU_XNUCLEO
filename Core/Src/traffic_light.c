/*
 * traffic_light.c
 *
 *  Module: Two-Road Traffic Light Driver via Dual 74S139 Decoders + PNP Drivers
 *  Course: CO3009 - Microprocessors & Microcontrollers
 */

#include "traffic_light.h"

/* ---- Private State ------------------------------------------------------ */

static TrafficLightColor_t current_color[2] = { LIGHT_OFF, LIGHT_OFF };

/* ---- Private Helpers ---------------------------------------------------- */

static void write_decoder(GPIO_TypeDef *port_a, uint16_t pin_a,
                          GPIO_TypeDef *port_b, uint16_t pin_b,
                          TrafficLightColor_t color)
{
    /*
     * 74S139 select inputs:
     *   A = bit 0 of color code
     *   B = bit 1 of color code
     *   0 (0b00) -> Y0 (NC)       -> OFF
     *   1 (0b01) -> Y1 (RED_x)    -> RED
     *   2 (0b10) -> Y2 (YELLOW_x) -> AMBER/YELLOW
     *   3 (0b11) -> Y3 (GREEN_x)  -> GREEN
     */
    uint8_t code = (uint8_t)color & 0x03U;

    HAL_GPIO_WritePin(port_a, pin_a, (code & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(port_b, pin_b, (code & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* ---- Public API --------------------------------------------------------- */

void traffic_init(void)
{
    traffic_off_all();
}

void traffic_set(uint8_t road, TrafficLightColor_t color)
{
    if (color > LIGHT_GREEN)
    {
        color = LIGHT_OFF;
    }

    if (road == TRAFFIC_ROAD_1)
    {
        write_decoder(TL1_A_GPIO_Port, TL1_A_Pin,
                      TL1_B_GPIO_Port, TL1_B_Pin,
                      color);
        current_color[0] = color;
    }
    else if (road == TRAFFIC_ROAD_2)
    {
        write_decoder(TL2_A_GPIO_Port, TL2_A_Pin,
                      TL2_B_GPIO_Port, TL2_B_Pin,
                      color);
        current_color[1] = color;
    }
}

void traffic_set_both(TrafficLightColor_t road1_color, TrafficLightColor_t road2_color)
{
    /* Safety interlock: never allow both roads to be GREEN simultaneously */
    if (road1_color == LIGHT_GREEN && road2_color == LIGHT_GREEN)
    {
        road1_color = LIGHT_RED;
        road2_color = LIGHT_RED;
    }

    /* Apply the non-GREEN road first to eliminate transient GREEN+GREEN overlap */
    if (road1_color == LIGHT_GREEN)
    {
        traffic_set(TRAFFIC_ROAD_2, road2_color);
        traffic_set(TRAFFIC_ROAD_1, road1_color);
    }
    else
    {
        traffic_set(TRAFFIC_ROAD_1, road1_color);
        traffic_set(TRAFFIC_ROAD_2, road2_color);
    }
}

TrafficLightColor_t traffic_get(uint8_t road)
{
    if (road == TRAFFIC_ROAD_1)
    {
        return current_color[0];
    }
    if (road == TRAFFIC_ROAD_2)
    {
        return current_color[1];
    }
    return LIGHT_OFF;
}

void traffic_toggle(uint8_t road, TrafficLightColor_t color)
{
    if (traffic_get(road) == color)
    {
        traffic_set(road, LIGHT_OFF);
    }
    else
    {
        traffic_set(road, color);
    }
}

void traffic_off_all(void)
{
    traffic_set(TRAFFIC_ROAD_1, LIGHT_OFF);
    traffic_set(TRAFFIC_ROAD_2, LIGHT_OFF);
}

const char *traffic_color_str(TrafficLightColor_t color)
{
    switch (color)
    {
        case LIGHT_RED:   return "RED";
        case LIGHT_AMBER: return "AMB";
        case LIGHT_GREEN: return "GRN";
        default:          return "OFF";
    }
}
