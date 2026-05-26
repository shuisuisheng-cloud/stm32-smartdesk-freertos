#ifndef __APP_ESP8266_H__
#define __APP_ESP8266_H__

#include <stdint.h>

void AppESP8266_Init(void);
uint8_t AppESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeout);
uint8_t AppESP8266_TestAT(void);
uint8_t AppESP8266_SetStationMode(void);
uint8_t AppESP8266_ConnectWiFi(const char *ssid, const char *password);
uint8_t AppESP8266_IsWiFiConnected(void);
uint8_t AppESP8266_CheckWiFiStatus(void);
uint8_t AppESP8266_PingTest(void);
uint8_t AppESP8266_ConfigSNTP(void);
uint8_t AppESP8266_GetSNTPTime(char *out, uint16_t out_len);
uint8_t AppESP8266_PingHost(const char *host);
uint8_t AppESP8266_TestTcpConnect(const char *host, uint16_t port);
uint8_t AppESP8266_HTTPGet(const char *host, const char *path, uint8_t use_ssl, char *out, uint16_t out_len, uint32_t timeout);
uint16_t AppESP8266_ReadRawResponse(char *buf, uint16_t buf_size, uint32_t timeout_ms);

#endif /* __APP_ESP8266_H__ */
