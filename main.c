#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

// pini
#define PIN_R   PD5     // led rosu
#define PIN_G   PD6     // led verde
#define BUZZER  PD1     // buzzer pasiv
#define BUTTON  PD2     // buton

#define SENSOR_CHANNEL 0   // senzor pe ADC0

// config
#define OLED_ADDR 0x3C
#define MEASURE_SECONDS  60

// praguri senzor
#define VAL_MIN_VALID    200
#define VAL_MAX_VALID    950
#define PRAG_DEGET_SUS   650   // peste asta = deget pus
#define PRAG_DEGET_JOS   580   // sub asta = deget luat

#define FINGER_LOST_MS   1200  // timp fara deget pana revenim la OFF
#define CALIB_MS         2000  // durata calibrarii

// limitele BPM aleator
#define BPM_MIN          50
#define BPM_MAX          120
#define BPM_NORMAL_JOS   60
#define BPM_NORMAL_SUS   100

// stari
typedef enum {
    STATE_OFF,
    STATE_CALIB,
    STATE_WAIT_FINGER,
    STATE_MEASURING
} State;

State state = STATE_OFF;

// timp - Timer0 la 1 ms
volatile uint32_t millis_counter = 0;

void timer0_init(void) {
    TCCR0A = (1 << WGM01);                  // mod CTC
    OCR0A  = 249;                           // 1 ms
    TIMSK0 = (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00);     // prescaler 64
}

ISR(TIMER0_COMPA_vect) {
    millis_counter++;
}

// citire atomica a contorului
uint32_t millis(void) {
    uint32_t v;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        v = millis_counter;
    }
    return v;
}

// led + buzzer
void red_on(void)    { PORTD |=  (1 << PIN_R); }
void red_off(void)   { PORTD &= ~(1 << PIN_R); }
void green_on(void)  { PORTD |=  (1 << PIN_G); }
void green_off(void) { PORTD &= ~(1 << PIN_G); }

void all_off(void) {
    red_off();
    green_off();
    PORTD &= ~(1 << BUZZER);
}

// ton pentru buzzer
void buzzer_tone(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        PORTD |=  (1 << BUZZER);
        _delay_us(416);
        PORTD &= ~(1 << BUZZER);
        _delay_us(416);
    }
}

// efect lub-dub la fiecare bataie
void beat_effect(void) {
    buzzer_tone(70);
    _delay_ms(30);
    buzzer_tone(50);
}

// buton
bool lastButtonState = true;

bool button_read(void) {
    return (PIND & (1 << BUTTON)) != 0;
}

// true o data, la apasare, cu debounce
bool button_pressed_event(void) {
    bool current = button_read();
    if (lastButtonState == true && current == false) {
        _delay_ms(50);
        current = button_read();
        if (current == false) {
            lastButtonState = false;
            return true;
        }
    }
    lastButtonState = current;
    return false;
}

// ADC
void adc_init(void) {
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel) {
    channel &= 0x07;
    ADMUX = (ADMUX & 0xF0) | channel;
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// citire filtrata - arunca glitch-urile, face media
uint16_t citeste_filtrat(void) {
    uint32_t sum = 0;
    uint8_t  valide = 0;
    for (uint8_t i = 0; i < 16; i++) {
        uint16_t v = adc_read(SENSOR_CHANNEL);
        if (v >= VAL_MIN_VALID && v <= VAL_MAX_VALID) {
            sum += v;
            valide++;
        }
        _delay_ms(2);
    }
    if (valide == 0) return 0xFFFF;   // toate au fost glitch
    return (uint16_t)(sum / valide);
}

// I2C / TWI
void twi_init(void) {
    TWSR = 0x00;
    TWBR = 72;
}

void twi_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

void twi_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

void twi_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

// OLED SSD1306
void oled_command(uint8_t cmd) {
    twi_start();
    twi_write(OLED_ADDR << 1);
    twi_write(0x00);
    twi_write(cmd);
    twi_stop();
}

void oled_data(uint8_t data) {
    twi_start();
    twi_write(OLED_ADDR << 1);
    twi_write(0x40);
    twi_write(data);
    twi_stop();
}

void oled_init(void) {
    _delay_ms(100);
    oled_command(0xAE);
    oled_command(0xD5); oled_command(0x80);
    oled_command(0xA8); oled_command(0x3F);
    oled_command(0xD3); oled_command(0x00);
    oled_command(0x40);
    oled_command(0x8D); oled_command(0x14);
    oled_command(0x20); oled_command(0x00);
    oled_command(0xA1);
    oled_command(0xC8);
    oled_command(0xDA); oled_command(0x12);
    oled_command(0x81); oled_command(0x7F);
    oled_command(0xD9); oled_command(0xF1);
    oled_command(0xDB); oled_command(0x40);
    oled_command(0xA4);
    oled_command(0xA6);
    oled_command(0xAF);
}

void oled_set_position(uint8_t page, uint8_t col) {
    oled_command(0xB0 + page);
    oled_command(0x00 + (col & 0x0F));
    oled_command(0x10 + ((col >> 4) & 0x0F));
}

void oled_clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_position(page, 0);
        for (uint8_t col = 0; col < 128; col++) {
            oled_data(0x00);
        }
    }
}

// font
const uint8_t font_digits[10][5] PROGMEM = {
    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E}
};

const uint8_t font_B[5] PROGMEM = {0x7F,0x49,0x49,0x49,0x36};
const uint8_t font_P[5] PROGMEM = {0x7F,0x09,0x09,0x09,0x06};
const uint8_t font_M[5] PROGMEM = {0x7F,0x02,0x0C,0x02,0x7F};
const uint8_t font_O[5] PROGMEM = {0x3E,0x41,0x41,0x41,0x3E};
const uint8_t font_F[5] PROGMEM = {0x7F,0x09,0x09,0x09,0x01};
const uint8_t font_A[5] PROGMEM = {0x7E,0x09,0x09,0x09,0x7E};
const uint8_t font_I[5] PROGMEM = {0x00,0x41,0x7F,0x41,0x00};
const uint8_t font_T[5] PROGMEM = {0x01,0x01,0x7F,0x01,0x01};
const uint8_t font_L[5] PROGMEM = {0x7F,0x40,0x40,0x40,0x40};
const uint8_t font_H[5] PROGMEM = {0x7F,0x08,0x08,0x08,0x7F};
const uint8_t font_E[5] PROGMEM = {0x7F,0x49,0x49,0x49,0x41};
const uint8_t font_C[5] PROGMEM = {0x3E,0x41,0x41,0x41,0x22};
const uint8_t font_R[5] PROGMEM = {0x7F,0x09,0x19,0x29,0x46};
const uint8_t font_N[5] PROGMEM = {0x7F,0x04,0x08,0x10,0x7F};
const uint8_t font_G[5] PROGMEM = {0x3E,0x41,0x49,0x49,0x7A};
const uint8_t font_K[5] PROGMEM = {0x7F,0x08,0x14,0x22,0x41};
const uint8_t font_space[5] PROGMEM = {0x00,0x00,0x00,0x00,0x00};

const uint8_t* get_char_bitmap(char c) {
    if (c >= '0' && c <= '9') return font_digits[c - '0'];
    switch (c) {
        case 'B': return font_B; case 'P': return font_P;
        case 'M': return font_M; case 'O': return font_O;
        case 'F': return font_F; case 'A': return font_A;
        case 'I': return font_I; case 'T': return font_T;
        case 'L': return font_L; case 'H': return font_H;
        case 'E': return font_E; case 'C': return font_C;
        case 'R': return font_R; case 'N': return font_N;
        case 'G': return font_G; case 'K': return font_K;
        case ' ': return font_space;
        default:  return font_space;
    }
}

void oled_char(char c) {
    const uint8_t *bitmap = get_char_bitmap(c);
    for (uint8_t i = 0; i < 5; i++) oled_data(pgm_read_byte(&bitmap[i]));
    oled_data(0x00);
}

void oled_print(const char *s) {
    while (*s) { oled_char(*s); s++; }
}

// numar pe 3 cifre, latime fixa
void oled_print_number(uint16_t n) {
    oled_char('0' + (n / 100) % 10);
    oled_char('0' + (n / 10) % 10);
    oled_char('0' + (n % 10));
}

void oled_heart(uint8_t page, uint8_t col) {
    static const uint8_t heart[8] PROGMEM = {
        0x0C, 0x1E, 0x3E, 0x7C,
        0x7C, 0x3E, 0x1E, 0x0C
    };
    oled_set_position(page, col);
    for (uint8_t i = 0; i < 8; i++) oled_data(pgm_read_byte(&heart[i]));
}

// ecrane
void display_off(void) {
    oled_clear();
    oled_set_position(3, 50);
    oled_print("OFF");
}

void display_calib(void) {
    oled_clear();
    oled_set_position(2, 40);
    oled_print("CALIB");
    oled_heart(4, 56);
}

// ecran de asteptare deget
void display_wait_finger(void) {
    oled_clear();
    oled_set_position(2, 34);
    oled_print("FINGER");
    oled_heart(4, 56);
}

void display_measure(uint8_t secondsLeft, uint8_t beats) {
    oled_set_position(0, 0);
    oled_print("TIME ");
    oled_print_number(secondsLeft);

    oled_set_position(3, 0);
    oled_print("BPM ");
    oled_print_number(beats);

    // inima langa numarul batailor
    oled_heart(3, 48);
}

// ecran final cu verdict
void display_result(uint8_t bpm, bool ok) {
    oled_clear();
    oled_set_position(1, 20);
    oled_print("BPM ");
    oled_print_number(bpm);
    oled_heart(1, 90);

    oled_set_position(4, 30);
    if (ok) oled_print("OK");
    else    oled_print("ANORMAL");
}

// setup
void setup_pins(void) {
    DDRD |= (1 << PIN_R) | (1 << PIN_G) | (1 << BUZZER);  // iesiri
    DDRD &= ~(1 << BUTTON);                               // buton intrare
    PORTD |= (1 << BUTTON);                               // pull-up buton
    all_off();
}

// converteste BPM in interval intre batai (ms)
uint16_t bpm_to_interval(uint8_t bpm) {
    if (bpm == 0) bpm = 1;
    return (uint16_t)(60000UL / bpm);
}

// generator simplu de numere aleatoare
uint32_t rng_state = 12345;

uint16_t rng_next(void) {
    rng_state = rng_state * 1103515245UL + 12345UL;
    return (uint16_t)((rng_state >> 16) & 0x7FFF);
}

// alege un BPM aleator intre BPM_MIN si BPM_MAX
uint8_t bpm_aleator(void) {
    uint16_t span = BPM_MAX - BPM_MIN + 1;
    return (uint8_t)(BPM_MIN + (rng_next() % span));
}

// main
int main(void) {
    setup_pins();
    adc_init();
    twi_init();
    oled_init();
    timer0_init();
    sei();

    display_off();

    // variabile de sesiune
    uint8_t  beats        = 0;
    uint32_t sessionStart = 0;
    uint32_t lastBeatMs   = 0;
    uint32_t fingerLostAt = 0;
    uint8_t  lastSecShown = 255;
    bool     fingerOn     = false;     // starea degetului, cu histerezis
    uint8_t  bpm_ales     = 75;        // BPM ales aleator la atingere
    uint16_t interval     = 800;       // interval intre batai (ms)

    while (1) {

        // buton
        if (button_pressed_event()) {
            if (state == STATE_OFF) {
                state = STATE_CALIB;
            } else {
                // apasare in alta stare = oprire, revenim la OFF
                state = STATE_OFF;
                all_off();
                display_off();
            }
        }

        // OFF
        if (state == STATE_OFF) {
            _delay_ms(20);
            continue;
        }

        // CALIB
        if (state == STATE_CALIB) {
            display_calib();

            // citim senzorul fara deget cateva secunde (pas vizual)
            uint32_t t0 = millis();
            while (millis() - t0 < CALIB_MS) {
                citeste_filtrat();
            }

            beats        = 0;
            lastBeatMs   = millis();
            fingerLostAt = 0;
            lastSecShown = 255;
            fingerOn     = false;

            // dupa calibrare asteptam degetul
            display_wait_finger();
            state = STATE_WAIT_FINGER;
            continue;
        }

        // WAIT FINGER - asteapta sa fie pus degetul
        if (state == STATE_WAIT_FINGER) {
            uint16_t v = citeste_filtrat();

            if (v != 0xFFFF && v >= PRAG_DEGET_SUS) {
                // degetul a fost pus, pornim masurarea
                fingerOn     = true;
                sessionStart = millis();
                lastBeatMs   = millis();
                fingerLostAt = 0;
                lastSecShown = 255;

                // samanta RNG = momentul atingerii, deci alt BPM de fiecare data
                rng_state ^= millis();
                bpm_ales = bpm_aleator();
                interval = bpm_to_interval(bpm_ales);

                oled_clear();
                display_measure(MEASURE_SECONDS, 0);
                state = STATE_MEASURING;
            }
            continue;
        }

        // MEASURING
        if (state == STATE_MEASURING) {
            uint16_t v = citeste_filtrat();
            uint32_t now = millis();

            // determinam daca degetul e prezent, cu histerezis
            if (v != 0xFFFF) {
                if (!fingerOn && v >= PRAG_DEGET_SUS) fingerOn = true;
                if (fingerOn  && v <  PRAG_DEGET_JOS) fingerOn = false;
            }
            // daca v == 0xFFFF (glitch total) pastram starea anterioara

            // deget luat - dupa un timp revenim la OFF
            if (!fingerOn) {
                if (fingerLostAt == 0) {
                    fingerLostAt = now;
                } else if (now - fingerLostAt >= FINGER_LOST_MS) {
                    state = STATE_OFF;
                    all_off();
                    display_off();
                    continue;
                }
            } else {
                fingerLostAt = 0;
            }

            // inima bate doar cat timp degetul e prezent
            if (fingerOn) {
                if (now - lastBeatMs >= interval) {
                    lastBeatMs = now;
                    beats++;

                    // ledul rosu pulseaza sincron cu buzzer-ul
                    green_off();
                    red_on();                   // led rosu aprins
                    beat_effect();              // lub-dub
                    red_off();                  // se stinge dupa bataie

                    // actualizam display-ul
                    uint32_t e = now - sessionStart;
                    uint8_t sp = (uint8_t)(e / 1000);
                    uint8_t secLeft = (sp < MEASURE_SECONDS)
                                    ? (MEASURE_SECONDS - sp) : 0;
                    display_measure(secLeft, beats);
                }
            } else {
                // fara deget: leduri stinse
                red_off();
                green_off();
            }

            // cronometru - actualizam o data pe secunda
            uint32_t e = now - sessionStart;
            uint8_t secondsPassed = (uint8_t)(e / 1000);
            if (secondsPassed != lastSecShown) {
                lastSecShown = secondsPassed;
                uint8_t secLeft = (secondsPassed < MEASURE_SECONDS)
                                ? (MEASURE_SECONDS - secondsPassed) : 0;
                display_measure(secLeft, beats);
            }

            // final - afisam verdictul
            if (e >= (uint32_t)MEASURE_SECONDS * 1000UL) {
                all_off();

                // verdict dupa BPM-ul sesiunii: 60..100 = OK
                bool ok = (bpm_ales >= BPM_NORMAL_JOS &&
                           bpm_ales <= BPM_NORMAL_SUS);
                if (ok) {
                    green_on();
                } else {
                    red_on();
                }
                display_result(bpm_ales, ok);

                // ramanem pe ecranul de rezultat
                state = STATE_OFF;
                continue;
            }

            continue;
        }
    }

    return 0;
}
