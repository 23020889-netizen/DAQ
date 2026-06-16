#include "button.h"



void Button_Init(void) {
    // 1. Bật xung nhịp cho cổng GPIOC và SYSCFG
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // 2. Cấu hình PC0, PC1, PC2, PC3 làm Input
    GPIOC->MODER &= ~((3U << (0 * 2)) | (3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (3 * 2)));
    
    // Cấu hình điện trở kéo lên (Pull-up) cho 4 nút nhấn
    GPIOC->PUPDR &= ~((3U << (0 * 2)) | (3U << (1 * 2)) | (3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOC->PUPDR |= ((1U << (0 * 2)) | (1U << (1 * 2)) | (1U << (2 * 2)) | (1U << (3 * 2)));

    // 3. Liên kết ngắt ngoài EXTI Line 0-3 với cổng C (PC0-PC3)
    SYSCFG->EXTICR[0] &= ~0xFFFF; // Xóa cấu hình cũ của EXTI0-3
    SYSCFG->EXTICR[0] |= 0x2222;  // Cấu hình 0010 (Port C) cho EXTI0, EXTI1, EXTI2, EXTI3

    // Kích hoạt ngắt trên sườn xuống (Falling Edge - nút được nhấn nối GND)
    EXTI->FTSR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
    // Vô hiệu hóa sườn lên
    EXTI->RTSR &= ~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3));
    
    // Bỏ mặt nạ ngắt (Unmask) cho EXTI 0-3
    EXTI->IMR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);

    // 4. Cấu hình NVIC (Độ ưu tiên ngắt ngẫu nhiên chọn là 5)
    NVIC_SetPriority(EXTI0_IRQn, 5);
    NVIC_SetPriority(EXTI1_IRQn, 5);
    NVIC_SetPriority(EXTI2_IRQn, 5);
    NVIC_SetPriority(EXTI3_IRQn, 5);
    
    // Cho phép ngắt hoạt động
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);
}

// Hàm Weak Callback: Cho phép ghi đè ở main.c mà không cần sửa file này
__attribute__((weak)) void Button_Callback(uint8_t button_id) {
    // Để trống
}

// Hàm delay ngắn dùng cho chống rung (Khoảng 20ms)
static void Debounce_Delay(void) {
    for(volatile uint32_t i = 0; i < 32000; i++);
}

// --- CÁC TRÌNH PHỤC VỤ NGẮT ---
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1 << 0)) {
        EXTI->PR = (1 << 0); // Xóa cờ ngắt
        
        Debounce_Delay(); // Chờ qua khoảng thời gian dội phím
        
        if ((GPIOC->IDR & (1 << 0)) == 0) { // Nếu vẫn thực sự đang được nhấn
            Button_Callback(0);
        }
        
        EXTI->PR = (1 << 0); // Xóa cờ ngắt lần nữa để loại bỏ các xung nhiễu sinh ra trong lúc delay
    }
}

void EXTI1_IRQHandler(void) {
    if (EXTI->PR & (1 << 1)) {
        EXTI->PR = (1 << 1); 
        
        Debounce_Delay();
        
        if ((GPIOC->IDR & (1 << 1)) == 0) {
            Button_Callback(1);
        }
        
        EXTI->PR = (1 << 1); 
    }
}

void EXTI2_IRQHandler(void) {
    if (EXTI->PR & (1 << 2)) {
        EXTI->PR = (1 << 2); 
        
        Debounce_Delay();
        
        if ((GPIOC->IDR & (1 << 2)) == 0) {
            Button_Callback(2);
        }
        
        EXTI->PR = (1 << 2); 
    }
}

void EXTI3_IRQHandler(void) {
    if (EXTI->PR & (1 << 3)) {
        EXTI->PR = (1 << 3); 
        
        Debounce_Delay();
        
        if ((GPIOC->IDR & (1 << 3)) == 0) {
            Button_Callback(3);
        }
        
        EXTI->PR = (1 << 3); 
    }
}
