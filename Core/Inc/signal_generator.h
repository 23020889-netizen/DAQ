#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include "stm32f4xx.h"
#include <stdint.h>

// --- WAVEFORM MODES ---
typedef enum {
    MODE_SINE = 1,
    MODE_TRIANGLE = 2,
    MODE_SQUARE = 3
} WaveformMode;

// --- EXPORTED VARIABLES ---
// Biến này được public để main.c có thể tự động đổi tốc độ lấy mẫu ADC (Auto Time/Div)
extern volatile uint32_t current_sample_rate;
extern volatile uint32_t current_frequency;
extern volatile WaveformMode current_mode;

// --- FUNCTION PROTOTYPES ---
void SignalGenerator_Init(void);
void Set_Frequency(uint32_t freq);
void Set_Mode(WaveformMode mode);

#endif // SIGNAL_GENERATOR_H
