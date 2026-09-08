#pragma once

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(ANALOG_EVENT);

typedef enum
{
  ANALOG_EVENT_NEW_VALUE,
} analog_event_t;

typedef struct
{
  int value;
} analog_event_new_data_payload_t;
