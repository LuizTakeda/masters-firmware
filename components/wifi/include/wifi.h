#pragma once

#include "esp_err.h"
#include "esp_wifi.h"

//**************************************************
// Structs
//**************************************************

typedef struct
{
  char ssid[64];
  char password[64];
} wifi_sta_credentials_t;

//**************************************************
// Public Functions
//**************************************************

esp_err_t wifi_init(const wifi_sta_credentials_t *config);