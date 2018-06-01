/*
   MicroDexed

   MicroDexed is a port of the Dexed sound engine
   (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield

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
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "dexed.h"
#include "dexed_sysex.h"
#include "config.h"

void load_sysex(char *name, uint8_t voice_number)
{
  File root;

  if (sd_card_available)
  {
    root = SD.open("/");
    while (42 == 42)
    {
      File entry = root.openNextFile();
      if (!entry)
        break;
      else
      {
        if (!entry.isDirectory())
        {
          if (strcmp(name, entry.name()) == 0)
          {
            uint8_t data[128];

            Serial.println(entry.name());
            if (get_sysex_voice(entry, voice_number, data))
              dexed->loadSysexVoice(data);
            else
              Serial.println(F("Cannot load voice data"));
            entry.close();
            break;
          }
          entry.close();
        }
      }
    }
  }
}

bool get_sysex_voice(File sysex, uint8_t voice_number, uint8_t* data)
{
  File file;
  uint16_t i,n;
  uint32_t calc_checksum = 0;

  if (sysex.size() != 4104) // check sysex size
    return (false);
  if (file = SD.open(sysex.name()))
  {
    if (file.read() != 0xf0)  // check sysex start-byte
    {
      Serial.println(F("E: SysEx start byte not found."));
      return (false);
    }
    if (file.read() != 0x43)  // check sysex vendor is Yamaha
    {
      Serial.println(F("E: SysEx vendor not Yamaha."));
      return (false);
    }
    file.seek(4103);
    if (file.read() != 0xf7)  // check sysex end-byte
    {
      Serial.println(F("E: SysEx end byte not found."));
      return (false);
    }
    file.seek(3);
    if (file.read() != 0x09)  // check for sysex type (0x09=32 voices)
    {
      Serial.println(F("E: SysEx type not 32 voices."));
      return (false);
    }
    file.seek(6);             // start of 32*128 (=4096) bytes data
    for (i = 0; i < 32; i++)
    {
      for (n = 0; n < 128; n++)
      {
        uint8_t d = file.read();
        if (i == voice_number)
        {
          data[n] = d;
        }
        calc_checksum += (d & 0x7F); // calculate checksum
      }
    }
    calc_checksum = uint8_t(~calc_checksum + 1);
    if (calc_checksum != uint8_t(file.read()))
    {
      Serial.println(F("E: checksum mismatch."));
      return (false);
    }
  }
  return (true);
}
