#include "lcd_i2c.h"

/* Hàm delay nội bộ */
// NOP sẽ mất 4 chu kỳ xung nhịp
static void LCD_Delay(uint32_t count) {
    while(count--) {
        __NOP();
    }
}

/* Hàm truyền 1 byte dữ liệu thô qua I2C1 (Tầng thanh ghi) */
static void I2C1_WriteByte(uint8_t data) {
    // 1. Chờ bus rảnh
	// Busy = 0 => I2C trống
    while (I2C1->SR2 & I2C_SR2_BUSY);

    // 2. Tạo  START
    I2C1->CR1 |= I2C_CR1_START;
    // Chờ SB = 1 => tín hiệu START
    while (!(I2C1->SR1 & I2C_SR1_SB));

    // 3. Gửi địa chỉ LCD (Chế độ Write)
    // Địa chỉ I2C đã dịch sẵn 0x27 << 1
    // Bit cuối = 0 => I2C ghi vào LCD
    I2C1->DR = LCD_I2C_ADDR;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR1; // Xóa cờ ADDR
    (void)I2C1->SR2;

    // 4. Gửi dữ liệu vào thanh ghi dịch
    I2C1->DR = data;
    // Cờ TXE = 1, data đã được đẩy từ DR sang Shift reg
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    // Cờ BTF = 1, CPU chờ bytes dữ liệu cuối cùng truyền xong
    // Và Slave đã gửi xác nhận
    while (!(I2C1->SR1 & I2C_SR1_BTF)); // Chờ truyền xong hoàn toàn

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

void LCD_Init(void) {
    /* KHÔNG CẦN khởi tạo lại chân I2C vì driver mpu6050 đã bật I2C1 rồi */
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
