#include "LED_Animation.h"

#include <algorithm>
#include <cmath>

#include "LED.h"

bool LED::dirty = true;
float LED::GlobalBrightness = 1.0f;

LED::LED(const uint32_t led, const Effect effect, const uint32_t period, const uint8_t r, const uint8_t g, const uint8_t b)
	: effect(effect), period(period), led(led), baseR(r), baseG(g), baseB(b)
{
}

void LED::SetEffect(const Effect effect)
{
	this->effect = effect;
}

void LED::SetPeriod(const uint32_t period)
{
	this->period = period;
}

void LED::SetColor(const uint8_t r, const uint8_t g, const uint8_t b)
{
	this->baseR = r;
	this->baseG = g;
	this->baseB = b;
}

void LED::Update()
{
	const uint32_t now = HAL_GetTick();

	float brightness = oldBrightness;

	switch (effect)
	{
		case Effect::Off:
			{
				brightness = 0.0f;

				break;
			}
		case Effect::Solid:
			{
				brightness = 1.0f;

				break;
			}
		case Effect::Blinking:
			{
				const auto elapsed = static_cast<float>(now % period);
				const float onTime = static_cast<float>(period) * 0.5f;

				if (elapsed < onTime)
				{
					brightness = 1.0f;
				}
				else
				{
					brightness = 0.0f;
				}
				break;
			}

		case Effect::Breathing:
			{
				if (now % 10 == 0)
				{
					const auto elapsed = static_cast<float>(now % period);
					const float time = elapsed / static_cast<float>(period);

					brightness = 0.55f + sinf(2 * 3.14159f * time) * 0.45f;
				}

				break;
			}
	}

	// brightness *= GlobalBrightness;

	if (brightness != oldBrightness)
	{
		brightness = std::clamp(brightness * GlobalBrightness, 0.0f, 1.0f);

		const auto r = static_cast<uint8_t>(static_cast<float>(baseR) * brightness);
		const auto g = static_cast<uint8_t>(static_cast<float>(baseG) * brightness);
		const auto b = static_cast<uint8_t>(static_cast<float>(baseB) * brightness);

		LED_SetColorRGB(led, r, g, b);

		dirty = true;
		oldBrightness = brightness;
	}
}