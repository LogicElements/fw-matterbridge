#include "AppTask.h"
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app::Clusters;

using namespace chip;
using namespace chip::app::Clusters;

void MatterPostAttributeChangeCallback(const app::ConcreteAttributePath& attributePath, uint8_t type, uint16_t size, uint8_t* value)
{
	const ClusterId clusterId = attributePath.mClusterId;
	const AttributeId attributeId = attributePath.mAttributeId;

	if (clusterId == Thermostat::Id)
	{
		if (attributeId == Thermostat::Attributes::LocalTemperature::Id)
		{
			ThermostatMgr().AttributeChangeHandler(ThermostatManager::LocalTemperature, value, size);
		}
		else if (attributeId == Thermostat::Attributes::OccupiedCoolingSetpoint::Id)
		{
			ThermostatMgr().AttributeChangeHandler(ThermostatManager::CoolSet, value, size);
		}
		else if (attributeId == Thermostat::Attributes::OccupiedHeatingSetpoint::Id)
		{
			ThermostatMgr().AttributeChangeHandler(ThermostatManager::HeatSet, value, size);
		}
		else if (attributeId == Thermostat::Attributes::SystemMode::Id)
		{
			ThermostatMgr().AttributeChangeHandler(ThermostatManager::ModeSet, value, size);
		}
	}
	else if (clusterId == Identify::Id)
	{
		ChipLogProgress(Zcl, "Cluster Identify: value set to %u", *value);

		// LightingMgr().StartIdentification(*value);
	}
	else
	{
		ChipLogProgress(NotSpecified, "Unhandled ZCL callback, clusterId %d, attribute %x", clusterId, attributeId);
	}
}