#include "stm32f4xx.h"
#include "signal_generator.h"
#include "lcd_i2c.h" // Thư viện LCD
#include "analog_input.h" // Thư viện cấu hình và lọc ADC
#include "uart.h"         // Thư viện UART
#include "button.h"       // Thư viện cấu hình và xử lý nút nhấn
#include "menu.h"         // Thư viện xử lý Giao diện LCD Menu
#include <stdio.h>        // Thư viện hỗ trợ sprintf

// --- CONFIGURATION CONSTANTS & VARIABLES ---
#define ADC_BUFFER_SIZE 1000
uint16_t adc_buffer[ADC_BUFFER_SIZE];

// --- FUNCTION PROTOTYPES ---

int main(void) {
    // 1. Cấu hình khối phát xung (Signal Generator) nằm trong file signal_generator.c
    SignalGenerator_Init();

    // 2. Cấu hình khối thu thập dữ liệu (DAQ) và Hiển thị
    AnalogInput_Init();
    USART2_Config();
    Button_Init();
    
    // Khởi tạo LCD (Bao gồm cả cấu hình I2C1 bên trong file lcd_i2c.c)
    LCD_Init();

    // Khởi tạo SysTick làm bộ đếm tự do (Free-running counter) ở 16MHz
    SysTick->LOAD = 0x00FFFFFF; 
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    // 3. Khởi tạo Giao diện Menu LCD (Sẽ tự hiển thị và thiết lập mặc định)
    Menu_Init();

    while (1) {
        // --- BLOCK CAPTURE & LỌC NHIỄU SỐ ---
        AnalogInput_CaptureAndFilter(adc_buffer, ADC_BUFFER_SIZE, current_sample_rate);

        // --- GỬI DỮ LIỆU QUA UART ---
        // Gửi Marker (0xFFFF) để Python nhận biết điểm bắt đầu khối dữ liệu
        uint16_t marker = 0xFFFF;
        USART2_SendBuffer((uint8_t*)&marker, 2);
        
        // Gửi tần số lấy mẫu hiện tại (4 bytes) báo cho Python
        USART2_SendBuffer((uint8_t*)&current_sample_rate, 4);
        
        // Gửi mảng dữ liệu ADC thô (2000 bytes)
        USART2_SendBuffer((uint8_t*)adc_buffer, ADC_BUFFER_SIZE * 2);
        
        // --- XỬ LÝ GIAO DIỆN MÀN HÌNH ---
        Menu_Tick();
    }
}

// --- CALLBACK XỬ LÝ NÚT NHẤN ---
// Hàm này ghi đè lên hàm weak trong button.c
void Button_Callback(uint8_t button_id) {
    Menu_ButtonHandler(button_id);
}
