#include "menu.h"
#include "lcd_i2c.h"
#include "signal_generator.h"
#include "analog_input.h"
#include "digital_output.h"
#include "digital_input.h"
#include <stdio.h>

typedef enum {
    MENU_STATE_SETUP = 0,
    MENU_STATE_OK = 1
} MenuState;

static uint8_t current_menu = 0; // Mode 0
static MenuState sig_gen_state = MENU_STATE_SETUP;
static MenuState volt_state = MENU_STATE_SETUP;
static MenuState dig_out_state = MENU_STATE_SETUP;
static MenuState dig_in_state = MENU_STATE_SETUP;

uint8_t save_mode = 0; // Cờ lưu dữ liệu (0: OFF, 1: ON)

// Biến cờ lưu sự kiện nút nhấn từ ngắt để xử lý ở Main Loop
static volatile uint8_t pending_button = 0xFF;

void Menu_Init(void) {
    current_menu = 0;
    sig_gen_state = MENU_STATE_SETUP;
    volt_state = MENU_STATE_SETUP;
    dig_out_state = MENU_STATE_SETUP;
    dig_in_state = MENU_STATE_SETUP;
    
    save_mode = 0;
    
    Set_Mode(MODE_SINE);
    Set_Frequency(500);
    Stop_Signal(); 
    
    DigitalOutput_Init(); // Setup Digital Output
    DigitalInput_Init();  // Setup Digital Input
    
    Menu_UpdateDisplay();
}

void Menu_UpdateDisplay(void) {
    char buf[17];
    LCD_SetCursor(0, 0);
    
    if (current_menu == 0) { // Mode 0: Signal Generator
        if (sig_gen_state == MENU_STATE_SETUP) {
            LCD_String("Sig Gen [Setup] ");
        } else {
            LCD_String("Sig Gen [OK]    ");
        }

        LCD_SetCursor(1, 0);
        const char* mode_str = "";
        if (current_mode == MODE_SINE) mode_str = "SIN";
        else if (current_mode == MODE_TRIANGLE) mode_str = "TRI";
        else if (current_mode == MODE_SQUARE) mode_str = "SQU";

        // Format: S:SIN F:1000Hz (Tối đa 16 ký tự)
        sprintf(buf, "S:%-3s F:%-5luHz ", mode_str, current_frequency);
        buf[16] = '\0';
        LCD_String(buf);
    } 
    else if (current_menu == 1) { // Mode 1: Voltmeter
        if (volt_state == MENU_STATE_SETUP) {
            LCD_String("Voltmeter[Setup]");
            LCD_SetCursor(1, 0);
            LCD_String("Volt: --- V     "); 
        } else {
            if (save_mode) {
                LCD_String("Voltmeter[OK][S]");
            } else {
                LCD_String("Voltmeter[OK][ ]");
            }
            LCD_SetCursor(1, 0);
            LCD_String("Volt: Meas...   ");
        }
    }
    else if (current_menu == 2) { // Mode 2: Digital Output
        if (dig_out_state == MENU_STATE_SETUP) {
            LCD_String("Dig_Out  [Setup]"); // 16 chars
        } else {
            LCD_String("Dig_Out  [OK]   "); // 16 chars
        }
        LCD_SetCursor(1, 0);
        sprintf(buf, "F:%-4luHz D:%-3lu%% ", dig_out_freq, dig_out_duty);
        buf[16] = '\0';
        LCD_String(buf);
    }
    else if (current_menu == 3) { // Mode 3: Digital Input
        if (dig_in_state == MENU_STATE_SETUP) {
            LCD_String("Dig_In   [Setup]"); // 16 chars
            LCD_SetCursor(1, 0);
            LCD_String("Wait for OK...  ");
        } else {
            LCD_String("Dig_In   [OK]   "); // 16 chars
            LCD_SetCursor(1, 0);
            LCD_String("Measuring...    ");
        }
    }
}

// Hàm này được gọi liên tục trong vòng lặp main để update màn hình nếu cần
void Menu_Tick(void) {
    // 1. Xử lý sự kiện nút nhấn (Đưa ra khỏi ngắt để tránh đụng độ I2C)
    if (pending_button != 0xFF) {
        uint8_t btn = pending_button;
        pending_button = 0xFF; // Xóa cờ
        
        if (btn == 0) { // PC0: Switch Menu Mode
            current_menu = (current_menu + 1) % 4; // 4 modes
            Menu_UpdateDisplay();
        }
        else if (btn == 1) { // PC1: Switch Signal / Change Freq D_Out / Toggle Save
            if (current_menu == 0 && sig_gen_state == MENU_STATE_SETUP) {
                WaveformMode next_mode = MODE_SINE;
                if (current_mode == MODE_SINE) next_mode = MODE_TRIANGLE;
                else if (current_mode == MODE_TRIANGLE) next_mode = MODE_SQUARE;
                else if (current_mode == MODE_SQUARE) next_mode = MODE_SINE;
                
                Set_Mode(next_mode);
                Stop_Signal(); 
                Menu_UpdateDisplay();
            }
            else if (current_menu == 1 && volt_state == MENU_STATE_OK) {
                save_mode = !save_mode;
                Menu_UpdateDisplay();
            }
            else if (current_menu == 2 && dig_out_state == MENU_STATE_SETUP) {
                uint32_t f = dig_out_freq;
                if (f < 100) f += 10;
                else f += 100;
                if (f > 1000) f = 10;
                
                Set_DigitalOutput(f, dig_out_duty);
                Stop_DigitalOutput();
                Menu_UpdateDisplay();
            }
        }
        else if (btn == 2) { // PC2: Change Freq SigGen / Change Duty D_Out
            if (current_menu == 0 && sig_gen_state == MENU_STATE_SETUP) {
                uint32_t new_freq = current_frequency;
                if (current_mode == MODE_SINE || current_mode == MODE_TRIANGLE) {
                    new_freq += 100;
                    if (new_freq > 1000) new_freq = 100;
                } else if (current_mode == MODE_SQUARE) {
                    if (new_freq < 100) {
                        new_freq += 10;
                    } else {
                        new_freq += 100;
                    }
                    if (new_freq > 1000) new_freq = 10;
                }
                Set_Frequency(new_freq);
                Stop_Signal(); 
                Menu_UpdateDisplay();
            }
            else if (current_menu == 2 && dig_out_state == MENU_STATE_SETUP) {
                uint32_t d = dig_out_duty;
                d += 10;
                if (d > 100) d = 0;
                Set_DigitalOutput(dig_out_freq, d);
                Stop_DigitalOutput();
                Menu_UpdateDisplay();
            }
        }
        else if (btn == 3) { // PC3: Enter (Toggle Setup/OK)
            if (current_menu == 0) { // Sig Gen
                if (sig_gen_state == MENU_STATE_SETUP) {
                    sig_gen_state = MENU_STATE_OK;
                    Start_Signal(); 
                } else {
                    sig_gen_state = MENU_STATE_SETUP;
                    Stop_Signal(); 
                }
            } 
            else if (current_menu == 1) { // Voltmeter
                if (volt_state == MENU_STATE_SETUP) {
                    volt_state = MENU_STATE_OK;
                } else {
                    volt_state = MENU_STATE_SETUP;
                    save_mode = 0; // Ngừng lưu khi thoát chế độ OK
                }
            }
            else if (current_menu == 2) { // Digital Output
                if (dig_out_state == MENU_STATE_SETUP) {
                    dig_out_state = MENU_STATE_OK;
                    Start_DigitalOutput();
                } else {
                    dig_out_state = MENU_STATE_SETUP;
                    Stop_DigitalOutput();
                }
            }
            else if (current_menu == 3) { // Digital Input
                if (dig_in_state == MENU_STATE_SETUP) dig_in_state = MENU_STATE_OK;
                else dig_in_state = MENU_STATE_SETUP;
            }
            Menu_UpdateDisplay();
        }
    }

    // 2. Cập nhật dữ liệu động trên LCD (Nếu đang ở chế độ cần đọc liên tục)
    if (current_menu == 1 && volt_state == MENU_STATE_OK) {
        float volt = AnalogInput_ReadDC();
        char buf[17];
        
        int v_int = (int)volt;
        int v_frac = (int)((volt - v_int) * 1000);
        if (v_frac < 0) v_frac = 0;
        
        sprintf(buf, "Volt: %d.%03d V   ", v_int, v_frac);
        LCD_SetCursor(1, 0);
        LCD_String(buf);
    }
    else if (current_menu == 3 && dig_in_state == MENU_STATE_OK) {
        uint32_t freq = 0;
        uint32_t duty = 0;
        DigitalInput_Read(&freq, &duty);
        
        char buf[17];
        if (freq == 0) {
            sprintf(buf, "NO SIGNAL       ");
        } else {
            // Tối đa: 10000Hz 100% (13 ký tự)
            sprintf(buf, "%-5luHz %-3lu%%   ", freq, duty);
        }
        buf[16] = '\0';
        LCD_SetCursor(1, 0);
        LCD_String(buf);
    }
}

void Menu_ButtonHandler(uint8_t btn) {
    // Chỉ lưu trạng thái nút nhấn vào biến cờ, KHÔNG GỌI LCD_String ở đây
    // Tránh bị đụng độ (Reentrancy) với hàm Menu_Tick đang chạy ở main loop
    pending_button = btn;
}
