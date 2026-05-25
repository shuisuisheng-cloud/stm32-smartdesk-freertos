#ifndef __APP_ESP8266_H__
#define __APP_ESP8266_H__

#include <stdint.h>

void AppESP8266_Init(void);
uint8_t AppESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeout);
uint8_t AppESP8266_TestAT(void);
uint8_t AppESP8266_SetStationMode(void);
uint8_t AppESP8266_ConnectWiFi(const char *ssid, const char *password);
uint8_t AppESP8266_IsWiFiConnected(void);

#endif /* __APP_ESP8266_H__ */
