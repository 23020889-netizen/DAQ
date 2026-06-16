#include "digital_input.h"

// Sử dụng chân PA15 làm ngõ vào đo đạc (TIM2_CH1)
static void PA15_TIM2_CH1_Init(void) {
    // 1. Cấu hình Clock cho GPIOA và TIM2
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Cấu hình PA15 làm Alternate Function
    GPIOA->MODER &= ~(3U << (15 * 2));
    GPIOA->MODER |=  (2U << (15 * 2)); // AF mode
    
    // Thiết lập Pull-up cho chân PA15 để tránh thả nổi khi không có tín hiệu
    GPIOA->PUPDR &= ~(3U << (15 * 2));
    GPIOA->PUPDR |=  (1U << (15 * 2)); // Pull-up

    // Chọn AF1 (TIM2) cho PA15 (AFR[1] vì chân 15)
    GPIOA->AFR[1] &= ~(0xFU << ((15 - 8) * 4));
    GPIOA->AFR[1] |=  (1U << ((15 - 8) * 4)); 
}

static void TIM2_PWM_Input_Init(void) {
    TIM2->CR1 = 0; // Tắt timer để cấu hình
    
    // Clock hệ thống là 16MHz. Timer 2 là 32-bit nên không cần chia
    TIM2->PSC = 0; 
    TIM2->ARR = 0xFFFFFFFF; // Max 32-bit

    // Cấu hình PWM Input Mode:
    // CC1S = 01: CH1 nối với TI1
    // CC2S = 10: CH2 nối với TI1 (Cùng 1 chân vật lý nhưng đi vào 2 kênh)
    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
    TIM2->CCMR1 |= (1U << TIM_CCMR1_CC1S_Pos) | (2U << TIM_CCMR1_CC2S_Pos);
    
    // Cấu hình Cạnh bắt tín hiệu:
    // CH1 bắt sườn LÊN (Period)
    // CH2 bắt sườn XUỐNG (Pulse width)
    TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC2P | TIM_CCER_CC2NP);
    TIM2->CCER |= TIM_CCER_CC2P; // CH2 bắt sườn xuống (Đảo cực)
    
    // Cấu hình chế độ Slave: Reset Mode
    // TS = 101: TI1FP1
    // SMS = 100: Reset mode (Mỗi lần có sườn lên ở TI1, CNT sẽ tự reset về 0)
    TIM2->SMCR &= ~(TIM_SMCR_TS | TIM_SMCR_SMS);
    TIM2->SMCR |= (5U << TIM_SMCR_TS_Pos) | (4U << TIM_SMCR_SMS_Pos);

    // Kích hoạt Capture cho cả CH1 và CH2
    TIM2->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
    
    // Bật timer
    TIM2->CR1 |= TIM_CR1_CEN;
}

void DigitalInput_Init(void) {
    PA15_TIM2_CH1_Init();
    TIM2_PWM_Input_Init();
}

void DigitalInput_Read(uint32_t *freq, uint32_t *duty_percent) {
    uint32_t period = TIM2->CCR1;
    uint32_t pulse  = TIM2->CCR2;
    uint32_t cnt    = TIM2->CNT;
    
    // Xử lý Timeout: Nếu hơn 1 giây không có sườn mới, coi như mất tín hiệu (0 Hz)
    // Clock = 16MHz, nên 1 giây = 16.000.000 ticks.
    if (cnt > 16000000 || period == 0) {
        *freq = 0;
        *duty_percent = 0;
        return;
    }
    
    // Tần số = Tần số Clock Timer / Số tick của 1 chu kỳ (làm tròn)
    *freq = (16000000 + (period / 2)) / period;
    
    // Duty cycle = Thời gian mức cao / Tổng thời gian chu kỳ
    // Nhân 100 và cộng period/2 để làm tròn (round to nearest) thay vì cắt bỏ (truncate)
    *duty_percent = (uint32_t)(((uint64_t)pulse * 100ULL + (period / 2)) / (uint64_t)period);
}
