#ifndef MENU_H_
#define MENU_H_

#include "stm32f4xx.h"

extern uint8_t save_mode;

void Menu_Init(void);
void Menu_UpdateDisplay(void);
void Menu_ButtonHandler(uint8_t btn);
void Menu_Tick(void);

#endif /* MENU_H_ */
