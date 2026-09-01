#ifndef LP5861_H
#define LP5861_H

#include <Wire.h>

struct Lp5861
{
  Lp5861(int addr):
    m_addr(0x40 | ((addr & 3) << 2)) {}

  int Begin() {
    int r = Set8(0x0A9, 0xFF); // Software reset
    if (r)
      return r;

    delay(1);
    r |= Enable();
    r |= Set8(0x004, 4 << 1); // 20mA max current
    r |= Set8(0x001, 0); // Mode 1, 125 kHz PWM
    return r;
  }

  int Enable() {
    int r = Set8(0x000, 1); // Enable
    delay(1);
    return r;
  }

  int Disable() {
    return Set8(0x000, 0); // Disable
  }

  int SetChannelPWM(int channel, unsigned char intensity) {
    if (channel >= 18)
      return;

    return Set8(0x200 + channel, intensity); // 8-bit PWM
  }

  int Set8(int reg, unsigned char val) {
    int r;

    Wire.beginTransmission(m_addr | ((reg >> 8) & 0x3));
    Wire.write(reg & 0xFF);
    Wire.write(val);
    r = Wire.endTransmission();

    delayMicroseconds(10);
    return r;
  }

  int m_addr;
};

#endif // LP5861_H
