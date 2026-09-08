#include "nvs_flash.h"
#include "esp_event.h"
#include "wifi.h"
#include "iota_json.h"
#include "analog.h"
#include "analog_events.h"
#include "io.h"
#include "io_events.h"

#include <stdio.h>

//**************************************************
// Private Function Prototypes
//**************************************************

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

//**************************************************
// Public Functions
//**************************************************

void app_main(void)
{
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_sta_credentials_t credentials = {
      .ssid = CONFIG_WIFI_SSID,
      .password = CONFIG_WIFI_PASSWORD,
  };

  ESP_ERROR_CHECK(wifi_init(&credentials));

  iota_json_config_t iota_json_config = {
      .mqtt_broker = "mqtts://192.168.1.65:8883",
      .api_key = "apitest",
      .device_id = "esp32_01",
      .user_name = "greenhouse",
      .user_password = "0123456789",
  };

  ESP_ERROR_CHECK(iota_json_init(&iota_json_config));

  ESP_ERROR_CHECK(analog_init());
  ESP_ERROR_CHECK(io_init());

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      ANALOG_EVENT,
      ANALOG_EVENT_NEW_VALUE,
      event_handler,
      NULL,
      NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IO_EVENT,
      IO_EVENT_NEW_INPUT,
      event_handler,
      NULL,
      NULL));
}

//**************************************************
// Private Functions
//**************************************************

static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  if (base == ANALOG_EVENT && event_id == ANALOG_EVENT_NEW_VALUE)
  {
    analog_event_new_data_payload_t *payload = (analog_event_new_data_payload_t *)event_data;

    char str[16] = "";
    sprintf(str, "%d", payload->value);
    iota_json_send_attr("pot", str);

    return;
  }

  if (base == IO_EVENT && event_id == IO_EVENT_NEW_INPUT)
  {
    io_event_new_input_payload_t *payload = (io_event_new_input_payload_t *)event_data;

    char str[16] = "";
    sprintf(str, "%d", payload->button_one);
    iota_json_send_attr("btn", str);

    return;
  }
}
