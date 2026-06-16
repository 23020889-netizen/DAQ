#ifndef DIGITAL_INPUT_H_
#define DIGITAL_INPUT_H_

#include "stm32f4xx.h"
#include <stdint.h>

void DigitalInput_Init(void);
void DigitalInput_Read(uint32_t *freq, uint32_t *duty_percent);

#endif /* DIGITAL_INPUT_H_ */
