#include <Wire.h>
#include <MsTimer2.h>
#include <util/delay.h>

#include "iv6_n.h"

//#define DEBUG_CLOCK 1

#define FASTLED_ENABLED 1

/***********************************
* WS2812B
***********************************/

#ifdef FASTLED_ENABLED

#include <FastLED.h>

#define PIN_WS21B_DATA analogInputToDigitalPin(1)

#define NUM_LEDS 5
#define LED_DITHER 255
#define CORRECTION TypicalLEDStrip

uint8_t BRIGHTNESS_HIGH = 255;
uint8_t BRIGHTNESS_LOW = 100;
CRGB leds[NUM_LEDS];

CHSV solid_color;
#endif

void fastled_render_routine()
{
#ifdef FASTLED_ENABLED
    for (int i = NUM_LEDS - 1; i >= 0; i--)
    {
        leds[i] = solid_color;
    }
    FastLED.show();
#endif
}

/***********************************
* Shift Register
***********************************/

#define PIN_SR_DATA (uint8_t)5  //PD5  // DS
#define PIN_SR_CLOCK (uint8_t)6 //PD6 // SH_CP
#define PIN_SR_LATCH (uint8_t)7 //PD7 // ST_CP

/***********************************
* RTC
***********************************/

#include <DS3231.h>

DS3231 clock;
DateTime now;

#define PIN_SQW 10 //PB2

/***********************************
* DHT12
***********************************/

#define DHT12_ENABLED 1

#ifdef DHT12_ENABLED

#include "DHT12.h"

DHT12 dht12;
unsigned long int temp_read_timer;
#endif

/***********************************
* EEPROM
***********************************/

#include <avr/eeprom.h>

#define EEPROM_ADDR_COLOR 0
#define EEPROM_ADDR_BRIGHTNESS 1

void EEPROM_save(uint8_t color, uint8_t brightness)
{
    eeprom_write_byte((uint8_t *)EEPROM_ADDR_COLOR, color);
    eeprom_write_byte((uint8_t *)EEPROM_ADDR_BRIGHTNESS, brightness);
}

/***********************************
* IV6
***********************************/

unsigned long int render_timer;

volatile uint8_t scan_grid_n = 0;

struct
{
    volatile uint8_t byte_0;
    volatile uint8_t byte_1;
    volatile uint8_t byte_2;
    volatile uint8_t byte_3;
    volatile uint8_t byte_4;
} display_bytes;

/***********************************
* LDR
***********************************/

#define PIN_LDR A2
#define LDR_LOW_TRESHOLD 800
#define LDR_HIGH_TRESHOLD 900

enum
{
    LDR_STATE_HIGH,
    LDR_STATE_LOW,
};

unsigned long int ldr_timer;
uint8_t ldr_state = LDR_STATE_HIGH;
uint8_t ldr_toggle = 0;

void ldr_routine()
{
    int ldr_val = analogRead(PIN_LDR);

    if (ldr_toggle == 0)
    {
        if ((ldr_val < LDR_LOW_TRESHOLD && ldr_state == LDR_STATE_HIGH) ||
            (ldr_val > LDR_HIGH_TRESHOLD && ldr_state == LDR_STATE_LOW))
        {
            ldr_toggle = 1;
            ldr_timer = millis();
        }
    }
    else
    {
        if (millis() - ldr_timer > 5000)
        {
            ldr_toggle = 0;

            if (ldr_state == LDR_STATE_HIGH && ldr_val < LDR_LOW_TRESHOLD)
            {
                ldr_state = LDR_STATE_LOW;
                solid_color.value = BRIGHTNESS_LOW;
            }
            else if (ldr_state == LDR_STATE_LOW && ldr_val > LDR_HIGH_TRESHOLD)
            {
                ldr_state = LDR_STATE_HIGH;
                solid_color.value = BRIGHTNESS_HIGH;
            }
        }
    }
}

/***********************************
* Activities
***********************************/

#include "menu.h"

ActivityManager activity_manager;

ClockActivity clock_activity(&activity_manager);
MainMenuActivity main_menu_activity(&activity_manager);
TimeSetupActivity time_setup_activity(&activity_manager);
ColorSetupActivity color_setup_activity(&activity_manager);

typedef const menu_t menu;
menu main_menu = {
    .size = 3,
    .items = {
        {
            .title = SYMBOL_C,
            .activity = &color_setup_activity,
        },
        {
            .title = SYMBOL_CH,
            .activity = &time_setup_activity,
        },
        {
            .title = SYMBOL_MINUS,
            .activity = &clock_activity,
        },
    },
};

/***********************************
* MainMenu Activity
***********************************/

void MainMenuActivity::render()
{
    display_bytes.byte_0 = SYMBOL_P;
    display_bytes.byte_1 = SYMBOL_EMPTY;
    display_bytes.byte_2 = SYMBOL_EMPTY;
    display_bytes.byte_3 = SYMBOL_EMPTY;
    display_bytes.byte_4 = this->menu->items[this->_index].title;
}

/***********************************
* Clock Activity
***********************************/

void ClockActivity::render()
{
    if (this->mode == 0)
    {
        display_bytes.byte_0 = now.hour() / 10;
        display_bytes.byte_1 = now.hour() % 10;
        if (digitalRead(PIN_SQW) == HIGH)
            display_bytes.byte_2 = SYMBOL_MINUS;
        else
            display_bytes.byte_2 = SYMBOL_EMPTY;
        display_bytes.byte_3 = now.minute() / 10;
        display_bytes.byte_4 = now.minute() % 10;
    }
    else
    {
#ifdef DHT12_ENABLED
        uint8_t temperature = dht12.getTemperature() - 4;
        display_bytes.byte_0 = SYMBOL_EMPTY;
        display_bytes.byte_1 = SYMBOL_EMPTY;
        display_bytes.byte_2 = temperature / 10;
        display_bytes.byte_3 = temperature % 10;
        display_bytes.byte_4 = SYMBOL_DEGREE;
#endif
    }

#ifdef DHT12_ENABLED
    if (mode == 0 && millis() - mode_render_timer > 20000)
    {
        uint8_t running_bytes[5] = {
            display_bytes.byte_0, display_bytes.byte_1, SYMBOL_MINUS, display_bytes.byte_3, display_bytes.byte_4};

        for (uint8_t i = 0; i < 5; i++)
        {
            for (uint8_t j = 0; j < 4 - i; j++)
            {
                running_bytes[j] = running_bytes[j + 1];
            }

            running_bytes[4 - i] = SYMBOL_EMPTY;

            display_bytes.byte_0 = running_bytes[0];
            display_bytes.byte_1 = running_bytes[1];
            display_bytes.byte_2 = running_bytes[2];
            display_bytes.byte_3 = running_bytes[3];
            display_bytes.byte_4 = running_bytes[4];

            _delay_ms(150);
        }

        mode = 1;
        mode_render_timer = millis();
    }
    else if (mode == 1 && millis() - mode_render_timer > 3000)
    {
        mode = 0;
        mode_render_timer = millis();
    }
#endif
}

void ClockActivity::rotate(uint8_t direction) {}

void ClockActivity::press()
{
    main_menu_activity.set_index(0);
    this->_activity_manager->set_current(&main_menu_activity);
}

/***********************************
* Time setup Activity
***********************************/

void TimeSetupActivity::render()
{
    display_bytes.byte_0 = SYMBOL_CH;
    display_bytes.byte_1 = SYMBOL_EMPTY;
    display_bytes.byte_2 = SYMBOL_EMPTY;

    if (this->mode == TIME_SETUP_MODE_HOUR)
    {
        display_bytes.byte_3 = this->hour / 10;
        display_bytes.byte_4 = this->hour % 10;
    }
    else if (this->mode == TIME_SETUP_MODE_MINUTE)
    {
        display_bytes.byte_3 = this->minute / 10;
        display_bytes.byte_4 = this->minute % 10;
    }
}

/***********************************
* Color setup Activity
***********************************/

void ColorSetupActivity::init()
{
    this->mode = COLOR_SETUP_MODE_COLOR;
    switch (solid_color.hue)
    {
    case HUE_AQUA:
        this->color = 0;
        break;
    case HUE_BLUE:
        this->color = 1;
        break;
    case HUE_GREEN:
        this->color = 2;
        break;
    case HUE_ORANGE:
        this->color = 3;
        break;
    case HUE_YELLOW:
        this->color = 4;
        break;
    case HUE_PINK:
        this->color = 5;
        break;
    case HUE_RED:
        this->color = 6;
        break;
    }

    this->brighness = map(BRIGHTNESS_HIGH, 0, 255, 0, 9);
}

void ColorSetupActivity::render()
{
    display_bytes.byte_0 = SYMBOL_C;
    display_bytes.byte_1 = SYMBOL_EMPTY;
    display_bytes.byte_2 = SYMBOL_EMPTY;
    display_bytes.byte_3 = SYMBOL_EMPTY;

    if (this->mode == COLOR_SETUP_MODE_COLOR)
    {
        display_bytes.byte_4 = this->color;
    }
    else if (this->mode == TIME_SETUP_MODE_MINUTE)
    {
        display_bytes.byte_4 = this->brighness;
    }
}

void ColorSetupActivity::write_color() const
{
    switch (this->color)
    {
    case 0:
        solid_color.hue = HUE_AQUA;
        break;
    case 1:
        solid_color.hue = HUE_BLUE;
        break;
    case 2:
        solid_color.hue = HUE_GREEN;
        break;
    case 3:
        solid_color.hue = HUE_ORANGE;
        break;
    case 4:
        solid_color.hue = HUE_YELLOW;
        break;
    case 5:
        solid_color.hue = HUE_PINK;
        break;
    case 6:
        solid_color.hue = HUE_RED;
        break;
    }

    BRIGHTNESS_HIGH = map(this->brighness, 0, 9, 0, 255);

    EEPROM_save(solid_color.hue, BRIGHTNESS_HIGH);
}

/***********************************
* Encoder
***********************************/

struct encoder_t
{
    volatile uint8_t flag_R;
    volatile uint8_t flag_L;
    volatile uint8_t flag_BTN;
    volatile uint8_t state;
};

encoder_t encoder = {/*flag_R=*/0, /*flag_L=*/0, /*flag_BTN=*/0, /*state=*/0};

#define PIN_ENCODER_A PD2
#define PIN_ENCODER_B PD3
#define PIN_ENCODER_BTN PD4

/* Rotation handler */
void encoderRotH()
{
    const uint8_t pin_a_high = digitalRead(PIN_ENCODER_A);
    const uint8_t pin_b_high = digitalRead(PIN_ENCODER_B);

    if (!pin_a_high && pin_b_high)
        encoder.state = 1;
    if (!pin_a_high && !pin_b_high)
        encoder.state = 2;

    if (pin_a_high && encoder.state != 0)
    {
        if ((encoder.state == 1 && !pin_b_high) || (encoder.state == 2 && pin_b_high))
        {
            if (encoder.state == 1)
                encoder.flag_L = 1;
            if (encoder.state == 2)
                encoder.flag_R = 1;
            encoder.state = 0;
        }
    }
}

ISR(PCINT2_vect)
{
    if (digitalRead(PIN_ENCODER_BTN) == 1)
    {
        encoder.flag_BTN = 1;
    }
    else
    {
        encoder.flag_BTN = 0;
    }
}

static void encoder_init()
{
    pinMode(PIN_ENCODER_A, INPUT);
    pinMode(PIN_ENCODER_B, INPUT);
    pinMode(PIN_ENCODER_BTN, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderRotH, CHANGE);

    cli();
    PCICR |= 1u << PCIE2;
    PCMSK2 |= 1u << PCINT20;
    sei();
}

static void encoder_routine()
{
    if (encoder.flag_R)
    {
        activity_manager.current_activity->rotate(ENCODER_ROTATION_RIGHT);
        encoder.flag_R = 0;
    }

    if (encoder.flag_L)
    {
        activity_manager.current_activity->rotate(ENCODER_ROTATION_LEFT);
        encoder.flag_L = 0;
    }

    if (encoder.flag_BTN)
    {
        _delay_ms(5);
        if (digitalRead(PIN_ENCODER_BTN) == 1)
        {
            activity_manager.current_activity->press();
        }
        encoder.flag_BTN = 0;
    }
}

/***********************************
* Render
***********************************/

void IV6_scan()
{
    uint8_t byte1;
    uint8_t byte2;

    switch (scan_grid_n)
    {
    case 0:
        byte1 = IV6_numbers[display_bytes.byte_4][0] | IV6_grids[scan_grid_n][0];
        byte2 = IV6_numbers[display_bytes.byte_4][1] | IV6_grids[scan_grid_n][1];
        break;
    case 1:
        byte1 = IV6_numbers[display_bytes.byte_3][0] | IV6_grids[scan_grid_n][0];
        byte2 = IV6_numbers[display_bytes.byte_3][1] | IV6_grids[scan_grid_n][1];
        break;
    case 2:
        byte1 = IV6_numbers[display_bytes.byte_2][0] | IV6_grids[scan_grid_n][0];
        byte2 = IV6_numbers[display_bytes.byte_2][1] | IV6_grids[scan_grid_n][1];
        break;
    case 3:
        byte1 = IV6_numbers[display_bytes.byte_1][0] | IV6_grids[scan_grid_n][0];
        byte2 = IV6_numbers[display_bytes.byte_1][1] | IV6_grids[scan_grid_n][1];
        break;
    case 4:
        byte1 = IV6_numbers[display_bytes.byte_0][0] | IV6_grids[scan_grid_n][0];
        byte2 = IV6_numbers[display_bytes.byte_0][1] | IV6_grids[scan_grid_n][1];
        break;
    default:
        byte1 = SYMBOL_EMPTY;
        byte2 = SYMBOL_EMPTY;
    }

    digitalWrite(PIN_SR_LATCH, LOW);
    shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, (uint8_t)~byte1);
    shiftOut(PIN_SR_DATA, PIN_SR_CLOCK, MSBFIRST, (uint8_t)~byte2);
    digitalWrite(PIN_SR_LATCH, HIGH);

    scan_grid_n++;
    if (scan_grid_n > 4)
        scan_grid_n = 0;
}

static void display_render_routine()
{
    activity_manager.render();
}

void setup()
{
    Serial.begin(9600);
    Serial.println(F("Booting..."));

    pinMode(PIN_SR_DATA, OUTPUT);
    pinMode(PIN_SR_LATCH, OUTPUT);
    pinMode(PIN_SR_CLOCK, OUTPUT);

    pinMode(PIN_LDR, INPUT);

    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);

#ifdef FASTLED_ENABLED
    pinMode(PIN_WS21B_DATA, OUTPUT);

    CFastLED::addLeds<WS2812B, PIN_WS21B_DATA, GRB>(leds, NUM_LEDS);
    FastLED.setCorrection(CORRECTION);
    FastLED.setBrightness(BRIGHTNESS_HIGH);
    FastLED.setDither(LED_DITHER);

    solid_color.hue = eeprom_read_byte((uint8_t *)EEPROM_ADDR_COLOR);
    solid_color.saturation = 255;
    BRIGHTNESS_HIGH = eeprom_read_byte((uint8_t *)EEPROM_ADDR_BRIGHTNESS);
    solid_color.value = BRIGHTNESS_HIGH;

    fastled_render_routine();
#endif

    Wire.begin();

    pinMode(PIN_SQW, INPUT);
    clock.setClockMode(false);
    clock.enableOscillator(true, false, 0);

    _delay_ms(5);
    MsTimer2::set(2, IV6_scan);
    MsTimer2::start();

    encoder_init();

    main_menu_activity.set_menu(&main_menu);
    time_setup_activity.set_back_activity(&clock_activity);
    time_setup_activity.set_clock(&clock);
    color_setup_activity.set_back_activity(&clock_activity);
    activity_manager.set_current(&clock_activity);
}

void loop()
{
    if (millis() - render_timer > 100)
    {
        now = RTClib::now();
        render_timer = millis();
        display_render_routine();
        fastled_render_routine();
        ldr_routine();
    }

#ifdef DHT12_ENABLED
    if (millis() - temp_read_timer > 10000)
    {
        dht12.read();
        temp_read_timer = millis();
    }
#endif

    encoder_routine();
}
