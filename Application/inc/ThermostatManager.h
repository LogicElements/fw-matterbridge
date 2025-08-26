#pragma once

#include <functional>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <lib/core/CHIPError.h>

using namespace chip;
namespace ThermMode = app::Clusters::Thermostat;

class ThermostatManager
{
public:
	enum Action_t
	{
		Invalid = 0,
		CoolSet,
		HeatSet,
		ModeSet,
		LocalTemperature,
	} Action;

    CHIP_ERROR Init();
    void AttributeChangeHandler(Action_t aAction, uint8_t * value, uint16_t size);
    ThermMode::SystemModeEnum GetMode() const;
    int8_t GetCurrentTemp() const;
    int8_t GetHeatingSetPoint() const;
    int8_t GetCoolingSetPoint() const;
    void SetCurrentTemp(int16_t temp);
    void SetSetpoints(int16_t cool_change, int16_t heat_change);

    using ThermostatCallback_fn = std::function<void(Action_t)>;
    void SetCallbacks(ThermostatCallback_fn aActionCompleted_CB);

    void SetIntReadOk(bool ok);


private:
    friend ThermostatManager & ThermostatMgr();

    int8_t mCurrentTempCelsius;
    int8_t mCoolingCelsiusSetPoint;
    int8_t mHeatingCelsiusSetPoint;
    ThermMode::SystemModeEnum mSystemMode;

    ThermostatCallback_fn mActionCompleted_CB;

    bool mInternal_read_ok;

    void RestoreSetup();

    // int8_t ConvertToPrintableTemp(int16_t temperature);
    static ThermostatManager sThermostatMgr;
};

inline ThermostatManager & ThermostatMgr()
{
    return ThermostatManager::sThermostatMgr;
}
