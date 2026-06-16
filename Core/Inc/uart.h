#ifndef UART_H_
#define UART_H_

#include "stm32f4xx.h"

void USART2_Config(void);
void USART2_SendBuffer(uint8_t* data, uint16_t size);

#endif /* UART_H_ */
