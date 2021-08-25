#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mdns.h"
#include "wifi_con.h"
#include "string.h"
char *TAG = "CONNECTION";

EventGroupHandle_t mq_eventgroup;
extern const uint32_t WIFI_CONNECTED;
extern const uint32_t SSID_UPDATED;
extern const uint32_t PWD_UPDATED;

static void initialise_mdns(void)
{

    xEventGroupWaitBits(mq_eventgroup,WIFI_CONNECTED,pdFALSE,pdTRUE,portMAX_DELAY);
    char* hostname = "test.local";
    ESP_ERROR_CHECK( mdns_init() );
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    ESP_ERROR_CHECK( mdns_instance_name_set("mymdnsservice") );

    mdns_txt_item_t serviceTxtData[1] = {
        {"board","esp32"},
     };
   
    ESP_ERROR_CHECK( mdns_service_add("myservice", "_http", "_tcp", 80, serviceTxtData,1) );
   
}

static void event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  switch (event_id)
  {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect(); 
    ESP_LOGI(TAG,"STA_START_EVENT...\n");
    break; 

  case SYSTEM_EVENT_STA_CONNECTED:
    ESP_LOGI(TAG,"connected\n");
    break;

  case IP_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG,"got ip\n");
    xEventGroupSetBits(mq_eventgroup,WIFI_CONNECTED);
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG,"disconnected\n");
    xEventGroupClearBits(mq_eventgroup,WIFI_CONNECTED);
    break;

  default:
    break;
  }
 
}

void wifiInit()
{
 
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,event_handler,NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event_handler,NULL));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

}


void wifi_connect(struct wifi_credentials *wifi_cred)
{

      
      ESP_LOGI(TAG,"Connecting...\n");
      wifi_config_t wifi_config =
      {
          .sta = {
              .ssid = "",
              .password = ""
              }};
      memcpy(wifi_config.sta.ssid,wifi_cred->ussid,32);
      memcpy(wifi_config.sta.password,wifi_cred->upwd,50);
      //printf("LOG:%s--%s\n",wifi_config.sta.ssid,wifi_config.sta.password);
      ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
      ESP_ERROR_CHECK(esp_wifi_start());
      
  
}
void OnInit(void *params)
{

while(true)
{
  
  xEventGroupWaitBits(mq_eventgroup,SSID_UPDATED|PWD_UPDATED,pdTRUE,pdTRUE,portMAX_DELAY);
  wifi_connect((struct wifi_credentials *)params);
  initialise_mdns();
  vTaskDelete(NULL);
}

}
void wifi_main(struct wifi_credentials *wifi_cred)
{
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  mq_eventgroup = xEventGroupCreate();
  wifiInit();
  xTaskCreate(OnInit,"Wifi Connect Task",1024*4,wifi_cred,5,NULL );
  
  
    
}