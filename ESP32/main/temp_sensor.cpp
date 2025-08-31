#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <temp_sensor.h>
#include <lib/support/CodeUtils.h>
#include "driver/spi_master.h"

static const char *TAG = "MAX6675";

typedef struct {
	temp_sensor_config_t *config;
	esp_timer_handle_t timer;
	bool is_initialized = false;
} temp_sensor_ctx_t;

static temp_sensor_ctx_t s_ctx;

#define PIN_NUM_MISO 11
#define PIN_NUM_MOSI (-1)
#define PIN_NUM_CLK  3
#define PIN_NUM_CS   2

static spi_device_handle_t max6675_handle;

esp_err_t max6675_init() {
	spi_bus_config_t bus_config = {
		.mosi_io_num = PIN_NUM_MOSI,
		.miso_io_num = PIN_NUM_MISO,
		.sclk_io_num = PIN_NUM_CLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 2,
	};

	esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_DISABLED);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SPI, err:%d", err);
		return err;
	}

	spi_device_interface_config_t device_config = {
		.mode = 0,
		.clock_speed_hz = 1000000,
		.spics_io_num = PIN_NUM_CS,
		.queue_size = 1
	};

	return spi_bus_add_device(SPI2_HOST, &device_config, &max6675_handle);
}

static esp_err_t max6675_read_temp(float &temperature) {
	uint8_t rx_buf[2] = {};

	spi_transaction_t t = {
		.length = 16,
		.rxlength = 16,
		.tx_buffer = nullptr,
		.rx_buffer = rx_buf,
	};

	esp_err_t err = spi_device_transmit(max6675_handle, &t);
	if (err != ESP_OK) {
		return err;
	}

	uint16_t raw_temperature = static_cast<uint16_t>(rx_buf[0]) << 8 | rx_buf[1];

	ESP_LOGI(TAG, "Raw data: 0x%02X 0x%02X", rx_buf[0], rx_buf[1]);

	if (raw_temperature & 0x4) {
		ESP_LOGW(TAG, "Thermocouple not connected");
		return ESP_ERR_NOT_FOUND;
	}

	raw_temperature >>= 3;

	temperature = raw_temperature * 0.25f;

	return ESP_OK;
}

static void timer_cb_internal(void *arg) {
	const auto *ctx = static_cast<temp_sensor_ctx_t *>(arg);

	if (!(ctx && ctx->config)) {
		return;
	}

	float temperature = 0.0f;
	esp_err_t err = max6675_read_temp(temperature);
	if (err != ESP_OK) {
		return;
	}

	if (ctx->config->cb) {
		ctx->config->cb(ctx->config->endpoint_id, temperature, ctx->config->user_data);
	}
}

esp_err_t temp_sensor_init(temp_sensor_config_t *config) {
	if (config == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (config->cb == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}

	if (s_ctx.is_initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	esp_err_t err = max6675_init();
	if (err != ESP_OK) {
		return err;
	}

	s_ctx.config = config;

	const esp_timer_create_args_t args = {
		.callback = timer_cb_internal,
		.arg = &s_ctx,
	};

	err = esp_timer_create(&args, &s_ctx.timer);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_timer_create failed, err:%d", err);
		return err;
	}

	err = esp_timer_start_periodic(s_ctx.timer, config->interval_ms * 1000);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "esp_timer_start_periodic failed: %d", err);
		return err;
	}

	s_ctx.is_initialized = true;
	ESP_LOGI(TAG, "temp initialized successfully");

	return ESP_OK;
}
