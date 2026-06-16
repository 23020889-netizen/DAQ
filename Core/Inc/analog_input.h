#ifndef ANALOG_INPUT_H_
#define ANALOG_INPUT_H_

#include "stm32f4xx.h"

// Số lượng mẫu trung bình cho bộ lọc Moving Average (Nên là lũy thừa của 2: 2, 4, 8... để tối ưu tốc độ)
#define FILTER_WINDOW_SIZE 4

void AnalogInput_Init(void);
void AnalogInput_CaptureAndFilter(uint16_t* buffer, uint16_t buffer_size, uint32_t sample_rate);
float AnalogInput_ReadDC(void);

#endif /* ANALOG_INPUT_H_ */
