#ifndef MATTERBRIDGE_LED_H
#define MATTERBRIDGE_LED_H

#ifdef __cplusplus
extern "C" {



#endif

#include <stdint.h>
#include <stm32wbxx_hal.h>

typedef struct Color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Color;

void LED_Init(void);

void LED_Send();

void LED_Reset();

void LED_SetColorRGB(uint8_t led, uint8_t r, uint8_t g, uint8_t b);

void LED_SetColor(uint8_t led, Color color);

#ifdef __cplusplus
}
#endif

#endif