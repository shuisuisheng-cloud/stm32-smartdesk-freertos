#ifndef __APP_UI_H__
#define __APP_UI_H__

#include <stdint.h>

#define APP_UI_PAGE_COUNT 5U

void AppUI_Init(void);
void AppUI_ShowPage(uint8_t page);

#endif /* __APP_UI_H__ */
