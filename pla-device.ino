#include "Joystick.h"
#include "lp55231.h"
#include "SparkFunSX1509.h"
#include "util/sx1509_registers.h"

// Uncomment to swap loop for serial dump of inputs
//#define DEBUG

// Default controller color
#define COLOR_DEFAULT (0x03F7FF)

// Number of milliseconds between updating state
#define JOY_TIMEDELTA (10)

// Joystick and wheel range definitions
#define JOY_RANGELOW  (512 - 250)
#define JOY_RANGEHIGH (512 + 250)
#define WHL_RANGE     (80)
#define WHL_RANGELOW  (512 - 80)
#define WHL_RANGEHIGH (512 + 80)
#define WHL_THRESHOLD (10)

// Creates the joystick object 
static Joystick_ joy (JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
    11, 0,                // Button count, hat switch count
    true, true, true,     // X/Y/Z axis
    true, true, true,     // Rx/Ry/Rz axis
    false, true,          // Rudder, throttle (used for wheel)
    false, false, false); // Accelerator, brake, steering
static int wheelCenter = -1;

/**
 * @class PgButtons
 * @brief Handles and provides control over the PG buttons and LEDs.
 */
class PgButtons {
private:
    // Object for the IO Expander that controls the buttons and LEDs.
    static SX1509 io;

    // Table to translate a generic index of LEDs to their proper pins.
    static const int leds[8];
    // Table to translate generic index of buttons to their proper pins.
    static const int buttons[8];

    // If true, light LED corresponding to PG key. If false, no LEDs.
    static bool trackingPG;
    // Tracks last pressed PG key, for light control.
    static unsigned int lastPressed;

public:
    /**
     * Prepares the PG buttons and LEDs for use.
     */
    static void begin(void) {
        io.begin(0x3E);

        // Button initialization
        for (int i = 0; i < 8; i++)
            io.pinMode(buttons[i], INPUT_PULLUP);

        if (trackingPG) {
            for (int i = 0; i < 8; i++) {
                 io.pinMode(leds[i], OUTPUT);
                setLed(i, false);
            }
            setLed(lastPressed, true);
        } else {
            io.clock(INTERNAL_CLOCK_2MHZ, 3);
            io.writeByte(REG_OPEN_DRAIN_A, 0xF0);
            io.writeByte(REG_OPEN_DRAIN_B, 0xF0);
            for (int i = 0; i < 8; i++) {
                io.ledDriverInit(leds[i]);
                io.breathe(leds[i], 750, 250, 2000, 2000);
            }
        }
    }

    /**
     * Enables or disables lighting LEDs according to the current PG.
     * @param enable True to enable LEDs
     */
    static void trackPG(bool enable) {
        trackingPG = enable;
        io.reset(true);
        begin();
    }

    static bool isTrackingPG(void) {
        return trackingPG;
    }

    /**
     * Sets the state of the given LED.
     * @param n Index of the LED, 0-7
     * @param state True to turn on the LED, false for off
     */
    static void setLed(unsigned int n, bool state) {
        if (n < 8)
            io.digitalWrite(leds[n], !state);
    }

    /**
     * Reads the state of the given button.
     * @param n Index of the button, 0-7
     * @return True if the button is in a pressed state
     */
    static bool read(unsigned int n) {
        if (n < 8) {
             bool state = !io.digitalRead(buttons[n]);
            // Update LEDs if a new PG button is pressed
            if (trackingPG && lastPressed != n && state) {
                setLed(lastPressed, false);
                setLed(n, true);
                lastPressed = n;
            }
            return state;
        } else {
            return false;
        }
    }

    /**
     * Runs an LED sequence, so that the user can check for faulty LEDs.
     */
    static void testLeds(void) {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 8; j++)
                setLed(j, true);
            delay(50);
            for (int j = 0; j < 8; j++)
                setLed(j, false);
            delay(50);
        }
    }
};

/**
 * @class RgbLed
 * @brief Handles and controls the RGB LEDs.
 */
class RgbLed {
private:
    // Object for controlling the RGB LED driver chips.
    static Lp55231 rgb[2];

    // Defines the RGB channels for each LED on the LED chip.
    static const char channel[9];

public:
    /**
     * Prepares the RGB LEDs for use.
     */
    static void begin(void) {
        for (int i = 0; i < 2; i++) {
            rgb[i].Begin();
            rgb[i].Enable();
        }
    }

    /**
     * Sets the given LED on the LED driver to the desired RGB value.
     * @param n the LED to set, 0-5
     * @param rgb24 the 24-bit RGB value to write
     */
    static void set(unsigned int n, unsigned long rgb24) {
        int index = 0;
        if (n > 2) {
            index = 1;
            n -= 3;
        }

        for (int i = 0; i < 3; i++) {
            rgb[index].SetChannelPWM(channel[n * 3 + i],
                (rgb24 >> (8 * (2 - i))) & 0xFF);
        }
    }

    /**
     * Sets all LEDs to the given color.
     * @param rgb24 The 24-bit RGB value to write
     */
    static void setAll(unsigned long rgb24) {
        for (unsigned int i = 0; i < 6; i++)
            set(i, rgb24);
    }
};

/**
 * Arduino setup and initialization.
 */
static void enterTestMode();
void setup() {
    // Enable entering sleep mode
    SMCR = 1;

    Serial.begin(115200);

    // Set joystick buttons to inputs
    pinMode(5, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);

    // Set analog ranges for the joysticks and potentiometer
#ifndef DEBUG
    RgbLed::begin();
    PgButtons::begin();
    
    joy.setXAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setYAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setRxAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setRyAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setZAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setRzAxisRange(JOY_RANGELOW, JOY_RANGEHIGH);
    joy.setThrottleRange(WHL_RANGELOW, WHL_RANGEHIGH); // Wheel

    // Calibrate the wheel
    wheelCenter = 0;
    for (int i = 0; i < 8; ++i) {
        wheelCenter += analogRead(6);
        delay(1);
    }
    wheelCenter = wheelCenter / 8 - 512;

    // Begin functioning as a controller
    joy.begin();

    if (!digitalRead(6)) {
        PgButtons::trackPG(true);
        PgButtons::testLeds();
        enterTestMode();
    }

    RgbLed::setAll(COLOR_DEFAULT);
#endif
}


Lp55231 RgbLed::rgb[2] = {
    Lp55231(0x32), Lp55231(0x33)
};
const char RgbLed::channel[9] = {
    6, 0, 1, // R, G, B
    7, 2, 3,
    8, 4, 5,
};

SX1509 PgButtons::io;
const int PgButtons::leds[8] = {
    6, 7, 12, 13, 14, 15, 4, 5
};
const int PgButtons::buttons[8] = {
    0, 1, 2, 3, 8, 9, 11, 10
};
bool PgButtons::trackingPG = false;
unsigned int PgButtons::lastPressed = 0;

#ifndef DEBUG

static void handleSerial(void);

/**
 * Main loop of the controller program.
 */
void loop() {
    unsigned long updateTimeTarget = millis() + JOY_TIMEDELTA;

    // Check for serial request from computer
    if (Serial.available() > 0)
        handleSerial();

    // Update analog values
    joy.setXAxis(analogRead(2));  // Primary joystick
    joy.setYAxis(analogRead(3));
    joy.setRxAxis(analogRead(0)); // Left joystick
    joy.setRyAxis(analogRead(1));
    joy.setZAxis(1000 - analogRead(4));  // Right joystick (actually primary?)
    joy.setRzAxis(1000 - analogRead(5));

    // Update wheel position
    int wheel = analogRead(6) - wheelCenter;
    if (wheel < 512 + WHL_THRESHOLD && wheel > 512 - WHL_THRESHOLD)
      wheel = 512;
    joy.setThrottle(wheel);

    // Update digital values
    joy.setButton(0, !digitalRead(7));   // left joystick button
    joy.setButton(1, !digitalRead(6));   // center button
    joy.setButton(2, !digitalRead(5));   // right button
    for (unsigned int i = 0; i < 8; i++)
        joy.setButton(3 + i, PgButtons::read(i));

    // Sleep if we have extra time
    while (millis() < updateTimeTarget)
        asm("sleep");
}

void enterTestMode()
{   
    while (1) {
        auto joyToColor = [](int joy, unsigned long int colorl, unsigned long int colorr) {
            bool neg = joy < 512;
            joy = neg ? 512 - joy : joy - 512;
            if (joy > 510)
                joy = 510;

            unsigned long int mask = 0;
            if (joy > 50) {
                mask = (joy / 2) & 0xFF;
                mask |= (mask << 8) | (mask << 16);
                mask = (neg ? colorl : colorr) & mask;
            }
            return mask;
        };
        unsigned long int mask = 0;
        
        // Left        
        if (!digitalRead(5)) {
            mask = 0xFFFFFF;
        } else {
            mask = joyToColor(analogRead(0), 0xFFFF00, 0x00FF00) |
                   joyToColor(analogRead(1), 0x0000FF, 0xFF0000);
        }
        RgbLed::set(2, mask);
        RgbLed::set(3, mask);
        // Primary
        if (!digitalRead(6)) {
            mask = 0xFFFFFF;
        } else {
            mask = joyToColor(analogRead(2), 0xFFFF00, 0x00FF00) |
                   joyToColor(analogRead(3), 0x0000FF, 0xFF0000) |
                   joyToColor(analogRead(6), 0xFF00FF, 0x00FFFF);
        }
        RgbLed::set(4, mask);
        RgbLed::set(5, mask);
        // Right
        if (!digitalRead(7)) {
            mask = 0xFFFFFF;
        } else {
            mask = joyToColor(1024 - analogRead(4), 0xFFFF00, 0x00FF00) |
                   joyToColor(1024 - analogRead(5), 0x0000FF, 0xFF0000);
        }
        RgbLed::set(0, mask);
        RgbLed::set(1, mask);
        
        for (unsigned int i = 0; i < 8; i++)
          PgButtons::read(i);

        delay(100);
    }
}

void handleSerial(void)
{
    switch (Serial.read()) {
    // Change color
    case 'c':
        {
            unsigned int timeout = 500;
            unsigned long color;
            for (; Serial.available() < 3 && timeout > 0; --timeout)
                delay(1);
            if (timeout == 0)
                break;
            color = (Serial.read() & 0xFF) << 16;
            color |= (Serial.read() & 0xFF) << 8;
            color |= Serial.read() & 0xFF;
            RgbLed::setAll(color);
        }
        break;
    // Enable PG lights
    case 'e':
        PgButtons::trackPG(true);
        break;
    // Disable PG lights
    case 'd':
        PgButtons::trackPG(false);
        RgbLed::setAll(COLOR_DEFAULT);
        break;
    // Identification
    case 'i':
        Serial.println("PLA");
        break;
    default:
        break;
    }
}

#else

// Debug loop
void loop() {
    for (int i = 0; i < 7; i++) {
        Serial.print(analogRead(i));
        Serial.print(", ");
    }

    // Append joystick button states
    for (int i = 5; i < 8; i++) {
        Serial.print(!digitalRead(i));
        Serial.print(", ");
    }

    // Append PG button states
    for (int i = 0; i < 8; i++) {
        Serial.print(PgButtons::read(i));
        Serial.print(", ");
    }

    Serial.println();
    delay(1000);
}

#endif // DEBUG
