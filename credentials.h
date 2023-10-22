/******************************************************************************/
// Settings and credentials for the door opener

// SERIAL
#define BOUD 115200

// board configuration (Only for future enhancement purpose)
#define ESP01_ONE_RELAY_BOARD

#ifdef ESP01_ONE_RELAY_BOARD
  #define PUSH_BUTTON_ACTIVE
  #define RELAIS_PIN 0
  #define DEFAUL_DURATION_MS 200
#else 
  // place to define your own board configuration
#endif 


// WLAN
#define HOST_NAME "Door2"

#ifndef WIFI_MANAGER_ACTIVE
  #define WLAN_SSID   "NODES"
  #define WLAN_PASSWD "HappyNodes1234"
#endif


// Relay
#define DEFAULT_ONTIME = 300

// MQTT
#define MQTT_HOST_ADDRESS "192.168.10.102"
#define MQTT_HOST_PORT 1883
#define MQTT_USR "mqtt"
#define MQTT_PASSWD "Qwertz1234"
#define MQTT_TOPIC "/Haus/Garten/"


// webserver 
#define WEB_SERVER_PORT 80


/******************************************************************************/

