#include "wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include <string.h>

//**************************************************
// Defines
//**************************************************

#define MAX_RETRY 5

//**************************************************
// Globals
//**************************************************

static const char *TAG = "wifi";

static wifi_sta_credentials_t s_config;
static int s_retry_counter = 0;

//**************************************************
// Private Function Prototypes
//**************************************************

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//**************************************************
// Public Functions
//**************************************************

esp_err_t wifi_init(const wifi_sta_credentials_t *config)
{
  if (config == NULL)
  {
    ESP_LOGE(TAG, "Config is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(&s_config, config, sizeof(wifi_sta_credentials_t));

  esp_err_t err = esp_netif_init();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
    return err;
  }

  if (esp_netif_create_default_wifi_sta() == NULL)
  {
    ESP_LOGE(TAG, "Failed to create default WiFi STA netif");
    return ESP_FAIL;
  }

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&cfg);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &event_handler,
                                            NULL,
                                            NULL);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register WIFI_EVENT handler: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_event_handler_instance_register(IP_EVENT,
                                            IP_EVENT_STA_GOT_IP,
                                            &event_handler,
                                            NULL,
                                            NULL);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register IP_EVENT handler: %s", esp_err_to_name(err));
    return err;
  }

  wifi_config_t wifi_sta_config = {0};
  strncpy((char *)wifi_sta_config.sta.ssid, config->ssid, sizeof(wifi_sta_config.sta.ssid) - 1);
  strncpy((char *)wifi_sta_config.sta.password, config->password, sizeof(wifi_sta_config.sta.password) - 1);
  wifi_sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_wifi_set_mode failed: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_wifi_set_config failed: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_wifi_start();
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "WiFi station started. Connecting to SSID: %s", config->ssid);

  return ESP_OK;
}

//**************************************************
// Private Functions
//**************************************************

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_counter = 0;
    return;
  }

  if (event_base != WIFI_EVENT)
  {
    return;
  }

  switch (event_id)
  {
  case WIFI_EVENT_STA_START:
    ESP_LOGI(TAG, "STA started");
    s_retry_counter = 0;
    esp_wifi_connect();
    break;

  case WIFI_EVENT_STA_CONNECTED:
    ESP_LOGI(TAG, "Connected to AP");
    break;

  case WIFI_EVENT_STA_DISCONNECTED:
    if (s_retry_counter < MAX_RETRY)
    {
      s_retry_counter++;
      ESP_LOGW(TAG, "Disconnected. Retrying connection... (%d/%d)", s_retry_counter, MAX_RETRY);
      esp_wifi_connect();
    }
    else
    {
      ESP_LOGE(TAG, "Failed to connect to AP after %d attempts", MAX_RETRY);
    }
    break;

  default:
    break;
  }
}
