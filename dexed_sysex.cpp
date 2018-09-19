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
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "dexed.h"
#include "dexed_sysex.h"
#include "config.h"

bool load_sysex(uint8_t b, uint8_t v)
{
  File root;
  bool found = false;

  v %= 32;
  b %= 10;

  if (sd_card_available)
  {
    char bankdir[3];

    bankdir[0] = '/';
    bankdir[2] = '\0';
    itoa(b, &bankdir[1], 10);

    root = SD.open(bankdir);
    if (!root)
    {
#ifdef DEBUG
      Serial.println(F("E: Cannot open main dir from SD."));
#endif
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
          if (get_sysex_voice(bankdir, entry, v, data))
          {
#ifdef DEBUG
            char n[11];
            strncpy(n, (char*)&data[118], 10);
            Serial.print("Loading sysex ");
            Serial.print(bankdir);
            Serial.print("/");
            Serial.print(entry.name());
            Serial.print(F(" ["));
            Serial.print(n);
            Serial.println(F("]"));
            strcpy(bank_name, bankdir);
            strcpy(voice_name, entry.name());
#endif
            return (dexed->loadSysexVoice(data));
          }
          else
#ifdef DEBUG
            Serial.println(F("E: Cannot load voice data"));
#endif
          entry.close();
          break;
        }
      }
    }
  }
#ifdef I2C_DISPLAY
  char tmp[3];
  itoa(bank, tmp, 10);
  lcd.show(0, 0, 2, tmp);
  lcd.show(0, 3, 10, bank_name);
  itoa(voice, tmp, 10);
  lcd.show(1, 0, 2, tmp);
  lcd.show(1, 3, 10, voice_name);
#endif

#ifdef DEBUG
  if (found == false)
    Serial.println(F("E: File not found."));
#endif

  return (false);
}

bool get_sysex_voice(char* dir, File sysex, uint8_t voice_number, uint8_t* data)
{
  File file;
  uint16_t n;
  int32_t bulk_checksum_calc = 0;
  int8_t bulk_checksum;
  char sysex_file[20];

  strcpy(sysex_file, dir);
  strcat(sysex_file, "/");
  strcat(sysex_file, sysex.name());

  if (sysex.size() != 4104) // check sysex size
  {
#ifdef DEBUG
    Serial.println(F("E: SysEx file size wrong."));
#endif
    return (false);
  }
  if (file = SD.open(sysex_file))
  {
    if (file.read() != 0xf0)  // check sysex start-byte
    {
#ifdef DEBUG
      Serial.println(F("E: SysEx start byte not found."));
#endif
      return (false);
    }
    if (file.read() != 0x43)  // check sysex vendor is Yamaha
    {
#ifdef DEBUG
      Serial.println(F("E: SysEx vendor not Yamaha."));
#endif
      return (false);
    }
    file.seek(4103);
    if (file.read() != 0xf7)  // check sysex end-byte
    {
#ifdef DEBUG
      Serial.println(F("E: SysEx end byte not found."));
#endif
      return (false);
    }
    file.seek(3);
    if (file.read() != 0x09)  // check for sysex type (0x09=32 voices)
    {
#ifdef DEBUG
      Serial.println(F("E: SysEx type not 32 voices."));
#endif
      return (false);
    }
    file.seek(4102); // Bulk checksum
    bulk_checksum = file.read();

    file.seek(6); // start of bulk data
    for (n = 0; n < 4096; n++)
    {
      uint8_t d = file.read();
      if (n >= voice_number * 128 && n < (voice_number + 1) * 128)
      {
        data[n - (voice_number * 128)] = d;
      }
      bulk_checksum_calc -= d;
    }
    bulk_checksum_calc &= 0x7f;

#ifdef DEBUG
    Serial.print(F("Bulk checksum: 0x"));
    Serial.print(bulk_checksum_calc, HEX);
    Serial.print(F(" [0x"));
    Serial.print(bulk_checksum, HEX);
    Serial.println(F("]"));
#endif

    if (bulk_checksum_calc != bulk_checksum)
    {
#ifdef DEBUG
      Serial.print(F("E: Bulk checksum mismatch: 0x"));
      Serial.print(bulk_checksum_calc, HEX);
      Serial.print(F(" != 0x"));
      Serial.println(bulk_checksum, HEX);
#endif
      return (false);
    }
  }
#ifdef DEBUG
  else
  {
    Serial.print(F("Cannot open "));
    Serial.println(sysex.name());
  }
#endif

  render_time_max = 0;

  return (true);
}
