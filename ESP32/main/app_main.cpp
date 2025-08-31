#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include <log_heap_numbers.h>

#include <app_priv.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>

#include "temp_sensor.h"

static const char *TAG = "app_main";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg) {
	switch (event->Type) {
		case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
			ESP_LOGI(TAG, "Interface IP Address changed");
			break;

		case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
			ESP_LOGI(TAG, "Commissioning complete");
			MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning complete");
			break;

		case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
			ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
			break;

		case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
			ESP_LOGI(TAG, "Commissioning session started");
			break;

		case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
			ESP_LOGI(TAG, "Commissioning session stopped");
			break;

		case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
			ESP_LOGI(TAG, "Commissioning window opened");
			MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning window opened");
			break;

		case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
			ESP_LOGI(TAG, "Commissioning window closed");
			break;

		case chip::DeviceLayer::DeviceEventType::kFabricRemoved: {
			ESP_LOGI(TAG, "Fabric removed successfully");
			if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
				chip::CommissioningWindowManager &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
				constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(300);
				if (!commissionMgr.IsCommissioningWindowOpen()) {
					CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds, chip::CommissioningWindowAdvertisement::kDnssdOnly);
					if (err != CHIP_NO_ERROR) {
						ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
					}
				}
			}
			break;
		}

		case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
			ESP_LOGI(TAG, "Fabric will be removed");
			break;

		case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
			ESP_LOGI(TAG, "Fabric is updated");
			break;

		case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
			ESP_LOGI(TAG, "Fabric is committed");
			break;

		case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
			ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
			MEMORY_PROFILER_DUMP_HEAP_STAT("BLE deinitialized");
			break;

		default:
			break;
	}
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, uint8_t effect_variant, void *priv_data) {
	ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
	return ESP_OK;
}

static esp_err_t app_attribute_update_cb(callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
	return ESP_OK;
}

static void temp_sensor_notification(uint16_t endpoint_id, float temperature, void *user_data) {
	chip::DeviceLayer::SystemLayer().ScheduleLambda([endpoint_id, temperature] {
		attribute_t *attribute = attribute::get(endpoint_id, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id);

		esp_matter_attr_val_t val = esp_matter_invalid(nullptr);
		get_val(attribute, &val);
		val.val.i16 = static_cast<int16_t>(temperature * 100);
		update(endpoint_id, TemperatureMeasurement::Id, TemperatureMeasurement::Attributes::MeasuredValue::Id, &val);
	});
}

extern "C" void app_main() {
	esp_err_t err = ESP_OK;

	nvs_flash_init();

	node::config_t node_config;
	node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
	ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

	temperature_sensor::config_t temp_sensor_config;
	endpoint_t *temp_sensor_ep = temperature_sensor::create(node, &temp_sensor_config, ENDPOINT_FLAG_NONE, nullptr);
	ABORT_APP_ON_FAILURE(temp_sensor_ep != nullptr, ESP_LOGE(TAG, "Failed to create temperature_sensor endpoint"));

	static temp_sensor_config_t temp_config = {
		.cb = temp_sensor_notification,
		.endpoint_id = endpoint::get_id(temp_sensor_ep),
	};
	err = temp_sensor_init(&temp_config);
	ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialize temperature sensor driver"));

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
	esp_openthread_platform_config_t config = {
		.radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
		.host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
		.port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
	};
	set_openthread_platform_config(&config);
#endif

	err = start(app_event_cb);
	ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));
}
