#include "nvs_flash.h"
#include "esp_event.h"
#include "wifi.h"
#include "iota_json.h"

#include <stdio.h>

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

  int value = 0;
  char str[16] = "";

  while (1)
  {
    sprintf(str, "%d", value);
    iota_json_send_attr("pot", str);
    value++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}