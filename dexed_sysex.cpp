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

bool load_sysex(uint8_t bank, uint8_t voice_number)
{
  File root;
  bool found = false;

  voice_number %= 32;
  bank %= 10;

  if (sd_card_available)
  {
    char bankdir[3];

    bankdir[0] = '/';
    bankdir[2] = '\0';
    itoa(bank, &bankdir[1], 10);

    root = SD.open(bankdir);
    if (!root)
    {
      Serial.println(F("E: Cannot open main dir from SD."));
      return (false);
    }
    while (42 == 42)
    {
      File entry = root.openNextFile();
      if (!entry)
      {
        // No more files
        break;
      }
      else
      {
        if (!entry.isDirectory())
        {
          uint8_t data[128];
          found = true;
          if (get_sysex_voice(bankdir, entry, voice_number, data))
          {
            char n[11];
            strncpy(n, (char*)&data[118], 10);
            Serial.print(F("<"));
            Serial.print(entry.name());
            Serial.print(F("|'"));
            Serial.print(n);
            Serial.println(F("'>"));
            return (dexed->loadSysexVoice(data));
          }
          else
            Serial.println(F("E: Cannot load voice data"));
          entry.close();
          break;
        }
      }
    }
  }
  if (found == false)
    Serial.println(F("E: File not found."));

  return (false);
}

bool get_sysex_voice(char* dir, File sysex, uint8_t voice_number, uint8_t* data)
{
  File file;
  uint16_t i, n;
  uint32_t calc_checksum = 0;
  char sysex_file[20];

  strcpy(sysex_file, dir);
  strcat(sysex_file, "/");
  strcat(sysex_file, sysex.name());

  if (sysex.size() != 4104) // check sysex size
  {
    Serial.println(F("E: SysEx file size wrong."));
    return (false);
  }
  if (file = SD.open(sysex_file))
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
  else
  {
    Serial.print(F("Cannot open "));
    Serial.println(sysex.name());
  }
  return (true);
}
