#include "signal_generator.h"
#include <math.h>

// --- CONFIGURATION CONSTANTS & VARIABLES ---
#define LUT_SIZE 256
uint16_t sine_lut[LUT_SIZE];
uint16_t tri_lut[LUT_SIZE];

volatile uint32_t phase_acc = 0;
volatile uint32_t phase_step = 0;

// Các biến Public được khai báo extern ở file header
volatile WaveformMode current_mode = MODE_SINE;
volatile uint32_t current_frequency = 500;
volatile uint32_t current_sample_rate = 40000;

// --- PRIVATE FUNCTION PROTOTYPES ---
static void Generate_LUTs(void);
static void GPIO_Signal_Config(void);
static void TIM1_PWM_Config(void);
static void TIM3_Config(void);
static void TIM3_Config(void);
static void Apply_Signal_Settings(void);

// --- SIGNAL CONTROL ---
void Stop_Signal(void) {
    TIM1->CR1 &= ~TIM_CR1_CEN;  // Dừng đếm Timer 1
    TIM3->CR1 &= ~TIM_CR1_CEN;  // Dừng đếm Timer 3
    TIM1->CCR1 = 0;             // Đặt độ rộng xung = 0
    TIM1->EGR |= TIM_EGR_UG;    // Bắt buộc cập nhật ngay giá trị 0 vào Shadow Register để kéo output về 0V ngay lập tức
}

void Start_Signal(void) {
    Apply_Signal_Settings();
}

// --- INITIALIZATION ---
void SignalGenerator_Init(void) {
    Generate_LUTs();
    GPIO_Signal_Config();
    TIM1_PWM_Config();
    TIM3_Config();
}

// --- DYNAMIC LUT GENERATION ---
static void Generate_LUTs(void) {
    for (int i = 0; i < LUT_SIZE; i++) {
        float angle = (2.0f * 3.14159265f * (float)i) / (float)LUT_SIZE;
        sine_lut[i] = (uint16_t)(99.5f * sinf(angle) + 99.5f);

        if (i < 128) {
            tri_lut[i] = (uint16_t)((199.0f * (float)i) / 127.0f);
        } else {
            tri_lut[i] = (uint16_t)((199.0f * (float)(255 - i)) / 127.0f);
        }
    }
}

// --- PERIPHERAL SETTING APPLICATION ---
static void Apply_Signal_Settings(void) {
    if (current_mode == MODE_SINE || current_mode == MODE_TRIANGLE) {
        TIM3->CR1 &= ~TIM_CR1_CEN;
        TIM1->CR1 &= ~TIM_CR1_CEN;

        TIM1->PSC = 0;
        TIM1->ARR = 199;
        TIM1->CCR1 = 99;
        TIM1->EGR |= TIM_EGR_UG; 

        phase_step = (uint32_t)((float)current_frequency * 107374.1824f);

        TIM1->CR1 |= TIM_CR1_CEN;
        TIM3->CR1 |= TIM_CR1_CEN;
        TIM3->DIER |= TIM_DIER_UIE;
    } 
    else if (current_mode == MODE_SQUARE) {
        TIM3->DIER &= ~TIM_DIER_UIE;
        TIM3->CR1 &= ~TIM_CR1_CEN;
        TIM1->CR1 &= ~TIM_CR1_CEN;

        uint32_t timer_clock = 16000000;
        uint32_t psc = timer_clock / (65536 * current_frequency);
        uint32_t arr = 0;

        while (1) {
            uint32_t clk = timer_clock / (psc + 1);
            uint32_t temp_arr = clk / current_frequency;
            if (temp_arr <= 65536) {
                arr = temp_arr - 1;
                break;
            }
            psc++;
        }

        TIM1->PSC = psc;
        TIM1->ARR = arr;
        TIM1->CCR1 = (arr + 1) / 2; 
        TIM1->EGR |= TIM_EGR_UG;
        TIM1->CR1 |= TIM_CR1_CEN;
    }
}

// --- API ĐIỀU KHIỂN TẦN SỐ VÀ CHẾ ĐỘ ---
void Set_Frequency(uint32_t freq) {
    if (current_mode == MODE_SINE || current_mode == MODE_TRIANGLE) {
        if (freq < 100) freq = 100;
        if (freq > 1000) freq = 1000;
    } else if (current_mode == MODE_SQUARE) {
        if (freq < 10) freq = 10;
        if (freq > 10000) freq = 10000;
    }
    current_frequency = freq;
    
    if (current_mode == MODE_SINE || current_mode == MODE_TRIANGLE) {
        // Khóa cứng tần số lấy mẫu DAQ ở mức cao nhất 40kHz 
        // để tạo ra số điểm ảnh dày đặc (40-400 điểm/chu kỳ), giúp sóng mượt mà, triệt tiêu gãy khúc.
        current_sample_rate = 40000; 
    } 
    else {
        // Xung vuông tần số có thể xuống 10Hz nên cần tự động zoom out (giảm sample rate)
        if (current_frequency <= 50) {
            current_sample_rate = 4000;  
        } else if (current_frequency <= 500) {
            current_sample_rate = 10000; 
        } else {
            current_sample_rate = 40000; 
        }
    }

    Apply_Signal_Settings();
}

void Set_Mode(WaveformMode mode) {
    current_mode = mode;
    Set_Frequency(current_frequency);
}

// --- GPIO CONFIGURATION (Chỉ dành cho bộ phát xung) ---
static void GPIO_Signal_Config(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    
    // Cấu hình PA8 (TIM1_CH1) là Alternate Function (AF1), High Speed, No Pull
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (2U << (8 * 2));    
    GPIOA->OSPEEDR &= ~(3U << (8 * 2));
    GPIOA->OSPEEDR |=  (3U << (8 * 2));  
    GPIOA->PUPDR &= ~(3U << (8 * 2));    
    GPIOA->AFR[1] &= ~(0xFU << (0 * 4)); 
    GPIOA->AFR[1] |=  (1U << (0 * 4));   
}

// --- TIMER 1 PWM CONFIGURATION ---
static void TIM1_PWM_Config(void) {
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->PSC = 0;       
    TIM1->ARR = 199;     
    TIM1->CCR1 = 99;     
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

// --- TIMER 3 SAMPLING CONFIGURATION ---
static void TIM3_Config(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC = 0;
    TIM3->ARR = 399; 
    TIM3->DIER |= TIM_DIER_UIE;

    NVIC->IP[TIM3_IRQn] = (1 << 4); 
    NVIC->ISER[TIM3_IRQn >> 5] = (1 << (TIM3_IRQn & 0x1F)); 

    TIM3->CR1 |= TIM_CR1_CEN;
}

// --- TIM3 INTERRUPT SERVICE ROUTINE ---
void TIM3_IRQHandler(void) {
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR = ~TIM_SR_UIF; 

        if (current_mode == MODE_SINE) {
            phase_acc += phase_step;
            uint32_t index = (phase_acc >> 24) & 0xFF; 
            TIM1->CCR1 = sine_lut[index];              
        } 
        else if (current_mode == MODE_TRIANGLE) {
            phase_acc += phase_step;
            uint32_t index = (phase_acc >> 24) & 0xFF;
            TIM1->CCR1 = tri_lut[index];
        }
    }
}
