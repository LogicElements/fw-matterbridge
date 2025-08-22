#include "AppTask.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip;
using namespace chip::app::Clusters;

void MatterPostAttributeChangeCallback(const app::ConcreteAttributePath& attributePath, uint8_t type, uint16_t size, uint8_t* value)
{
	ClusterId clusterId = attributePath.mClusterId;
	AttributeId attributeId = attributePath.mAttributeId;

	if (clusterId == Thermostat::Id && attributeId == Thermostat::Attributes::SystemMode::Id)
	{
		ChipLogProgress(Zcl, "Cluster OnOff: attribute OnOff set to %" PRIu8, *value);
	}
}


void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
}