#include "iota_json.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <stdio.h>
#include <string.h>

#define DEVELOPMENT

//**************************************************
// Globals
//**************************************************

static const char *TAG = "iota_json";

extern const uint8_t rootCA_pem_start[] asm("_binary_rootCA_pem_start");
extern const uint8_t rootCA_pem_end[] asm("_binary_rootCA_pem_end");

static esp_mqtt_client_handle_t s_client = NULL;
static iota_json_config_t s_config;
static bool s_mqtt_started = false;

//**************************************************
// Private Function Prototypes
//**************************************************

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

//**************************************************
// Public Functions
//**************************************************

esp_err_t iota_json_init(const iota_json_config_t *config)
{
  if (config == NULL)
  {
    ESP_LOGE(TAG, "Configuration is NULL");
    return ESP_ERR_INVALID_ARG;
  }

  memcpy(&s_config, config, sizeof(iota_json_config_t));

  const esp_mqtt_client_config_t mqtt_config = {
      .broker = {
          .address.uri = config->mqtt_broker,
          .verification = {
              .certificate = (const char *)rootCA_pem_start,
#ifdef DEVELOPMENT
              .skip_cert_common_name_check = true,
#endif
          },
      },
      .credentials = {
          .client_id = config->device_id,
          .username = config->user_name,
          .authentication.password = config->user_password,
      },
  };

  s_client = esp_mqtt_client_init(&mqtt_config);
  if (s_client == NULL)
  {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return ESP_FAIL;
  }

  esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

  esp_err_t err = esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      NULL);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register WIFI_EVENT handler: %s", esp_err_to_name(err));
    return err;
  }

  err = esp_event_handler_instance_register(IP_EVENT,
                                            IP_EVENT_STA_GOT_IP,
                                            &wifi_event_handler,
                                            NULL,
                                            NULL);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to register IP_EVENT handler: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "Initialized");

  return ESP_OK;
}

esp_err_t iota_json_send_attr(const char *name, const char *value)
{
  if (s_client == NULL || name == NULL || value == NULL)
  {
    ESP_LOGE(TAG, "Invalid argument or MQTT client not initialized");
    return ESP_ERR_INVALID_ARG;
  }

  char topic[160];
  snprintf(topic, sizeof(topic), "/json/%s/%s/attrs/%s", s_config.api_key, s_config.device_id, name);

  int msg_id = esp_mqtt_client_publish(s_client, topic, value, 0, 1, 0);
  if (msg_id < 0)
  {
    ESP_LOGE(TAG, "Failed to publish attribute '%s' (msg_id=%d)", name, msg_id);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Published to %s: %s (msg_id=%d)", topic, value, msg_id);
  return ESP_OK;
}


//**************************************************
// Private Functions
//**************************************************

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;

  switch ((esp_mqtt_event_id_t)event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

    // No IoT Agent JSON, inscreve-se no tópico de comandos: /json/<api_key>/<device_id>/cmd
    if (strlen(s_config.api_key) > 0 && strlen(s_config.device_id) > 0)
    {
      char cmd_topic[160];
      snprintf(cmd_topic, sizeof(cmd_topic), "/json/%s/%s/cmd", s_config.api_key, s_config.device_id);
      msg_id = esp_mqtt_client_subscribe(client, cmd_topic, 1);
      ESP_LOGI(TAG, "Subscribed to commands topic: %s (msg_id=%d)", cmd_topic, msg_id);
    }
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_PUBLISHED:
    ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;

  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    ESP_LOGI(TAG, "TOPIC: %.*s", event->topic_len, event->topic);
    ESP_LOGI(TAG, "DATA: %.*s", event->data_len, event->data);
    // Aqui você pode processar o comando JSON recebido do IoT Agent
    break;

  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
    {
      ESP_LOGE(TAG, "TLS/TCP Transport error:");
      ESP_LOGE(TAG, "  esp_tls_last_esp_err: 0x%x", event->error_handle->esp_tls_last_esp_err);
      ESP_LOGE(TAG, "  esp_tls_stack_err: 0x%x", event->error_handle->esp_tls_stack_err);
      ESP_LOGE(TAG, "  sock_errno: %d", event->error_handle->esp_transport_sock_errno);
    }
    break;

  default:
    ESP_LOGD(TAG, "Other event id: %ld", event_id);
    break;
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (!s_mqtt_started && event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    esp_mqtt_client_start(s_client);
    s_mqtt_started = true;
    return;
  }

  if (s_mqtt_started && event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    esp_mqtt_client_disconnect(s_client);
    s_mqtt_started = false;
    return;
  }
}