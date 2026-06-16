#include "digital_output.h"

volatile uint32_t dig_out_freq = 100; // Mặc định 100 Hz
volatile uint32_t dig_out_duty = 50;  // Mặc định 50%

static void TIM4_PWM_Config(void) {
    // 1. Cấu hình Clock cho GPIOB và TIM4
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;

    // 2. Cấu hình PB6 (TIM4_CH1) là Alternate Function (AF2)
    GPIOB->MODER &= ~(3U << (6 * 2));
    GPIOB->MODER |=  (2U << (6 * 2));    // AF mode
    GPIOB->OSPEEDR |= (3U << (6 * 2));   // High speed
    GPIOB->PUPDR &= ~(3U << (6 * 2));    // No pull
    
    // Chọn AF2 cho TIM4 trên PB6
    GPIOB->AFR[0] &= ~(0xFU << (6 * 4)); 
    GPIOB->AFR[0] |=  (2U << (6 * 4));   

    // 3. Cấu hình Timer 4 PWM Mode 1
    TIM4->CR1 = 0; // Reset
    TIM4->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM4->CCMR1 |= (6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1, Preload enable
    TIM4->CCER |= TIM_CCER_CC1E; // Output enable
    TIM4->CR1 |= TIM_CR1_ARPE;   // Auto-reload preload enable
}

void DigitalOutput_Init(void) {
    TIM4_PWM_Config();
    Set_DigitalOutput(dig_out_freq, dig_out_duty);
    Stop_DigitalOutput(); // Bắt đầu ở trạng thái Setup (chưa phát)
}

void Set_DigitalOutput(uint32_t freq, uint32_t duty_percent) {
    if (freq < 10) freq = 10;
    if (freq > 1000) freq = 1000;
    if (duty_percent > 100) duty_percent = 100;

    dig_out_freq = freq;
    dig_out_duty = duty_percent;

    uint32_t timer_clock = 16000000;
    uint32_t psc = timer_clock / (65536 * freq);
    uint32_t arr = 0;

    // Tìm giá trị bộ chia sao cho ARR nằm trong giới hạn 16-bit
    while (1) {
        uint32_t clk = timer_clock / (psc + 1);
        uint32_t temp_arr = clk / freq;
        if (temp_arr <= 65536 && temp_arr > 0) {
            arr = temp_arr - 1;
            break;
        }
        psc++;
    }

    TIM4->PSC = psc;
    TIM4->ARR = arr;
    
    // Tính toán duty cycle
    uint32_t ccr1 = ((arr + 1) * duty_percent) / 100;
    TIM4->CCR1 = ccr1;
    
    TIM4->EGR |= TIM_EGR_UG; // Cập nhật ngay các thanh ghi bóng
}

void Start_DigitalOutput(void) {
    Set_DigitalOutput(dig_out_freq, dig_out_duty); // Nạp lại thông số chuẩn
    TIM4->CR1 |= TIM_CR1_CEN; // Bắt đầu đếm
}

void Stop_DigitalOutput(void) {
    TIM4->CR1 &= ~TIM_CR1_CEN; // Dừng Timer
    TIM4->CCR1 = 0;            // Kéo PWM về 0V
    TIM4->EGR |= TIM_EGR_UG;   // Bắt buộc đẩy CCR1=0 xuống Shadow Register ngay lập tức
}
