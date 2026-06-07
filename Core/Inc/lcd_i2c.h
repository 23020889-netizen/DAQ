#ifndef LCD_I2C_H_
#define LCD_I2C_H_

#include "stm32f4xx.h"

/* Địa chỉ I2C của module LCD PCF8574
 * Thường là 0x27 (Dịch trái 1 bit thành 0x4E để ghi)
 * Nếu không chạy, hãy thử địa chỉ 0x3F (Dịch trái thành 0x7E)
 */
#define LCD_I2C_ADDR    (0x27 << 1)

/* Định nghĩa các bit điều khiển của PCF8574 map sang LCD */
#define LCD_RS          (1U << 0)  // Bit 0: Register Select
#define LCD_RW          (1U << 1)  // Bit 1: Read/Write
#define LCD_EN          (1U << 2)  // Bit 2: Enable
#define LCD_BL          (1U << 3)  // Bit 3: Backlight (Bật/Tắt đèn nền)

/* Các hàm giao tiếp LCD */
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Char(char data);
void LCD_String(const char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);

#endif /* LCD_I2C_H_ */
