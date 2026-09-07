#include "iota_json.h"

#include <stdio.h>

void app_main(void)
{
  iota_json_config_t iota_json_config = {
    .mqtt_broker = "mqtts://192.168.1.65:8883",
      .api_key = "apitest",
      .device_id = "esp32_01",
      .user_name = "greenhouse",
      .user_password = "0123456789",
  };

  ESP_ERROR_CHECK(iota_json_init(&iota_json_config));
}