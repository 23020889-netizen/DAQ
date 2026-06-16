#include "uart.h"

// --- USART2 CONFIGURATION ---
void USART2_Config(void) {
    // Bật xung nhịp cho GPIOA (Dùng cho PA2)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Cấu hình PA2 (USART2_TX) làm Alternate Function (AF7)
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |=  (2U << (2 * 2));
    GPIOA->AFR[0] &= ~(0xFU << (2 * 4));
    GPIOA->AFR[0] |=  (7U << (2 * 4));   // AF7

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
