#ifndef BUTTON_H_
#define BUTTON_H_

#include "stm32f4xx.h"

// Hàm khởi tạo GPIO và ngắt ngoài (EXTI) cho 4 nút nhấn PC0-PC3
void Button_Init(void);

// Hàm Callback được gọi khi nút nhấn hợp lệ (đã qua bộ lọc chống rung)
// button_id: 0 tương ứng PC0, 1 tương ứng PC1, 2 tương ứng PC2, 3 tương ứng PC3
// Hàm này được khai báo weak trong button.c, bạn có thể viết lại nội dung ở main.c
void Button_Callback(uint8_t button_id);

#endif /* BUTTON_H_ */
