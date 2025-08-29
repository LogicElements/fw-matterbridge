#include "ThermostatManager.h"
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include "platform/KeyValueStoreManager.h"

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::PersistedStorage;

namespace ThermAttr = app::Clusters::Thermostat::Attributes;
namespace ThermMode = app::Clusters::Thermostat;

constexpr EndpointId kThermostatEndpoint = 1;
app::DataModel::Nullable<int16_t> mTemp, mOutTemp;
int16_t mHeatSet, mCoolSet;

const char skCoolSet[] = "a/cs";
const char skHeatSet[] = "a/hs";
const char skSystemMode[] = "a/sm";

ThermostatManager ThermostatManager::sThermostatMgr;

CHIP_ERROR ThermostatManager::Init()
{
	RestoreSetup();

	mCurrentTempCelsius = -127;

	mHeatingCelsiusSetPoint = mHeatSet;
	mCoolingCelsiusSetPoint = mCoolSet;

	mInternal_read_ok = false;

	ChipLogProgress(NotSpecified, "sTempMgr.Init, Local temp %d", mCurrentTempCelsius);
	ChipLogProgress(NotSpecified, "sTempMgr.Init, CoolingSetpoint %d", mCoolingCelsiusSetPoint);
	ChipLogProgress(NotSpecified, "sTempMgr.Init, HeatingSetpoint %d", mHeatingCelsiusSetPoint);
	ChipLogProgress(NotSpecified, "sTempMgr.Init, SystemMode %d", mSystemMode);

	return CHIP_NO_ERROR;
}

void ThermostatManager::RestoreSetup()
{
	size_t outLen;

	// try to read values from NVM. If not yet present, get the (default) value from stack and save it to NVM.
	// If present, send the saved value to the stack
	CHIP_ERROR err = KeyValueStoreMgr().Get(skCoolSet, &mCoolSet, sizeof(int16_t), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Get(kThermostatEndpoint, &mCoolSet);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skCoolSet, &mCoolSet, sizeof(int16_t));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Set(kThermostatEndpoint, mCoolSet);
		PlatformMgr().UnlockChipStack();
	}

	err = KeyValueStoreMgr().Get(skHeatSet, &mHeatSet, sizeof(int16_t), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Get(kThermostatEndpoint, &mHeatSet);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skHeatSet, &mHeatSet, sizeof(int16_t));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Set(kThermostatEndpoint, mHeatSet);
		PlatformMgr().UnlockChipStack();
	}

	err = KeyValueStoreMgr().Get(skSystemMode, &mSystemMode, sizeof(ThermMode::SystemModeEnum), &outLen);
	if (err != CHIP_NO_ERROR)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::SystemMode::Get(kThermostatEndpoint, &mSystemMode);
		PlatformMgr().UnlockChipStack();

		err = KeyValueStoreMgr().Put(skSystemMode, &mSystemMode, sizeof(ThermMode::SystemModeEnum));
	}
	else
	{
		PlatformMgr().LockChipStack();
		ThermAttr::SystemMode::Set(kThermostatEndpoint, mSystemMode);
		PlatformMgr().UnlockChipStack();
	}
}

void ThermostatManager::SetCallbacks(ThermostatCallback_fn aActionCompleted_CB)
{
	mActionCompleted_CB = aActionCompleted_CB;
}

void ThermostatManager::AttributeChangeHandler(const Action_t aAction, uint8_t* value, uint16_t size)
{
	switch (aAction)
	{
		case LocalTemperature:
			{
				if (mInternal_read_ok)
				{
					const int8_t temp = (*reinterpret_cast<int16_t*>(value));
					ChipLogProgress(NotSpecified, "TempMgr: Local temp %d", temp);
					mCurrentTempCelsius = temp;
				}
				else
				{
					ChipLogProgress(NotSpecified, "TempMgr: Local temp NULL");
					mCurrentTempCelsius = -127;
				}
			}
			break;
		case CoolSet:
			{
				mCoolingCelsiusSetPoint = *reinterpret_cast<int16_t*>(value);

				const CHIP_ERROR err = KeyValueStoreMgr().Put(skCoolSet, value, sizeof(int16_t));
				ChipLogProgress(NotSpecified, "TempMgr: CoolingSetpoint %d. NVM.Put-> %d", *reinterpret_cast<int16_t*>(value), err);
			}
			break;

		case HeatSet:
			{
				mHeatingCelsiusSetPoint = *reinterpret_cast<int16_t*>(value);

				const CHIP_ERROR err = KeyValueStoreMgr().Put(skHeatSet, value, sizeof(int16_t));
				ChipLogProgress(NotSpecified, "TempMgr: HeatingSetpoint %d. NVM.Put-> %d", *reinterpret_cast<int16_t*>(value), err);
			}
			break;

		case ModeSet:
			{
				mSystemMode = static_cast<ThermMode::SystemModeEnum>(*value);

				const CHIP_ERROR err = KeyValueStoreMgr().Put(skSystemMode, value, sizeof(ThermMode::SystemModeEnum));
				ChipLogProgress(NotSpecified, "TempMgr: SystemMode %d. NVM.Put-> %d", mSystemMode, err);
			}
			break;

		default:
			{
				return;
			}
			break;
	}

	if (mActionCompleted_CB)
	{
		mActionCompleted_CB(aAction);
	}
}

ThermMode::SystemModeEnum ThermostatManager::GetMode() const
{
	return mSystemMode;
}

int8_t ThermostatManager::GetCurrentTemp() const
{
	return mCurrentTempCelsius;
}

int8_t ThermostatManager::GetHeatingSetPoint() const
{
	return mHeatingCelsiusSetPoint;
}

int8_t ThermostatManager::GetCoolingSetPoint() const
{
	return mCoolingCelsiusSetPoint;
}

void ThermostatManager::SetCurrentTemp(const int16_t temp)
{
	//	mCurrentTempCelsius = ConvertToPrintableTemp(temp);
	PlatformMgr().LockChipStack();
	ThermAttr::LocalTemperature::Set(kThermostatEndpoint, temp);
	PlatformMgr().UnlockChipStack();
}

void ThermostatManager::SetSetpoints(const int16_t cool_change, const int16_t heat_change)
{
	int16_t heatingSetpoint, coolingSetpoint;

	if (cool_change != 0)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedCoolingSetpoint::Get(kThermostatEndpoint, &coolingSetpoint);
		coolingSetpoint += cool_change;
		ThermAttr::OccupiedCoolingSetpoint::Set(kThermostatEndpoint, coolingSetpoint);
		PlatformMgr().UnlockChipStack();
	}
	if (heat_change != 0)
	{
		PlatformMgr().LockChipStack();
		ThermAttr::OccupiedHeatingSetpoint::Get(kThermostatEndpoint, &heatingSetpoint);
		heatingSetpoint += heat_change;
		ThermAttr::OccupiedHeatingSetpoint::Set(kThermostatEndpoint, heatingSetpoint);
		PlatformMgr().UnlockChipStack();
	}
}

void ThermostatManager::SetIntReadOk(const bool ok)
{
	mInternal_read_ok = ok;
}