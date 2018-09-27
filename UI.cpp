/*
   MicroDexed

   MicroDexed is a port of the Dexed sound engine
   (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield.
   Dexed ist heavily based on https://github.com/google/music-synthesizer-for-android

   (c)2018 H. Wirtz <wirtz@parasitstudio.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

*/

#include <Arduino.h>
#include "config.h"
#include "UI.h"

#ifdef I2C_DISPLAY // selecting sounds by encoder, button and display

void handle_ui(void)
{
  for (uint8_t i = 0; i < NUM_ENCODER; i++)
  {
    but[i].update();

    if (but[i].risingEdge())
    {
      // Button pressed
#ifdef DEBUG
      Serial.print(F("Button "));
      Serial.println(i, DEC);
#endif
    }

    if (enc[i].read() == enc_val[i])
      continue;
    else
    {
#ifdef DEBUG
      Serial.print(F("Encoder "));
      Serial.print(i, DEC);
      Serial.print(F(": "));
      Serial.println(enc[i].read(), DEC);
#endif
    }
    enc_val[i] = enc[i].read();
  }
}

/*int32_t getEncPosition(uint8_t encoder_number)
{
  return enc[encoder_number].read() / 4;
}

void setEncPosition(uint8_t encoder_number, int32_t value)
{
  enc[encoder_number].write(value * 4);
  enc_val[encoder_number] = value * 4;
}*/

#endif
