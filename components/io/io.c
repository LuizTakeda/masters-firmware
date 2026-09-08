#include <stdio.h>
#include "io.h"
#include "io_events.h"
#include "iot_button.h"
#include "button_gpio.h"

#include "esp_log.h"
#include "freertos/semphr.h"

//**************************************************
// Defines
//**************************************************

#define BUTTON_ONE_PIN 14
#define BUTTON_TWO_PIN 35

ESP_EVENT_DEFINE_BASE(IO_EVENT);

//**************************************************
// Struct
//**************************************************

typedef struct
{
  uint8_t id;
  bool value;
} button_context_t;

//**************************************************
// Globals
//**************************************************

static const char TAG[] = "io";

static bool s_button_states[2] = {false, false};

static SemaphoreHandle_t s_button_mutex = NULL;

//**************************************************
// Function Prototypes
//**************************************************

static void button_press_down_cb(void *arg, void *usr_data);
static void button_up_cb(void *arg, void *usr_data);

//**************************************************
// Public Functions
//**************************************************

esp_err_t io_init()
{
  s_button_mutex = xSemaphoreCreateMutex();

  if (s_button_mutex == NULL)
  {
    ESP_LOGE(TAG, "Fail to create mutex");
    return ESP_FAIL;
  }

  const button_config_t btn_cfg = {0};

  const button_gpio_config_t btn_one_cfg = {
      .gpio_num = BUTTON_ONE_PIN,
      .active_level = 0,
  };

  const button_gpio_config_t btn_two_cfg = {
      .gpio_num = BUTTON_TWO_PIN,
      .active_level = 0,
  };

  button_handle_t btn_one = NULL;
  ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_one_cfg, &btn_one));

  button_handle_t btn_two = NULL;
  ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &btn_two_cfg, &btn_two));

  iot_button_register_cb(btn_one, BUTTON_PRESS_DOWN, NULL, button_press_down_cb, &(s_button_states[0]));
  iot_button_register_cb(btn_one, BUTTON_PRESS_UP, NULL, button_up_cb, &(s_button_states[0]));

  iot_button_register_cb(btn_two, BUTTON_PRESS_DOWN, NULL, button_press_down_cb, &(s_button_states[1]));
  iot_button_register_cb(btn_two, BUTTON_PRESS_UP, NULL, button_up_cb, &(s_button_states[1]));

  return ESP_OK;
}

//**************************************************
// Private Functions
//**************************************************

static void button_press_down_cb(void *arg, void *usr_data)
{
  bool *button_state = (bool *)usr_data;

  if (xSemaphoreTake(s_button_mutex, portMAX_DELAY) != pdTRUE)
  {
    return;
  }

  *button_state = true;

  io_event_new_input_payload_t payload = {
      .button_one = s_button_states[0],
      .button_two = s_button_states[1],
  };

  esp_event_post(IO_EVENT, IO_EVENT_NEW_INPUT, (void *)&payload, sizeof(io_event_new_input_payload_t), pdMS_TO_TICKS(50));

  xSemaphoreGive(s_button_mutex);
}

static void button_up_cb(void *arg, void *usr_data)
{
  bool *button_state = (bool *)usr_data;

  if (xSemaphoreTake(s_button_mutex, portMAX_DELAY) != pdTRUE)
  {
    return;
  }

  *button_state = false;

  io_event_new_input_payload_t payload = {
      .button_one = s_button_states[0],
      .button_two = s_button_states[1],
  };

  esp_event_post(IO_EVENT, IO_EVENT_NEW_INPUT, (void *)&payload, sizeof(io_event_new_input_payload_t), pdMS_TO_TICKS(50));

  xSemaphoreGive(s_button_mutex);
}
