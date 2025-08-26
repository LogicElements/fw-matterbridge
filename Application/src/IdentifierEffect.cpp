#include "AppEvent.h"
#include "AppTask.h"
#include "app_common.h"
#include <app/clusters/identify-server/identify-server.h>

#include "LED_Animation.h"

app::Clusters::Identify::EffectIdentifierEnum sIdentifyEffect = app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;

extern LED ledIdentifier;

namespace
{
	void OnTriggerIdentifyEffectCompleted(System::Layer* systemLayer, void* appState)
	{
		sIdentifyEffect = app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;
	}
}

void OnTriggerIdentifyEffect(Identify* identify)
{
	sIdentifyEffect = identify->mCurrentEffectIdentifier;

	if (identify->mCurrentEffectIdentifier == app::Clusters::Identify::EffectIdentifierEnum::kChannelChange)
	{
		ChipLogProgress(Zcl, "IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE - Not supported, use effect variant %d", static_cast<int>(identify->mEffectVariant));
		sIdentifyEffect = static_cast<app::Clusters::Identify::EffectIdentifierEnum>(identify->mEffectVariant);
	}

	switch (sIdentifyEffect)
	{
		case app::Clusters::Identify::EffectIdentifierEnum::kBlink:
			ChipLogProgress(DeviceLayer, "IDENTIFY : kBlink");
		case app::Clusters::Identify::EffectIdentifierEnum::kBreathe:
			ChipLogProgress(DeviceLayer, "IDENTIFY : kBreathe");
		case app::Clusters::Identify::EffectIdentifierEnum::kOkay:
			ChipLogProgress(DeviceLayer, "IDENTIFY : kOkay");

			DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds16(5), OnTriggerIdentifyEffectCompleted, identify);

			break;
		case app::Clusters::Identify::EffectIdentifierEnum::kFinishEffect:
			ChipLogProgress(DeviceLayer, "IDENTIFY : kFinishEffect");

			DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
			DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds16(1), OnTriggerIdentifyEffectCompleted, identify);

			break;
		case app::Clusters::Identify::EffectIdentifierEnum::kStopEffect:
			ChipLogProgress(DeviceLayer, "IDENTIFY : kStopEffect");

			DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
			sIdentifyEffect = app::Clusters::Identify::EffectIdentifierEnum::kStopEffect;

			break;
		default:
			ChipLogProgress(Zcl, "No identifier effect");
	}
}

void OnIdentifyStart(Identify*)
{
	ChipLogProgress(Zcl, "OnIdentifyStart");

	ledIdentifier.SetEffect(LED::Effect::Solid);
}

void OnIdentifyStop(Identify*)
{
	ChipLogProgress(Zcl, "OnIdentifyStop");

	ledIdentifier.SetEffect(LED::Effect::Off);
}

static Identify gIdentify = {
	EndpointId{1},
	OnIdentifyStart,
	OnIdentifyStop,
	app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator,
	OnTriggerIdentifyEffect,
};