#pragma once

#include <esp_err.h>

using temp_sensor_cb_t = void (*)(uint16_t endpoint_id, float temperature, void *user_data);

typedef struct {
	temp_sensor_cb_t cb = nullptr;
	uint16_t endpoint_id = 0;
	void *user_data = nullptr;
	uint32_t interval_ms = 5000;
} temp_sensor_config_t;

/**
 * @brief Initialize sensor driver. This function should be called only once
 *
 * @param config sensor configurations. This should last for the lifetime of the driver
 *               as driver layer do not make a copy of this object.
 *
 * @return esp_err_t - ESP_OK on success,
 *                     ESP_ERR_INVALID_ARG if config is NULL
 *                     ESP_ERR_INVALID_STATE if driver is already initialized
 *                     appropriate error code otherwise
 */
esp_err_t temp_sensor_init(temp_sensor_config_t *config);