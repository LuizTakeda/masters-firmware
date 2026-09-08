#pragma once

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(IO_EVENT);

typedef enum
{
  IO_EVENT_NEW_INPUT,
} io_event_id_t;

typedef struct
{
  bool button_one;
  bool button_two;
} io_event_new_input_payload_t;