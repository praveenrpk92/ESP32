# mDNS in Local Network & MQTT

Device : ESP WROOM 32

Basic Flow Of Program:
1) Starts the BLE Server 
2) Waits for WiFi SSID & Password to be input in respective UUIDs.
3) Starts WiFi connection.
4) Starts mDNS service.
5) Connects to MQTT broker on WiFi connect & awaits messages sent from BLE client to be published.


Usage: 

1) BLE
   Device Name         : PRAV_DEVICE
   Service UUID        : 010addcc-bbaa-9988-7766-554433221100

   SSID UUID           : 010bddcc-bbaa-9988-7766-554433221100  
   Password UUID       : 020bddcc-bbaa-9988-7766-554433221100  
   MQTT Publish UUID   : 030bddcc-bbaa-9988-7766-554433221100

2) mDNS
   On Connection to WiFi, mDNS service is started with,
    Instance Name: myservice
    Type         : _http._tcp
    Domain       : local
    Port         : 80 

3) MQTT Broker URI    : broker.hivemq.com:1883 
               Topic  : channels/1485222/publish/fields/field1

Firmware can be flashed using esptool

esptool.py --port COM4 write_flash -fm dio -fs 32m 0x00000 BLE-MULTICAST-ESP32.bin
              

