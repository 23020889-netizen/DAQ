#include "lcd_i2c.h"

static void LCD_Delay(uint32_t count);
static void I2C1_WriteByte(uint8_t data);
static void LCD_Send(uint8_t data, uint8_t flags);

/* ----------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>

enum {
    LCD_MAX_COL = 16,

    PROP_SIGNAL_GEN_MIN     = 0,
    PROP_SIGNAL_GEN_MAX     = 1000,
    PROP_SIGNAL_GEN_STEP    = 5,
    PROP_SIGNAL_GEN_DEFAULT = 10,
};

enum OpMode {
    OPMODE_SIGNAL_GEN = 0,
    OPMODE_ANALOG_IN,
    OPMODE_DIGITAL_IN,
};

enum SigType {
    SIGTYPE_SINE = 0,
    SIGTYPE_TRIANGLE,
    SIGTYPE_SQUARE
};

struct OpState {
    enum OpMode prevMode;           // [out] Previous operation mode
    enum OpMode mode;               // [out] Current operation mode
    union {
        struct {
            enum SigType type;      // [out] Signal type
            int16_t freq;           // [out] Current signal frequency
            int16_t pendingFreq;    // [out] Pending signal frequency
        } signalGen;
        struct {
            float voltage;          // [in] Measured voltage
        } analogIn;
        struct {
            int16_t freq;           // [in] Measured frequency
            int8_t dutyCycle;       // [in] Measured duty cycle
        } digitalIn;
    };
};

/* UI text tables -------------------------------------- */

static const char *const menuTitleTbl[] = {
    [OPMODE_SIGNAL_GEN] = "Signal Generator",
    [OPMODE_ANALOG_IN]  = "Analog Input    ",
    [OPMODE_DIGITAL_IN] = "Digital Input   ",
};

static const char *const sigTypeTextTbl[] = {
    [SIGTYPE_SINE]      = "Sin",
    [SIGTYPE_TRIANGLE]  = "Tri",
    [SIGTYPE_SQUARE]    = "Sqr"
};

/* UI/operation state ---------------------------------- */

static struct OpState opState = { 0 };

/* UI functions declaration ---------------------------- */

void UI_UpdateText();
void UI_UpdateTitleText();
void UI_UpdateSubText();

void UI_OnMenuButtonClick();
void UI_OnLeftButtonClick();
void UI_OnRightButtonClick();
void UI_OnEnterButtonClick();

/* UI functions definition ----------------------------- */

void UI_UpdateText() {
    UI_UpdateTitleText();
    UI_UpdateSubText();
}

void UI_UpdateTitleText() {
    static uint8_t isFirstUpdate = 1;

    if (isFirstUpdate) {
        isFirstUpdate = 0;
    } else if (opState.prevMode == opState.mode) {
        return;
    }

    LCD_SetCursor(0, 0);
    LCD_String(menuTitleTbl[opState.mode]);
}

void UI_UpdateSubText() {
    char buf[LCD_MAX_COL + 1] = { 0 };

    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            // Type:abc|Hz:xxxx
            snprintf(
                buf, sizeof(buf), "Type:%3s/Hz%c%4u",
                sigTypeTextTbl[opState.signalGen.type],
                // indicate frequency change
                opState.signalGen.freq == opState.signalGen.pendingFreq ? ':' : '*',
                opState.signalGen.freq
            );
            break;
        }
        case OPMODE_ANALOG_IN: {
            // Voltage: x.xx
            snprintf(buf, sizeof(buf), "Voltage: %7.2f", opState.analogIn.voltage);
            break;
        }
        case OPMODE_DIGITAL_IN: {
            // Hz:xxxx|Duty:xxx
            snprintf(
                buf, sizeof(buf), "Hz:%4u/Duty:%3u",
                opState.digitalIn.freq, opState.digitalIn.dutyCycle
            );
            break;
        }
        default: {
            break;
        }
    }

    LCD_SetCursor(1, 0);
    LCD_String(buf);
}

void UI_OnMenuButtonClick() {
    const enum OpMode prevMode = opState.mode;

    // Change operation mode
    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            switch (opState.signalGen.type) {
                case SIGTYPE_SINE:
                    opState.signalGen.type = SIGTYPE_TRIANGLE;
                    break;
                case SIGTYPE_TRIANGLE:
                    opState.signalGen.type = SIGTYPE_SQUARE;
                    break;
                case SIGTYPE_SQUARE:
                    opState.mode = OPMODE_ANALOG_IN;
                    break;
                default:
                    break;
            }
            break;
        }
        case OPMODE_ANALOG_IN: {
            opState.mode = OPMODE_DIGITAL_IN;
            break;
        }
        case OPMODE_DIGITAL_IN: {
            opState.mode = OPMODE_SIGNAL_GEN;
            break;
        }
        default: {
            break;
        }
    }

    // Set default state when changing mode
    if (prevMode == opState.mode) {
        return;
    }
    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            opState.signalGen.type = SIGTYPE_SINE;
            opState.signalGen.freq = PROP_SIGNAL_GEN_DEFAULT;
            opState.signalGen.pendingFreq = opState.signalGen.freq;
            break;
        }
        case OPMODE_ANALOG_IN: {
            opState.analogIn.voltage = 0;
            break;
        }
        case OPMODE_DIGITAL_IN: {
            opState.digitalIn.freq = 0;
            opState.digitalIn.dutyCycle = 0;
            break;
        }
        default: {
            break;
        }
    }
}

void UI_OnLeftButtonClick() {
    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            const int16_t newFreq = opState.signalGen.pendingFreq - PROP_SIGNAL_GEN_STEP;
            opState.signalGen.pendingFreq = newFreq >= PROP_SIGNAL_GEN_MIN
                ? newFreq
                : PROP_SIGNAL_GEN_MIN;
            break;
        }
        case OPMODE_ANALOG_IN:
        case OPMODE_DIGITAL_IN:
        default: {
            break;
        }
    }
}

void UI_OnRightButtonClick() {
    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            const int16_t newFreq = opState.signalGen.pendingFreq + PROP_SIGNAL_GEN_STEP;
            opState.signalGen.pendingFreq = newFreq <= PROP_SIGNAL_GEN_MAX
                ? newFreq
                : PROP_SIGNAL_GEN_MAX;
            break;
        }
        case OPMODE_ANALOG_IN:
        case OPMODE_DIGITAL_IN:
        default: {
            break;
        }
    }
}

void UI_OnEnterButtonClick() {
    switch (opState.mode) {
        case OPMODE_SIGNAL_GEN: {
            opState.signalGen.freq = opState.signalGen.pendingFreq;
            break;
        }
        case OPMODE_ANALOG_IN:
        case OPMODE_DIGITAL_IN: 
        default: {
            break;
        }
    }
}
