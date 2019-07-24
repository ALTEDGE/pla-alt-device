#include "Joystick.h"
#include "lp55231.h"
#include "SparkFunSX1509.h"
#include "util/sx1509_registers.h"

// Uncomment to swap loop for serial dump of inputs.
//#define DEBUG

// The maximum range of a joystick axis
// Prototype limit: 675
#define JOY_LIMIT (1000)

// Creates the joystick object 
Joystick_ joy (JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
	11, 0,                // Button count, hat switch count
	true, true, true,     // X/Y/Z axis
	true, true, true,     // Rx/Ry/Rz axis
	false, true,          // Rudder, throttle (used for wheel)
	false, false, false); // Accelerator, brake, steering

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
	static bool lightsEnabled;
	// Tracks last pressed PG key, for light control.
	static unsigned int lastPressed;

public:
	/**
	 * Prepares the PG buttons and LEDs for use.
	 */
	static void begin(void) {
		io.begin(0x3E);
		io.clock();

		for (int i = 0; i < 16; i++) {
			io.pinMode(i, (i & 4) ? OUTPUT : INPUT_PULLUP);
			if (i & 4)
				io.digitalWrite(i, true);
		}
	}

	/**
	 * Enables or disables lighting LEDs according to the current PG.
	 * @param enable True to enable LEDs
	 */
	static void enableLights(bool enable) {
		lightsEnabled = enable;
		setLed(lastPressed, false);
		lastPressed = enable ? 0 : 8;
		setLed(lastPressed, true);
	}

	/**
	 * Sets the state of the given LED.
	 * @param n Index of the LED, 0-7
	 * @param state_ True to turn on the LED, false for off
	 */
	static void setLed(unsigned int n, bool state_) {
		if (n < 8)
			io.digitalWrite(leds[n], !state_);
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
			if (lightsEnabled && lastPressed != n && state) {
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
		for (int i = 0; i < 8; i++) {
			setLed(i, true);
			delay(75);
		}
		for (int i = 0; i < 8; i++) {
			setLed(i, false);
			delay(75);
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
void setup() {
	Serial.begin(9600);
	RgbLed::begin();
	PgButtons::begin();

	// Set joystick buttons to inputs
	pinMode(5, INPUT_PULLUP);
	pinMode(6, INPUT_PULLUP);
	pinMode(7, INPUT_PULLUP);

	// Set analog ranges for the joysticks and potentiometer
#ifndef DEBUG
	joy.setXAxisRange(0, JOY_LIMIT);
	joy.setYAxisRange(0, JOY_LIMIT);
	joy.setRxAxisRange(0, JOY_LIMIT);
	joy.setRyAxisRange(0, JOY_LIMIT);
	joy.setZAxisRange(0, JOY_LIMIT);
	joy.setRzAxisRange(0, JOY_LIMIT);
	joy.setThrottleRange(0, JOY_LIMIT); // Wheel

	// Begin functioning as a controller
	joy.begin();
#endif

	RgbLed::setAll(0x202020);
	PgButtons::testLeds();
}

#ifndef DEBUG

static void handleSerial(void);

/**
 * Main loop of the controller program.
 */
void loop() {
	// Check for serial request from computer
	if (Serial.available() > 0)
		handleSerial();

	// Update analog values
	joy.setXAxis(analogRead(2));  // Primary joystick
	joy.setYAxis(analogRead(3));
	joy.setRxAxis(analogRead(0)); // Left joystick
	joy.setRyAxis(analogRead(1));
	joy.setZAxis(analogRead(4));  // Right joystick
	joy.setRzAxis(analogRead(5));

	// Update wheel position
	static int wheelPosition = JOY_LIMIT / 2;
	static int lastPos = analogRead(6);
	int pos = analogRead(6);
	int diff = pos - lastPos;
	if (diff > 700)
		diff -= 1000;
	else if (diff < -700)
		diff += 1000;
	wheelPosition = max(min(wheelPosition + diff, JOY_LIMIT), 0);
	lastPos = pos;
	joy.setThrottle(wheelPosition);

	// Update digital values
	joy.setButton(0, !digitalRead(7));	 // left joystick button
	joy.setButton(1, !digitalRead(6));	 // center button
	joy.setButton(2, !digitalRead(5));	 // right button
	for (unsigned int i = 0; i < 8; i++)
		joy.setButton(3 + i, PgButtons::read(i));

	// Have some delay between updates (could be made shorter?)
	delay(20);
}

void handleSerial(void)
{
	unsigned int timeout = 500;
	unsigned long color;

	switch (Serial.read()) {
	// Change color
	case 'c':
		for (; Serial.available() < 3 && timeout > 0; timeout--)
			delay(1);
		if (timeout == 0)
			break;
		color = (((unsigned long)Serial.read() & 0xFF) << 16) |
			((Serial.read() & 0xFF) << 8) |
			(Serial.read() & 0xFF);
		RgbLed::setAll(color);
		break;
	// Enable PG lights
	case 'e':
		PgButtons::enableLights(true);
		break;
	// Disable PG lights
	case 'd':
		PgButtons::enableLights(false);
		break;
	// Identification
	case 'i':
		Serial.println("PLA");
		break;
	default:
		break;
	}
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
bool PgButtons::lightsEnabled = false;
unsigned int PgButtons::lastPressed = 8;

#else

// Debug loop
void loop() {
	char dump[100];
	char buffer[10];

	// Reset dump buffer; append joystick states
	dump[0] = '\0';
	for (int i = 0; i < 7; i++) {
		snprintf(10, buffer, "%5d ", analogRead(i));
		strcat(dump, buffer);
	}

	// Append joystick button states
	for (int i = 5; i < 8; i++) {
		snprintf(10, buffer, "%1d ", !digitalRead(i));
		strcat(dump, buffer);
	}

	// Append PG button states
	for (int i = 0; i < 8; i++) {
		snprintf(10, buffer, "%1d ", PgButtons::read(i));
		strcat(dump, buffer);
	}

	// Dump
	Serial.println(dump);
	delay(1500);
}

#endif // DEBUG

