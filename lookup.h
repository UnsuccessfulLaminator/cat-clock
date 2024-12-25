// Binary data of the 8 custom characters used to create large digits.
// The characters are 5x8, so the 3 most significant bits aren't used.
char CUSTOM_CHARS[8][8] = {
    {
        0b00000001,
        0b00000011,
        0b00000011,
        0b00000011,
        0b00000011,
        0b00000011,
        0b00000011,
        0b00000001
    },
    {
        0b00010000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00010000
    },
    {
        0b00011111,
        0b00011111,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },
    {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00011111,
        0b00011111
    },
    {
        0b00011111,
        0b00011111,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00011111,
        0b00011111
    },
    {
        0b00000001,
        0b00000011,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },
    {
        0b00010000,
        0b00011000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },
    {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00010000,
        0b00011000
    }
};

// Indices 0 to 7 correspond to one of the custom characters above.
// S contains the character code for ASCII space for convenience.
const char S = ' ';
const char DIGITS[10][6] = {
    {0, 2, 1, 0, 3, 1}, // 0
    {S, 0, S, S, 0, S}, // 1
    {5, 4, 1, 0, 3, 7}, // 2
    {5, 4, 1, S, 3, 1}, // 3
    {0, 3, 1, S, S, 1}, // 4
    {0, 4, 6, S, 3, 1}, // 5
    {0, 4, 6, 0, 3, 1}, // 6
    {5, 2, 1, S, 0, S}, // 7
    {0, 4, 1, 0, 3, 1}, // 8
    {0, 4, 1, S, S, 1}  // 9
};

const char *WEEKDAY_LETTERS = "MTWRFSU";
const char *MONTH_NAMES[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const char HELLO_MSG[2][16] = {
    {0, 3, 1, 0, 4, 6, 0, S, 0, S, S, 0, 2, 1, S, 1},
    {0, S, 1, 0, 3, 7, 0, 3, 0, 3, 7, 0, 3, 1, S, 7}
};

const char MENU_SCREEN[2][16] = {
    ">Alarm   Time   ",
    " Back           "
};