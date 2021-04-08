// from https://github.com/luc-github/ESP3D/files/3687027/config.txt
#if ESP32
//#include "FS.h"
//#include "SPIFFS.h"
//using fs::File;
#define WIFI_NONE_SLEEP WIFI_PS_NONE
#define WIFI_LIGHT_SLEEP WIFI_PS_MIN_MODEM
#define WIFI_MODEM_SLEEP WIFI_PS_MAX_MODEM
#define WIFI_PHY_MODE_11B WIFI_PROTOCOL_11B
#define WIFI_PHY_MODE_11G WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G
#define WIFI_PHY_MODE_11N WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N
#define AUTH_OPEN WIFI_AUTH_OPEN
#define AUTH_WEP WIFI_AUTH_WEP
#define AUTH_WPA_PSK WIFI_AUTH_WPA_PSK
#define AUTH_WPA2_PSK WIFI_AUTH_WPA2_PSK
#define AUTH_WPA_WPA2_PSK WIFI_AUTH_WPA_WPA2_PSK
#define ENC_TYPE_NONE AUTH_OPEN
#define FS_FILE File
#define FS_DIR File
#define SD_FILE_READ FILE_READ
#define SPIFFS_FILE_READ FILE_READ
#define SD_FILE_WRITE FILE_WRITE
#define SPIFFS_FILE_WRITE FILE_WRITE
#define WIFI_EVENT_STAMODE_CONNECTED SYSTEM_EVENT_STA_CONNECTED
#define WIFI_EVENT_STAMODE_DISCONNECTED SYSTEM_EVENT_STA_DISCONNECTED
#define WIFI_EVENT_STAMODE_GOT_IP SYSTEM_EVENT_STA_GOT_IP
#define WIFI_EVENT_SOFTAPMODE_STACONNECTED SYSTEM_EVENT_AP_STACONNECTED
// had this as 4 which was nothing on the lilygo eink display, but the esp32-cam's super bright light is on 4, and causes brownouts when powered by 3.3v.  change to 33 for dim red led on esp32-cam
//#if BOARD == esp32cam
//#define LED_BUILTIN 33
//#elif BOARD == lolin32
#define LED_BUILTIN 19
//#endif
#else
#define FS_DIR fs::Dir
#define FS_FILE fs::File
#define SD_FILE_READ FILE_READ
#define SPIFFS_FILE_READ "r"
#define SD_FILE_WRITE FILE_WRITE
#define SPIFFS_FILE_WRITE "w"
#endif
