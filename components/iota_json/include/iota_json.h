#pragma once

#include "esp_err.h"

//**************************************************
// Structs
//**************************************************

typedef struct
{
  char mqtt_broker[256];
  char api_key[64];
  char device_id[64];
  char user_name[64];
  char user_password[64];
} iota_json_config_t;

//**************************************************
// Public Functions
//**************************************************

esp_err_t iota_json_init(const iota_json_config_t *config);

esp_err_t iota_json_send_attr(const char *name, const char *value);