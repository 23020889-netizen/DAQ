#include "stm32f4xx.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h> // Thêm thư viện này để dùng hàm atoi()

// --- KHAI BÁO BIẾN TOÀN CỤC ---
uint8_t rx_buffer[10]; // Tăng lên 10 để chứa đủ lệnh *p100#
uint8_t rx_data;
uint8_t rx_index = 0;
uint8_t btn_state_last = 1;
uint8_t btn_state_now = 1;
uint8_t btn2_state_last = 1;
uint8_t btn2_state_now = 1;

// --- KHAI BÁO HÀM ---
void GPIO_Config(void);
void USART2_Config(void);
void PWM_Config(void); // Hàm mới: Cấu hình PWM
void UART_SendString(char* str);
void Delay_ms(volatile uint32_t ms);
void ADC1_Config(void);

int main(void) {
   // 1. Cấu hình phần cứng
   GPIO_Config();
   USART2_Config();
   ADC1_Config();
   PWM_Config(); // Bật bộ phát PWM ở chân PA1

   char adc_str[15]; // Mảng chứa chuỗi ADC để gửi đi
   uint16_t adc_value = 0;

   while (1) {
       // ĐỌC NÚT NHẤN 1 (Chân PC13)
       btn_state_now = (GPIOC->IDR & (1 << 13)) ? 1 : 0;
       if (btn_state_now != btn_state_last) {
           Delay_ms(20); // Chống dội phím (Debounce)
           if (btn_state_now == 0) {
               UART_SendString("*B11#"); // Nút nhấn
           } else {
               UART_SendString("*B10#"); // Nút nhả
           }
           btn_state_last = btn_state_now;
       }

       // --- ĐỌC NÚT 2 (PA7) ---
       btn2_state_now = (GPIOA->IDR & (1 << 7)) ? 1 : 0;
       if (btn2_state_now != btn2_state_last) {
               Delay_ms(20);
               if (btn2_state_now == 0) UART_SendString("*B21#");
               else UART_SendString("*B20#");
               btn2_state_last = btn2_state_now;
       }

       ADC1->CR2 |= (1 << 30);            // Bắt đầu chuyển đổi (Set bit SWSTART)
       while (!(ADC1->SR & (1 << 1)));    // Chờ đến khi chuyển đổi xong (Bit EOC = 1)
       adc_value = ADC1->DR;              // Đọc kết quả (Tự động xóa cờ EOC)

       // Đóng gói thành chuỗi *Axxxx# và gửi đi
       sprintf(adc_str, "*A%d#", adc_value);
       UART_SendString(adc_str);

       Delay_ms(5); // Gửi 200 mẫu/giây
   }
}

// --- CẤU HÌNH PWM (PA1) ---
void PWM_Config(void) {
    // 1. Cấu hình chân PA1 sang Alternate Function (TIM2_CH2)
    RCC->AHB1ENR |= (1 << 0);
    GPIOA->MODER &= ~(3 << (1 * 2));
    GPIOA->MODER |=  (2 << (1 * 2));
    GPIOA->AFR[0] &= ~(0xF << (1 * 4));
    GPIOA->AFR[0] |=  (1 << (1 * 4));

    // 2. Cấu hình Timer 2 để xuất PWM
    RCC->APB1ENR |= (1 << 0);
    TIM2->PSC = 0;       // Clock 1MHz
    TIM2->ARR = 1000 - 1;     // Tần số PWM 16kHz
    TIM2->CCR2 = 0;           // Duty Cycle mặc định 0%

    TIM2->CCMR1 &= ~(0xFF << 8);
    TIM2->CCMR1 |= (6 << 12); // PWM Mode 1
    TIM2->CCER |= (1 << 4);   // Enable Kênh 2
    TIM2->CR1 |= 1;           // Chạy Timer
}

// --- CẤU HÌNH GPIO ---
void GPIO_Config(void) {
   RCC->AHB1ENR |= (1 << 0) | (1 << 2);
   GPIOA->MODER &= ~(3 << (5 * 2));
   GPIOA->MODER |=  (1 << (5 * 2));
   GPIOC->MODER &= ~(3 << (13 * 2));
   GPIOA->MODER &= ~((3 << (2 * 2)) | (3 << (3 * 2)));
   GPIOA->MODER |=  ((2 << (2 * 2)) | (2 << (3 * 2)));
   GPIOA->AFR[0] &= ~((0xF << (2 * 4)) | (0xF << (3 * 4)));
   GPIOA->AFR[0] |=  ((7 << (2 * 4)) | (7 << (3 * 4)));
   GPIOA->MODER &= ~(3 << (6 * 2));
   GPIOA->MODER |=  (1 << (6 * 2));
   GPIOA->MODER &= ~(3 << (7 * 2));
   GPIOA->PUPDR &= ~(3 << (7 * 2));
   GPIOA->PUPDR |=  (1 << (7 * 2));
}

// --- CẤU HÌNH USART2 ---
void USART2_Config(void) {
   RCC->APB1ENR |= (1 << 17);
   USART2->BRR = 0x008B;
   USART2->CR1 |= (1 << 13) | (1 << 3) | (1 << 2) | (1 << 5);
   NVIC_EnableIRQ(USART2_IRQn);
}

// --- CẤU HÌNH ADC ---
void ADC1_Config(void) {
   RCC->AHB1ENR |= (1 << 0);
   RCC->APB2ENR |= (1 << 8);
   GPIOA->MODER |= (3 << (0 * 2));
   ADC1->CR2 &= ~(1 << 1);
   ADC1->SQR3 &= ~(0x1F << 0);
   ADC1->CR2 |= (1 << 0);
}

// --- HÀM GỬI CHUỖI ---
void UART_SendString(char* str) {
   while (*str) {
       while (!(USART2->SR & (1 << 7)));
       USART2->DR = (*str & 0xFF);
       str++;
   }
}

// --- HÀM NGẮT NHẬN DỮ LIỆU ---
void USART2_IRQHandler(void) {
   if (USART2->SR & (1 << 5)) {
       rx_data = USART2->DR;
       rx_buffer[rx_index] = rx_data;

       if (rx_data == '#') {
           // Lệnh điều khiển LED
           if (rx_buffer[0] == '*' && rx_buffer[1] == 'L' && rx_buffer[2] == '1') {
               if (rx_buffer[3] == '1') GPIOA->BSRR = (1 << 5);
               else if (rx_buffer[3] == '0') GPIOA->BSRR = (1 << (5 + 16));
           }
           else if (rx_buffer[0] == '*' && rx_buffer[1] == 'L' && rx_buffer[2] == '2') {
           	   if (rx_buffer[3] == '1') GPIOA->BSRR = (1 << 6);
           	   else if (rx_buffer[3] == '0') GPIOA->BSRR = (1 << (6 + 16));
           }
           // --- LỆNH XUẤT PWM MỚI THÊM VÀO ---
           else if (rx_buffer[0] == '*' && rx_buffer[1] == 'P') {
               // Hàm atoi tự động dừng biến đổi khi gặp ký tự không phải số (dấu #)
               int duty = atoi((char*)&rx_buffer[2]);
               if (duty < 0) duty = 0;
               if (duty > 100) duty = 100;
               TIM2->CCR2 = duty * 10; // Thay đổi độ rộng xung trên PA1
           }

           rx_index = 0;
           memset(rx_buffer, 0, 10);
       } else {
           rx_index++;
           if (rx_index >= 10) rx_index = 0;
       }
   }
}

// --- HÀM DELAY ĐƠN GIẢN ---
void Delay_ms(volatile uint32_t ms) {
   ms *= 1600;
   while(ms--);
}
