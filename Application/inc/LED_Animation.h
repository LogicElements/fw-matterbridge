#ifndef LED_ANIMATION_H
#define LED_ANIMATION_H

#include <cmath>
#include <cstdint>

class LED
{
public:
	enum class Effect
	{
		Off,
		Solid,
		Blinking,
		Breathing
	};

	explicit LED(uint32_t led, Effect effect = Effect::Solid, uint32_t period = 1000, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

	void SetEffect(Effect effect);
	void SetPeriod(uint32_t period);
	void SetColor(uint8_t r, uint8_t g, uint8_t b);

	void Update();

	static bool dirty;
	static float GlobalBrightness;

private:
	Effect effect;
	uint32_t period;
	uint32_t led;

	uint8_t baseR, baseG, baseB;
	float oldBrightness = 0.0f;
};


#endif