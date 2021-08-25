#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "freertos/event_groups.h"
#include "wifi_con.h"
#include "mqtt.h"


#define DEVICE_NAME "PRAV_DEVICE"

#define MAX_MESSAGE_SIZE 50

extern EventGroupHandle_t mq_eventgroup;
extern const uint32_t OUTM_UPDATED;
const uint32_t SSID_UPDATED= BIT5;
const uint32_t PWD_UPDATED= BIT6;
char *outbuff;



uint8_t ble_addr_type;
void ble_app_advertise(void);

struct wifi_credentials *wifi_cred;





uint16_t ssid_hdl;
uint16_t pwd_hdl;
uint16_t outm_hdl;




static int comcb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    
if(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
    {
     char* tempbuf;
     tempbuf=pvPortMalloc(sizeof(char)*60);
     
     if(attr_handle == ssid_hdl)
     {
        sprintf(tempbuf,"SSID:%s",wifi_cred->ussid);
     }
     else if(attr_handle == pwd_hdl)   
     {
        sprintf(tempbuf,"Password:%s",wifi_cred->upwd);
     }
          
     
     os_mbuf_append(ctxt->om, tempbuf, strlen(tempbuf));
     vPortFree(tempbuf);
    }
else
{
       
    if(attr_handle == ssid_hdl)
     {
        memcpy(wifi_cred->ussid, ctxt->om->om_data, ctxt->om->om_len);
        wifi_cred->ussid[(ctxt->om->om_len)]='\0';
        xEventGroupSetBits(mq_eventgroup,SSID_UPDATED);
     }
     else if(attr_handle == pwd_hdl)   
     {
        memcpy(wifi_cred->upwd, ctxt->om->om_data, ctxt->om->om_len);
        wifi_cred->upwd[(ctxt->om->om_len)]='\0';
        //printf("%s;%s\n",wifi_cred->ussid,wifi_cred->upwd);
        //printf("strlen->%d;%d\n",strlen((const char *)wifi_cred->ussid),strlen((const char *)wifi_cred->upwd));
        xEventGroupSetBits(mq_eventgroup,PWD_UPDATED);
        
     }
     else if(attr_handle == outm_hdl)
     {
        
        memcpy(outbuff, ctxt->om->om_data, ctxt->om->om_len);
        outbuff[(ctxt->om->om_len)]='\0';
        printf("outmsg:%s\n",outbuff);
        xEventGroupSetBits(mq_eventgroup,OUTM_UPDATED);
     }

}
     return 0;
}

static const struct ble_gatt_svc_def gat_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid =BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0x0A, 0x01),
     .characteristics = (struct ble_gatt_chr_def[]){
         
         {.uuid = BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0x0B, 0x01),
          .flags = BLE_GATT_CHR_F_WRITE|BLE_GATT_CHR_F_READ,
          .access_cb = comcb,
          .val_handle = &ssid_hdl,
        },
        
         {.uuid = BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0x0B, 0x02),
          .flags = BLE_GATT_CHR_F_WRITE|BLE_GATT_CHR_F_READ,
          .access_cb = comcb,
          .val_handle = &pwd_hdl,     
         },
        
         {.uuid = BLE_UUID128_DECLARE(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0x0B, 0x03),
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = comcb,
          .val_handle = &outm_hdl,     
         },

         {0}}
         
         },
         

    {0}};

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK" : "Failed");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_DISCONNECT");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_ADV_COMPLETE");
        ble_app_advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI("GAP", "BLE_GAP_EVENT_SUBSCRIBE");
        break;
    default:
        break;
    }
    return 0;
}

void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_DISC_LTD;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)ble_svc_gap_device_name();
    fields.name_len = strlen(ble_svc_gap_device_name());
    fields.name_is_complete = 1;

    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void host_task(void *param)
{
    nimble_port_run();
}




void app_main(void)
{
   
    
   // outmptr=pvPortMalloc(sizeof(struct message));
   // memset(outmptr,0x00,sizeof(struct message));
    
    wifi_cred=pvPortMalloc(sizeof(struct wifi_credentials));
    memset(wifi_cred,0x00,sizeof(struct wifi_credentials));
    
    outbuff=pvPortMalloc(MAX_MESSAGE_SIZE);
    memset(outbuff,0x00,MAX_MESSAGE_SIZE); 

    nvs_flash_init();

    esp_nimble_hci_and_controller_init();
    nimble_port_init();

    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gat_svcs);
    ble_gatts_add_svcs(gat_svcs);

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);
    
    wifi_main(wifi_cred);
    mqtt_main();
}
