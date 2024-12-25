#include <stddef.h>
#include <LiquidCrystal_I2C.h>
#include <Print.h>
#include "lookup.h"

#define LCD_ADDRESS 0x27
#define RTC_ADDRESS 0x68

#define BUTTON_PIN 2
#define ENCODER_CLK_PIN 3
#define ENCODER_DATA_PIN 4
#define SOUND_PIN 5
#define ALARM_SWITCH_PIN 6

#define BUTTON_INTERRUPT digitalPinToInterrupt(BUTTON_PIN)
#define ENCODER_INTERRUPT digitalPinToInterrupt(ENCODER_CLK_PIN)

enum Mode {
    SHOW_TIME,
    SHOW_TIME_BRIGHT,
    ALARM,
    MENU,
    SET_TIME,
    SET_ALARM
};

struct Time {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t date;
    uint8_t month;
    uint8_t year;
};

struct State {
    Mode mode;
    uint8_t elapsed;
    uint8_t menu_option;
    Time setting_time;

    LiquidCrystal_I2C lcd;
};

struct Events {
    bool button;
    bool timer;
    bool encoder;
    bool encoder_forward;
};

// Interrupts will set these to true
volatile bool button_pressed, encoder_moved, encoder_forward;

void lcd_write_bytes(LiquidCrystal_I2C *lcd, const char *bytes, int n);
void lcd_write_screen(LiquidCrystal_I2C *lcd, const char *bytes);
void display_time(LiquidCrystal_I2C *lcd, const Time *time);

void rtc_init();
Time rtc_time();
Time rtc_alarm_time();
void rtc_set_alarm_time();
bool rtc_alarm_active();
void rtc_alarm_deactivate();

// Not using this to avoid unnecessary global variables
void loop() {}

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK_PIN, INPUT);
    pinMode(ENCODER_DATA_PIN, INPUT);
    pinMode(SOUND_PIN, INPUT); // Actually using this as open-drain
    pinMode(ALARM_SWITCH_PIN, INPUT_PULLUP);

    attachInterrupt(BUTTON_INTERRUPT, BUTTON_ISR, RISING);
    attachInterrupt(ENCODER_INTERRUPT, ENCODER_ISR, RISING);

    Serial.begin(57600);

    State state = {
        .mode = SHOW_TIME,
        .elapsed = 0,
        .menu_option = 0,
        .setting_time = Time {0},
        .lcd = LiquidCrystal_I2C(LCD_ADDRESS, 16, 2)
    };

    rtc_init();
    state.lcd.init();
    state.lcd.noBacklight();

    for(int i = 0; i < 8; i++) {
        state.lcd.createChar(i, (uint8_t*) CUSTOM_CHARS[i]);
    }

    // Display the startup message for 1 second
    state.lcd.clear();
    lcd_write_screen(&state.lcd, &HELLO_MSG[0][0]);
    delay(1000);

    button_pressed = false;
    encoder_moved = false;

    Serial.println("Entering main loop");

    for(int i = 0; true; i = (i+1)%100) {
        Events events = {0};
        events.timer = i == 0;
        events.button = button_pressed;
        events.encoder = encoder_moved;
        events.encoder_forward = encoder_forward;

        button_pressed = false;
        encoder_moved = false;
        
        if(Serial.available()) {
            char rec = Serial.read();

            if(rec == 'f' || rec == 'b') {
                events.encoder = true;
                events.encoder_forward = rec == 'f';
            }
            else events.button = true;
        }

        update(&state, events);
        delay(10);
    }
}

void update(State *state, Events events) {
    if(state->mode == SHOW_TIME) {
        if(events.timer) {
            Serial.println("Timer tick in SHOW_TIME");

            if(state->elapsed == 0) {
                Time t = rtc_time();
                display_time(&state->lcd, &t);

                if(rtc_alarm_active()) {
                    if(digitalRead(ALARM_SWITCH_PIN)) state->mode = ALARM;
                    else rtc_alarm_deactivate();
                }
            }

            state->elapsed = (state->elapsed+1)%60;
        }

        if(events.button) {
            Serial.println("Button press in SHOW_TIME");

            state->lcd.backlight();
            state->elapsed = 0;
            state->mode = SHOW_TIME_BRIGHT;
        }
    }
    else if(state->mode == SHOW_TIME_BRIGHT) {
        if(events.timer) {
            Serial.println("Timer tick in SHOW_TIME_BRIGHT");

            if(state->elapsed < 3) state->elapsed++;
            else {
                state->lcd.noBacklight();
                state->elapsed = 0;
                state->mode = SHOW_TIME;
            }
        }

        if(events.button) {
            Serial.println("Button press in SHOW_TIME_BRIGHT");

            state->lcd.noBacklight();
            state->mode = MENU;
            state->menu_option = 0;

            lcd_write_screen(&state->lcd, &MENU_SCREEN[0][0]);
        }
    }
    else if(state->mode == ALARM) {
        if(events.timer) {
            Serial.println("Timer tick in ALARM");

            if(state->elapsed % 2) {
                state->lcd.backlight();

                pinMode(SOUND_PIN, OUTPUT);
                delay(100);
                pinMode(SOUND_PIN, INPUT);
            }
            else state->lcd.noBacklight();

            state->elapsed++;
        }

        if(events.button) {
            Serial.println("Button press in ALARM");

            pinMode(SOUND_PIN, INPUT);
            rtc_alarm_deactivate();

            state->lcd.noBacklight();
            state->mode = SHOW_TIME;
            state->elapsed = 0;
        }
    }
    else if(state->mode == MENU) {
        if(events.encoder) {
            if(events.encoder_forward) Serial.println("Encoder forward");
            else Serial.println("Encoder backward");

            if(events.encoder_forward) state->menu_option++;
            else state->menu_option += 2; // Same as decrement, mod 3

            state->menu_option %= 3;

            for(int i = 0; i < 4; i++) {
                state->lcd.setCursor((i%2)*8, i/2);
                
                if(i == state->menu_option) state->lcd.print('>');
                else state->lcd.print(' ');
            }
        }

        if(events.button) {
            Serial.print("Button press in MENU. Option = ");
            Serial.print((int) (state->menu_option));
            Serial.println("");

            if(state->menu_option == 0) {
                Serial.println("Selected alarm option");

                Time t = rtc_alarm_time();

                state->mode = SET_ALARM;
                state->setting_time = t;
                state->menu_option = 0;

                display_time_setting(&state->lcd, &t, 0, false);
            }
            else if(state->menu_option == 1) {
                Serial.println("Selected time option");
                
                Time t = rtc_time();

                state->mode = SET_TIME;
                state->setting_time = t;
                state->menu_option = 0;

                display_time_setting(&state->lcd, &t, 0, true);
            }
            else if(state->menu_option == 2) {
                Serial.println("Selected back option");
                
                state->mode = SHOW_TIME;
                state->elapsed = 0;
            }
        }
    }
    else if(state->mode == SET_TIME) {
        Time *t = &state->setting_time;

        if(events.button) {
            if(state->menu_option == 4) {
                rtc_set_time(t);

                state->mode = SHOW_TIME;
                state->elapsed = 0;
            }
            else {
                state->menu_option++;

                display_time_setting(&state->lcd, t, state->menu_option, true);
            }
        }

        if(events.encoder) {
            if(events.encoder_forward) {
                switch(state->menu_option) {
                case 0: t->hours = (t->hours+1)%24; break;
                case 1: t->minutes = (t->minutes+1)%60; break;
                case 2: t->weekday = (t->weekday%7)+1; break;
                case 3: t->date = (t->date%31)+1; break;
                case 4: t->month = (t->month%12)+1;
                }
            }
            else {
                switch(state->menu_option) {
                case 0: t->hours = (t->hours+23)%24; break;
                case 1: t->minutes = (t->minutes+59)%60; break;
                case 2: t->weekday = ((t->weekday+5)%7)+1; break;
                case 3: t->date = ((t->date+29)%31)+1; break;
                case 4: t->month = ((t->month+10)%12)+1;
                }
            }

            display_time_setting(&state->lcd, t, state->menu_option, true);
        }
    }
    else if(state->mode == SET_ALARM) {
        Time *t = &state->setting_time;
        
        if(events.button) {
            if(state->menu_option == 1) {
                rtc_set_alarm_time(t);

                state->mode = SHOW_TIME;
                state->elapsed = 0;
            }
            else {
                state->menu_option++;

                display_time_setting(&state->lcd, t, state->menu_option, false);
            }
        }

        if(events.encoder) {
            if(events.encoder_forward) {
                switch(state->menu_option) {
                case 0: t->hours = (t->hours+1)%24; break;
                case 1: t->minutes = (t->minutes+1)%60;
                }
            }
            else {
                switch(state->menu_option) {
                case 0: t->hours = (t->hours+23)%24; break;
                case 1: t->minutes = (t->minutes+59)%60;
                }
            }

            display_time_setting(&state->lcd, t, state->menu_option, false);
        }
    }
}

// Positions go through hours, minutes, weekday, date, and month.
void display_time_setting(
    LiquidCrystal_I2C *lcd, const Time *time, uint8_t pos, bool date
) {
    lcd->clear();
    lcd->home();
    lcd->write('0'+time->hours/10);
    lcd->write('0'+time->hours%10);
    lcd->write(':');
    lcd->write('0'+time->minutes/10);
    lcd->write('0'+time->minutes%10);

    if(date) {
        lcd->write(' ');
        lcd->write(WEEKDAY_LETTERS[time->weekday-1]);
        lcd->write(' ');
        lcd->write('0'+time->date/10);
        lcd->write('0'+time->date%10);
        lcd->write(' ');
        lcd->print(MONTH_NAMES[time->month-1]);
    }

    const uint8_t positions[5] = {1, 4, 6, 9, 12};

    lcd->setCursor(positions[pos], 1);
    lcd->write('^');
}

void rtc_init() {
    uint8_t alarm1_data[4];

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x07);
    Wire.endTransmission();
    Wire.requestFrom(RTC_ADDRESS, 4);
    Wire.readBytes(alarm1_data, 4);

    // Set the A1M bits so the alarm needs only HH:MM:SS to match
    alarm1_data[0] &= 0x7F;
    alarm1_data[1] &= 0x7F;
    alarm1_data[2] &= 0x7F;
    alarm1_data[3] |= 0x80;

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x07);
    Wire.write(alarm1_data, 4);
    Wire.endTransmission();
}

uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd & 0x0F)+(bcd >> 4)*10;
}

uint8_t bin_to_bcd(uint8_t bin) {
    return (bin % 10) | (bin/10) << 4;
}

void rtc_set_time(const Time *t) {
    uint8_t data[7] = {
        bin_to_bcd(t->seconds),
        bin_to_bcd(t->minutes),
        bin_to_bcd(t->hours),
        t->weekday,
        bin_to_bcd(t->date),
        bin_to_bcd(t->month),
        bin_to_bcd(t->year)
    };

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x00);
    Wire.write(data, 7);
    Wire.endTransmission();
}

void rtc_set_alarm_time(const Time *time) {
    uint8_t data[3] = {
        0,
        bin_to_bcd(time->minutes),
        bin_to_bcd(time->hours)
    };

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x07);
    Wire.write(data, 3);
    Wire.endTransmission();
}

Time rtc_time() {
    uint8_t data[7];

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.requestFrom(RTC_ADDRESS, 7);
    Wire.readBytes(data, 7);

    return Time {
        .seconds = bcd_to_bin(data[0]),
        .minutes = bcd_to_bin(data[1]),
        .hours = bcd_to_bin(data[2] & 0x3F),
        .weekday = data[3],
        .date = bcd_to_bin(data[4]),
        .month = bcd_to_bin(data[5] & 0x1F),
        .year = bcd_to_bin(data[6])
    };
}

Time rtc_alarm_time() {
    uint8_t data[2];

    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x08);
    Wire.endTransmission();
    Wire.requestFrom(RTC_ADDRESS, 2);
    Wire.readBytes(data, 2);

    return Time {
        .seconds = 0,
        .minutes = bcd_to_bin(data[0] & 0x7F),
        .hours = bcd_to_bin(data[1] & 0x3F),
        .weekday = 1,
        .date = 1,
        .month = 1,
        .year = 0
    };
}

bool rtc_alarm_active() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0F);
    Wire.endTransmission();
    Wire.requestFrom(RTC_ADDRESS, 1);

    return (Wire.read() & 0x01) > 0;
}

void rtc_alarm_deactivate() {
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0F);
    Wire.write(0);
    Wire.endTransmission();
}

void display_time(LiquidCrystal_I2C *lcd, const Time *time) {
    char screen[2][16];
    uint8_t hours = time->hours, minutes = time->minutes;
    uint8_t digits[4], offsets[4] = {3, 6, 10, 13};

    digits[0] = hours/10;
    digits[1] = hours%10;
    digits[2] = minutes/10;
    digits[3] = minutes%10;

    screen[0][9] = screen[1][9] = '.'; // Hour:minute separator dots

    for(int i = 0; i < 4; i++) {
        uint8_t offset = offsets[i], digit = digits[i];

        memcpy(&screen[0][offset], DIGITS[digit], 3);
        memcpy(&screen[1][offset], &DIGITS[digit][3], 3);
    }

    memcpy(screen[0], MONTH_NAMES[time->month-1], 3);

    screen[1][0] = WEEKDAY_LETTERS[time->weekday-1];
    screen[1][1] = '0'+time->date/10;
    screen[1][2] = '0'+time->date%10;
    
    lcd_write_screen(lcd, &screen[0][0]);
}

void lcd_write_bytes(LiquidCrystal_I2C *lcd, const char *bytes, int n) {
    for(int i = 0; i < n; i++) lcd->write(bytes[i]);
}

void lcd_write_screen(LiquidCrystal_I2C *lcd, const char *screen) {
    lcd->home();
    lcd_write_bytes(lcd, screen, 16);
    lcd->setCursor(0, 1);
    lcd_write_bytes(lcd, screen+16, 16);
}

void BUTTON_ISR() {
    button_pressed = true;
}

void ENCODER_ISR() {
    encoder_moved = true;
    encoder_forward = digitalRead(ENCODER_DATA_PIN);
}
