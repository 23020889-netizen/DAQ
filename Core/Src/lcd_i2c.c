#include "lcd_i2c.h"

/* Hàm delay nội bộ */
// NOP sẽ mất 4 chu kỳ xung nhịp
static void LCD_Delay(uint32_t count) {
    while(count--) {
        __NOP();
    }
}

static void I2C1_WriteByte(uint8_t data) {
    uint32_t timeout;

    // 1. Chờ bus rảnh
    timeout = 100000;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --timeout);
    if (!timeout) return;

    // 2. Tạo START
    I2C1->CR1 |= I2C_CR1_START;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --timeout);
    if (!timeout) return;

    // 3. Gửi địa chỉ LCD (Chế độ Write)
    I2C1->DR = LCD_I2C_ADDR;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --timeout) {
        // Cờ AF (Acknowledge Failure) bật lên do sai địa chỉ hoặc không kết nối
        if (I2C1->SR1 & I2C_SR1_AF) {
            I2C1->SR1 &= ~I2C_SR1_AF; // Xóa cờ AF
            I2C1->CR1 |= I2C_CR1_STOP; // Tạo STOP giải phóng bus
            return;
        }
    }
    if (!timeout) return;

    (void)I2C1->SR1; // Xóa cờ ADDR
    (void)I2C1->SR2;

    // 4. Gửi dữ liệu vào thanh ghi dịch
    I2C1->DR = data;
    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_TXE) && --timeout);
    if (!timeout) return;

    timeout = 100000;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --timeout); 
    if (!timeout) return;

    // 5. Tạo tín hiệu STOP
    I2C1->CR1 |= I2C_CR1_STOP;
}

/* Hàm trung gian chia 1 byte thành 2 nửa (4 bit cao, 4 bit thấp) để gửi qua I2C */
static void LCD_Send(uint8_t data, uint8_t flags) {
	// flags: RS - LCD biết đây là lệnh hay chữ cái
    uint8_t up = (data & 0xF0) | flags | LCD_BL;
    uint8_t lo = ((data << 4) & 0xF0) | flags | LCD_BL;

    // Gửi 4 bit cao kèm xung chốt chân Enable kéo lên 1
    I2C1_WriteByte(up | LCD_EN);
    LCD_Delay(2000); // Delay 0.5ms
    // Kéo EN xuống để LCD đọc data
    I2C1_WriteByte(up & ~LCD_EN);
    LCD_Delay(2000);

    // Gửi 4 bit thấp kèm xung chốt chân Enable
    I2C1_WriteByte(lo | LCD_EN);
    LCD_Delay(2000);
    I2C1_WriteByte(lo & ~LCD_EN);
    LCD_Delay(2000);
}

void LCD_Command(uint8_t cmd) {
	// cmd: command lênhj điều khiển
    LCD_Send(cmd, 0); // RS = 0 khi gửi lệnh
}

void LCD_Char(char data) {
    LCD_Send(data, LCD_RS); // RS = 1 khi gửi ký tự hiển thị
}

static void I2C1_Config(void) {
    // 1. Bật clock cho GPIOB và I2C1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // 2. Cấu hình PB8 (SCL) và PB9 (SDA) làm Alternate Function, Open-Drain
    GPIOB->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->MODER |= ((2U << (8 * 2)) | (2U << (9 * 2)));      // AF mode
    GPIOB->OTYPER |= ((1U << 8) | (1U << 9));                 // Open-Drain
    GPIOB->OSPEEDR |= ((3U << (8 * 2)) | (3U << (9 * 2)));    // High speed
    GPIOB->PUPDR |= ((1U << (8 * 2)) | (1U << (9 * 2)));      // Pull-up

    // Chọn AF4 cho I2C1 trên PB8 và PB9 (Sử dụng AFR[1] vì pin >= 8)
    GPIOB->AFR[1] &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4)));
    GPIOB->AFR[1] |= ((4U << (0 * 4)) | (4U << (1 * 4)));     // AF4

    // 3. Cấu hình I2C1 (100 kHz Standard Mode)
    I2C1->CR1 |= I2C_CR1_SWRST; // Reset I2C
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = 16; // Clock hệ thống APB1 = 16 MHz
    I2C1->CCR = 80; // CCR = 16MHz / (2 * 100kHz) = 80
    I2C1->TRISE = 17; // Thời gian tăng sườn tối đa

    // Bật I2C1
    I2C1->CR1 |= I2C_CR1_PE;
}

void LCD_Init(void) {
    I2C1_Config();     // Khởi tạo I2C1 cho LCD
    LCD_Delay(500000); // Chờ LCD ổn định nguồn

    // Trình tự thiết lập chế độ 4-bit qua giao tiếp I2C PCF8574
    LCD_Command(0x33);
    LCD_Command(0x32); // Di chuyển về chế độ 4-bit
    LCD_Delay(10000);

    // Cấu hình màn hình LCD
    LCD_Command(0x28); // Kiểu hiển thị: 2 dòng, matrix 5x8 1 ô
    LCD_Command(0x0C); // Bật màn hình, tắt con trỏ rườm rà
    LCD_Command(0x06); // Tự động dịch con trỏ sang phải khi viết chữ
    LCD_Clear();       // Xóa màn hình ban đầu
}

void LCD_String(const char *str) {
    while (*str) {
        LCD_Char(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Command(address);
}

void LCD_Clear(void) {
    LCD_Command(0x01);
    LCD_Delay(200000); // Lệnh clear cần thời gian xử lý lâu hơn
}
