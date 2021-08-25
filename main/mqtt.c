#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "mqtt_client.h"
#include "mqtt.h"

#define TAG "MQTT"
#define MESSAGE_TOPIC "channels/1485222/publish/fields/field1"





const uint32_t WIFI_CONNECTED = BIT1;
const uint32_t MQTT_CONNECTED = BIT2;
const uint32_t MQTT_PUBLISHED = BIT3;
const uint32_t OUTM_UPDATED   = BIT4;

extern EventGroupHandle_t mq_eventgroup;

extern char *outbuff;

void mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    xEventGroupSetBits(mq_eventgroup,MQTT_CONNECTED);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  mqtt_event_handler_cb(event_data);
}



void OnConnected(void *para)
{
esp_mqtt_client_handle_t client; 
xEventGroupWaitBits(mq_eventgroup,WIFI_CONNECTED,pdFALSE,pdTRUE,portMAX_DELAY);
esp_mqtt_client_config_t mqttConfig = {
      
      .uri= "mqtt://broker.hivemq.com:1883"
      
      };
   client = NULL;

 client = esp_mqtt_client_init(&mqttConfig);
      esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
      esp_mqtt_client_start(client);

    while (true)
  {
   
    xEventGroupWaitBits(mq_eventgroup,WIFI_CONNECTED|MQTT_CONNECTED,pdFALSE,pdTRUE,portMAX_DELAY);
    xEventGroupWaitBits(mq_eventgroup,OUTM_UPDATED,pdTRUE,pdTRUE,portMAX_DELAY);
    
    esp_mqtt_client_publish(client,MESSAGE_TOPIC, outbuff, strlen(outbuff), 2,false);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
  }
}



void mqtt_main()
{
     xTaskCreate(OnConnected, "mqtt init", 1024 * 5, NULL, 5, NULL);
}




