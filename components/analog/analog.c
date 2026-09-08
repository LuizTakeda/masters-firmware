#include "analog.h"
#include "analog_events.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

//**************************************************
// Defines
//**************************************************

ESP_EVENT_DEFINE_BASE(ANALOG_EVENT);

//**************************************************
// Globals
//**************************************************

adc_oneshot_unit_handle_t adc1_handle;
static const char TAG[] = "analog";

//**************************************************
// Function Prototypes
//**************************************************

static void task(void *pvParameters);

//**************************************************
// Public Functions
//**************************************************

esp_err_t analog_init(void)
{
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = ADC_UNIT_1,
  };

  esp_err_t err = adc_oneshot_new_unit(&init_config1, &adc1_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Fail to create new unit: %s", esp_err_to_name(err));
    return err;
  }

  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  err = adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Fail to config channel: %s", esp_err_to_name(err));
    return err;
  }

  if (xTaskCreate(task, "analog", 2048, NULL, 2, NULL) != pdTRUE)
  {
    ESP_LOGE(TAG, "Fail to create task");
    return ESP_FAIL;
  }

  return ESP_OK;
}

//**************************************************
// Private Functions
//**************************************************

static void task(void *pvParameters)
{
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));

    analog_event_new_data_payload_t new_data;

    esp_err_t err = adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &new_data.value);

    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Fail to read adc value: %s", esp_err_to_name(err));
      continue;
    }

    err = esp_event_post(ANALOG_EVENT, ANALOG_EVENT_NEW_VALUE, (void *)&new_data, sizeof(analog_event_new_data_payload_t), pdMS_TO_TICKS(50));

    if (err != ESP_OK)
    {
      ESP_LOGE(TAG, "Fail to post event: %s", esp_err_to_name(err));
      continue;
    }
  }
}