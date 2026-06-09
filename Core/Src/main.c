#include "stm32f4xx.h"
#include "signal_generator.h"

// --- CONFIGURATION CONSTANTS & VARIABLES ---
#define ADC_BUFFER_SIZE 1000
uint16_t adc_buffer[ADC_BUFFER_SIZE];

// --- FUNCTION PROTOTYPES ---
void GPIO_DAQ_Config(void);
void ADC1_Config(void);
void USART2_Config(void);
void USART2_SendBuffer(uint8_t* data, uint16_t size);

int main(void) {
    // 1. Cấu hình khối phát xung (Signal Generator) nằm trong file signal_generator.c
    SignalGenerator_Init();

    // 2. Cấu hình khối thu thập dữ liệu (DAQ)
    GPIO_DAQ_Config();
    ADC1_Config();
    USART2_Config();

    // Khởi tạo SysTick làm bộ đếm tự do (Free-running counter) ở 16MHz
    SysTick->LOAD = 0x00FFFFFF; 
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    // 3. Áp dụng cài đặt phát xung mặc định (Sóng Sin, 500 Hz)
    Set_Mode(MODE_SINE);
    Set_Frequency(500);

    while (1) {
        // --- BLOCK CAPTURE ---
        // Dùng con trỏ trực tiếp để ép xung vòng lặp (vượt qua giới hạn tốc độ của -O0)
        volatile uint32_t *adc_cr2 = &ADC1->CR2;
        volatile uint32_t *adc_sr  = &ADC1->SR;
        volatile uint32_t *adc_dr  = &ADC1->DR;
        volatile uint32_t *sys_val = &SysTick->VAL;
        uint16_t *buf_ptr = adc_buffer;

        uint32_t wait_cycles = 16000000 / current_sample_rate;

        // Lấy mẫu 1000 điểm ở tần số linh hoạt dựa trên Auto Time/Div
        for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
            uint32_t start_time = *sys_val; 
            
            *adc_cr2 |= ADC_CR2_SWSTART; // Bắt đầu chuyển đổi
            while (!(*adc_sr & ADC_SR_EOC)); // Chờ chuyển đổi xong
            *buf_ptr++ = *adc_dr; // Đọc giá trị
            
            // Khóa cứng thời gian bằng SysTick
            while (((start_time - *sys_val) & 0x00FFFFFF) < wait_cycles);
        }

        // --- GỬI DỮ LIỆU QUA UART ---
        // Gửi Marker (0xFFFF) để Python nhận biết điểm bắt đầu khối dữ liệu
        uint16_t marker = 0xFFFF;
        USART2_SendBuffer((uint8_t*)&marker, 2);
        
        // Gửi tần số lấy mẫu hiện tại (4 bytes) báo cho Python
        USART2_SendBuffer((uint8_t*)&current_sample_rate, 4);
        
        // Gửi mảng dữ liệu ADC thô (2000 bytes)
        USART2_SendBuffer((uint8_t*)adc_buffer, ADC_BUFFER_SIZE * 2);
    }
}

// --- GPIO CONFIGURATION (Chỉ dành cho DAQ) ---
void GPIO_DAQ_Config(void) {
    // Bật xung nhịp cho GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 1. Cấu hình PA0 làm ngõ vào Analog (ADC1_IN0)
    GPIOA->MODER |= (3U << (0 * 2));     // Analog mode

    // 2. Cấu hình PA2 (USART2_TX) làm Alternate Function (AF7)
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |=  (2U << (2 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (2 * 4));
    GPIOA->AFR[0] |=  (7U << (2 * 4));   // AF7
}

// --- ADC1 CONFIGURATION ---
void ADC1_Config(void) {
    // Bật xung nhịp ADC1
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    // Chuyển đổi đơn lẻ (Single conversion mode)
    ADC1->CR2 &= ~ADC_CR2_CONT;
    // Chọn kênh 0 (PA0) cho lần chuyển đổi đầu tiên trong sequence
    ADC1->SQR3 &= ~ADC_SQR3_SQ1; 
    // Bật bộ ADC1
    ADC1->CR2 |= ADC_CR2_ADON;
}

// --- USART2 CONFIGURATION ---
void USART2_Config(void) {
    // Bật xung nhịp USART2
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    // Tốc độ 115200 baud với Clock nguồn 16 MHz
    USART2->BRR = 0x008B;
    // Bật module USART và khối truyền (Transmitter)
    USART2->CR1 |= USART_CR1_UE | USART_CR1_TE;
}

// --- HÀM GỬI UART ---
void USART2_SendBuffer(uint8_t* data, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        while (!(USART2->SR & USART_SR_TXE)); // Chờ bộ đệm phát trống
        USART2->DR = data[i];
    }
}
