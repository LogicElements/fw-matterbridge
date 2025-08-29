#include <memory.h>
#include "LED.h"

#define NUM_LEDS 5
#define BLANK_INTERVAL 100
#define LOW_DUTY 22 // should be between 200-400 ns
#define HIGH_DUTY 44 // should be between 580-1000 ns

#define LED_LENGTH (24 * NUM_LEDS)
#define DATA_LENGTH (LED_LENGTH + 2 * BLANK_INTERVAL)

uint32_t data[DATA_LENGTH];

extern TIM_HandleTypeDef htim1;

void LED_Init(void)
{
	LED_Reset();
}

void LED_Send()
{
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3, data, DATA_LENGTH);
}

void LED_Reset()
{
	memset(data, 0, DATA_LENGTH);

	uint32_t* offset = data + BLANK_INTERVAL;
	for (uint32_t i = 0; i < LED_LENGTH; i++)
	{
		offset[i] = 22;
	}
}

void LED_SetColorRGB(const uint8_t led, const uint8_t r, const uint8_t g, const uint8_t b)
{
	const uint32_t color = (uint32_t)g << 16 | (uint32_t)r << 8 | (uint32_t)b;

	uint32_t* offset = data + BLANK_INTERVAL;
	for (int i = 23; i >= 0; i--)
	{
		if (color & 1 << i)
			offset[24 * led + 23 - i] = HIGH_DUTY;
		else
			offset[24 * led + 23 - i] = LOW_DUTY;
	}
}

void LED_SetColor(const uint8_t led, const Color color)
{
	LED_SetColorRGB(led, color.r, color.g, color.b);
}