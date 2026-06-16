#ifndef DIGITAL_OUTPUT_H_
#define DIGITAL_OUTPUT_H_

#include "stm32f4xx.h"
#include <stdint.h>

// Các biến trạng thái của khối Digital Output (Menu có thể đọc để hiển thị)
extern volatile uint32_t dig_out_freq;
extern volatile uint32_t dig_out_duty;

void DigitalOutput_Init(void);
void Set_DigitalOutput(uint32_t freq, uint32_t duty_percent);
void Start_DigitalOutput(void);
void Stop_DigitalOutput(void);

#endif /* DIGITAL_OUTPUT_H_ */
