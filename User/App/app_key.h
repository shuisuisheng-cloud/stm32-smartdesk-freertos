#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include <stdint.h>

#define APP_KEY_EVENT_NONE   0U
#define APP_KEY_EVENT_SHORT  1U
#define APP_KEY_EVENT_LONG   2U

void AppKey_Init(void);
uint8_t AppKey_Scan(void);

#endif /* __APP_KEY_H__ */
