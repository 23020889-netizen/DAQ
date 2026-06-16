#include "analog_input.h"

void AnalogInput_Init(void) {
    // Bật xung nhịp cho GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // 1. Cấu hình PA0 làm ngõ vào Analog (ADC1_IN0)
    GPIOA->MODER |= (3U << (0 * 2));     // Analog mode

    // 2. Cấu hình ADC1
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;  // Bật xung nhịp ADC1
    ADC1->CR2 &= ~ADC_CR2_CONT;          // Chuyển đổi đơn lẻ (Single conversion mode)
    ADC1->SQR3 &= ~ADC_SQR3_SQ1;         // Chọn kênh 0 (PA0) cho lần chuyển đổi đầu tiên trong sequence
    ADC1->CR2 |= ADC_CR2_ADON;           // Bật bộ ADC1
}

void AnalogInput_CaptureAndFilter(uint16_t* buffer, uint16_t buffer_size, uint32_t sample_rate) {
    // Dùng con trỏ trực tiếp để ép xung vòng lặp
    volatile uint32_t *adc_cr2 = &ADC1->CR2;
    volatile uint32_t *adc_sr  = &ADC1->SR;
    volatile uint32_t *adc_dr  = &ADC1->DR;
    volatile uint32_t *sys_val = &SysTick->VAL;
    
    uint32_t wait_cycles = 16000000 / sample_rate;
    
    // Mảng lưu lịch sử các mẫu để lấy trung bình (Moving Average Filter)
    uint16_t filter_history[FILTER_WINDOW_SIZE] = {0};
    uint8_t filter_index = 0;
    uint32_t filter_sum = 0;
    
    // Lấy một số mẫu ban đầu để lấp đầy mảng history tránh giá trị mốc 0 lúc đầu
    for(int i = 0; i < FILTER_WINDOW_SIZE; i++) {
        *adc_cr2 |= ADC_CR2_SWSTART;
        while (!(*adc_sr & ADC_SR_EOC));
        filter_history[i] = *adc_dr;
        filter_sum += filter_history[i];
    }

    uint32_t target_time = *sys_val;

    // Vòng lặp lấy mẫu chính
    for (int i = 0; i < buffer_size; i++) {
        *adc_cr2 |= ADC_CR2_SWSTART; // Bắt đầu chuyển đổi
        while (!(*adc_sr & ADC_SR_EOC)); // Chờ chuyển đổi xong
        uint16_t new_sample = *adc_dr; // Đọc giá trị
        
        // --- BỘ LỌC SỐ (Moving Average) tích hợp NHẬN DIỆN SƯỜN XUNG (Edge Detection) ---
        uint16_t current_avg = filter_sum / FILTER_WINDOW_SIZE;
        int32_t diff = (int32_t)new_sample - (int32_t)current_avg;
        
        // Nếu độ lệch giữa mẫu mới và trung bình lớn hơn 600 (Khoảng 0.5V), đây là sườn xung vuông
        if (diff > 600 || diff < -600) {
            // Reset toàn bộ bộ lọc về giá trị mới để tạo ra cạnh vuông đứng, triệt tiêu đường chéo
            for(int j = 0; j < FILTER_WINDOW_SIZE; j++) {
                filter_history[j] = new_sample;
            }
            filter_sum = new_sample * FILTER_WINDOW_SIZE;
        } else {
            // Nếu là sóng Sine, Triangle hoặc nhiễu nhỏ, thực hiện lọc trung bình trượt bình thường
            filter_sum -= filter_history[filter_index];
            filter_history[filter_index] = new_sample;
            filter_sum += new_sample;
            
            filter_index++;
            if (filter_index >= FILTER_WINDOW_SIZE) {
                filter_index = 0;
            }
        }
        
        // Tính giá trị trung bình và lưu vào mảng xuất
        buffer[i] = filter_sum / FILTER_WINDOW_SIZE;
        
        // Khóa cứng thời gian bằng SysTick để đồng bộ tần số lấy mẫu (Loại bỏ overhead vòng lặp)
        target_time = (target_time - wait_cycles) & 0x00FFFFFF;
        while (((target_time - *sys_val) & 0x00FFFFFF) > wait_cycles) {
             // Đợi cho đến khi sys_val vượt qua target_time
             // Vì đếm lùi nên khi sys_val < target_time, (target_time - sys_val) sẽ là số dương nhỏ (< wait_cycles)
             // Nhưng nếu sys_val chưa qua target_time, sys_val > target_time, hiệu số sẽ bị underflow thành số rất lớn
             // Do đó điều kiện thoát là khi hiệu số < wait_cycles
             if (((target_time - *sys_val) & 0x00FFFFFF) < (0x00FFFFFF / 2)) break;
        }
    }
}

// Hàm đọc điện áp DC ổn định cho Voltmeter
float AnalogInput_ReadDC(void) {
    volatile uint32_t *adc_cr2 = &ADC1->CR2;
    volatile uint32_t *adc_sr  = &ADC1->SR;
    volatile uint32_t *adc_dr  = &ADC1->DR;
    
    uint32_t sum = 0;
    // Lấy trung bình 16 mẫu liên tiếp để ra mức DC ổn định
    for(int i = 0; i < 16; i++) {
        *adc_cr2 |= ADC_CR2_SWSTART; 
        while (!(*adc_sr & ADC_SR_EOC)); 
        sum += *adc_dr;
    }
    
    float avg = (float)sum / 16.0f;
    return avg * (3.3f / 4095.0f); // Chuẩn hóa về thang 0-3.3V
}
