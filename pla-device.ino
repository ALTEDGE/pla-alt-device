#include "Joystick.h"
#include "lp55231.h"
#include "SparkFunSX1509.h"
#include "util/sx1509_registers.h"

// Uncomment for using a second LP55231
#define RGB2

// Uncomment to disable the IO expander (PWM LEDs + Buttons)
//#define NO_IOE

// Creates the joystick object 
Joystick_ joy (JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
  11, 0,                // Button count, hat switch count
  true, true, true,     // X/Y/Z axis
  true, true, true,     // Rx/Ry/Rz axis
  false, true,          // Rudder, throttle (used for wheel)
  false, false, false); // Accelerator, brake, steering

// IO Expander object
#ifndef NO_IOE
SX1509 ioe;
#endif 

// RGB objects
Lp55231 rgb (0x32);

#ifdef RGB2
Lp55231 rgb2 (0x33);
#endif

// Contains the port on the IO expander for each LED
int ledPorts[8] = {
  15, 14, 13, 12, 7, 6, 5, 4
};

// Defines the RGB channels for each LED on the LED driver (LP55231)
const char pwmChannels[9] = {
  6, 0, 1, // R, G, B
  7, 2, 3,
  8, 4, 5,
};

// Sets the given LED to a brightness value following a sine
// curve, for a breathing effect
void updateLED(unsigned int led, float x)
{
#ifndef NO_IOE
  if (led >= 8)
    return;
  ioe.analogWrite(ledPorts[led], abs(120 * sin(x)));
#else
  (void)led;
  (void)x;
#endif
}

/**
 * Sets the given LED on the LED driver to the desired RGB value.
 * @param led the LED to set, 0-2
 * @param rgb24 the 24-bit RGB value to write
 */
void setRGB(unsigned int led, unsigned long int rgb24)
{
#ifdef RGB2
  if (led > 2) {
    led -= 3;
    for (int i = 0; i < 3; i++)
      rgb2.SetChannelPWM(pwmChannels[led * 3 + i], (rgb24 >> (8 * (2 - i))) & 0xFF);  
    return;
  }
#endif
  for (int i = 0; i < 3; i++)
    rgb.SetChannelPWM(pwmChannels[led * 3 + i], (rgb24 >> (8 * (2 - i))) & 0xFF);
}

/**
 * Arduino setup and initialization.
 */
void setup() {
  // Start serial
  Serial.begin(9600);

  // Start the LED driver
  rgb.Begin();
  rgb.Enable();
#ifdef RGB2
  rgb2.Begin();
  rgb2.Enable();
#endif

#ifndef NO_IOE
  // Start the IO expander
  ioe.begin(0x3E);
  ioe.clock();
#endif
  
  // Set pin modes for all buttons
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
#ifndef NO_IOE
  for (int i = 0; i < 4; i++)
    ioe.pinMode(i, INPUT_PULLUP);
  for (int i = 8; i < 12; i++)
    ioe.pinMode(i, INPUT_PULLUP);

  // Set pin modes for the PWM'd LEDs
  ioe.writeByte(REG_OPEN_DRAIN_A, 0xF0);
  ioe.writeByte(REG_OPEN_DRAIN_B, 0xF0);
  for (int i = 4; i < 8; i++)
    ioe.ledDriverInit(i);
  for (int i = 12; i < 16; i++)
    ioe.ledDriverInit(i);
    
#endif

  // Set analog ranges for the joysticks and potentiometer
  // Prototype limit: 675
#if 1
  #define JOY_LIMIT 1000
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

  // TEST - put RGB LEDs to various colors
  setRGB(0, 0x040004);
  setRGB(1, 0x000404);
  setRGB(2, 0x040400);
}

#if 0

void loop() {
  char buffer[50];
  sprintf(buffer, "%5d %5d %5d %5d %5d %5d %5d",
  analogRead(0), analogRead(1), analogRead(2), 
  analogRead(3), analogRead(4), analogRead(5),
  analogRead(6));
  Serial.println(buffer);
  delay(1500);
}

#else

/**
 * Main loop of the controller program.
 */
void loop() {
#ifndef NO_IOE
  // Update LED fades
  static float x = 0;
  for (unsigned int i = 0; i < 8; i++)
    updateLED(i, x);
  x += 0.05f;
#endif

  // Check for serial request from computer
  unsigned long color;
  unsigned int timeout;
  if (Serial.available() > 0) {
    switch (Serial.read()) {
    case 'c': // change color
      for (timeout = 500; Serial.available() < 3 && timeout > 0; timeout--)
        delay(1);
      if (timeout == 0)
        break;
      color = (((unsigned long)Serial.read() & 0xFF) << 16) |
        ((Serial.read() & 0xFF) << 8) |
        (Serial.read() & 0xFF);
#ifdef RGB2
      for (int i = 0; i < 6; i++)
#else
      for (int i = 0; i < 3; i++)
#endif
        setRGB(i, color);
      break;
    case 'i':
      Serial.println("PLA");
      break;
    default:
      break;
    }
  }

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

  // Update analog values
  joy.setXAxis(analogRead(2));        // center joystick
  joy.setYAxis(analogRead(3));
  joy.setRxAxis(analogRead(0));       // left joystick
  joy.setRyAxis(analogRead(1));
  joy.setZAxis(analogRead(4));  // right joystick
  joy.setRzAxis(analogRead(5));

  // Update digital values
  joy.setButton(0, !digitalRead(7));   // left joystick button
  joy.setButton(1, !digitalRead(6));   // center button
  joy.setButton(2, !digitalRead(5));   // right button
#ifndef NO_IOE
  for (unsigned int i = 0; i < 4; i++)
    joy.setButton(i + 3, !ioe.digitalRead(i));
  for (unsigned int i = 8; i < 12; i++)
    joy.setButton(i - 1, !ioe.digitalRead(i));
#endif

  // Have some delay between updates (could be made shorter?)
  delay(25);
}

#endif
