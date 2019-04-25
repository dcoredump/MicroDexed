/*
   MicroDexed

   MicroDexed is a port of the Dexed sound engine
   (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield.
   Dexed ist heavily based on https://github.com/google/music-synthesizer-for-android

   (c)2018,2019 H. Wirtz <wirtz@parasitstudio.de>

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
#include "UI.h"

void create_sysex_filename(uint8_t b, char* sysex_file_name)
{
  // init and set name for actual bank
  memset(sysex_file_name, 0, 4 + VOICE_NAME_LEN);
  sysex_file_name[0] = '/';
  itoa(b, &sysex_file_name[1], 10);
  strcat(sysex_file_name, "/");
  strcat(sysex_file_name, bank_names[b]);
#ifdef DEBUG
  Serial.print(F("Created sysex_filename from bank "));
  Serial.print(b, DEC);
  Serial.print(F(" and name "));
  Serial.print(bank_names[b]);
  Serial.print(F(": ["));
  Serial.print(sysex_file_name);
  Serial.println(F("]"));
#endif
}

void strip_extension(char* s, char* target)
{
  char tmp[BANK_NAME_LEN];
  char* token;

  strcpy(tmp, s);
  token = strtok(tmp, ".");
  if (token == NULL)
    strcpy(target, "*ERROR*");
  else
    strcpy(target, token);
}

bool get_voice_names_from_bank(uint8_t b)
{
  File sysex;
  uint8_t voice_counter = 0;
  int32_t bulk_checksum_calc = 0;
  int8_t bulk_checksum;

  b %= MAX_BANKS;

  // erase all data for voice names
  memset(voice_names, 0, MAX_VOICES * VOICE_NAME_LEN);

  if (sd_card_available)
  {
    char sysex_file_name[4 + VOICE_NAME_LEN];

    // init and set name for actual bank
    create_sysex_filename(b, sysex_file_name);

#ifdef DEBUG
    Serial.print(F("Reading voice names for bank ["));
    Serial.print(sysex_file_name);
    Serial.println(F("]"));
#endif

    // try to open bank directory
    sysex = SD.open(sysex_file_name);
    if (!sysex)
      return (false);

    if (sysex.size() != 4104) // check sysex size
    {
#ifdef DEBUG
      Serial.println(F("E : SysEx file size wrong."));
#endif
      return (false);
    }
    if (sysex.read() != 0xf0)  // check sysex start-byte
    {
#ifdef DEBUG
      Serial.println(F("E : SysEx start byte not found."));
#endif
      return (false);
    }
    if (sysex.read() != 0x43)  // check sysex vendor is Yamaha
    {
#ifdef DEBUG
      Serial.println(F("E : SysEx vendor not Yamaha."));
#endif
      return (false);
    }
    sysex.seek(4103);
    if (sysex.read() != 0xf7)  // check sysex end-byte
    {
#ifdef DEBUG
      Serial.println(F("E : SysEx end byte not found."));
#endif
      return (false);
    }
    sysex.seek(3);
    if (sysex.read() != 0x09)  // check for sysex type (0x09=32 voices)
    {
#ifdef DEBUG
      Serial.println(F("E : SysEx type not 32 voices."));
#endif
      return (false);
    }
    sysex.seek(4102); // Bulk checksum
    bulk_checksum = sysex.read();

    sysex.seek(6); // start of bulk data

    for (uint16_t n = 0; n < 4096; n++)
    {
      uint8_t d = sysex.read();

      if ((n % 128) >= 118 && (n % 128) < 128) // found the start of the voicename
      {
        voice_names[voice_counter][(n % 128) - 118] = d;
      }
      if (n % 128 == 127)
        voice_counter++;
      bulk_checksum_calc -= d;
    }
    bulk_checksum_calc &= 0x7f;

#ifdef DEBUG
    Serial.print(F("Bulk checksum : 0x"));
    Serial.print(bulk_checksum_calc, HEX);
    Serial.print(F(" [0x"));
    Serial.print(bulk_checksum, HEX);
    Serial.println(F("]"));
#endif

    if (bulk_checksum_calc != bulk_checksum)
    {
#ifdef DEBUG
      Serial.print(F("E : Bulk checksum mismatch : 0x"));
      Serial.print(bulk_checksum_calc, HEX);
      Serial.print(F(" != 0x"));
      Serial.println(bulk_checksum, HEX);
#endif
      return (false);
    }
  }

  return (false);
}

uint8_t get_bank_names(void)
{
  File root;
  uint8_t bank_counter = 0;

  // erase all data for bank names
  memset(bank_names, 0, MAX_BANKS * BANK_NAME_LEN);

  if (sd_card_available)
  {
    char bankdir[4];

    do
    {
      // init and set name for actual bank directory
      memset(bankdir, 0, sizeof(bankdir));
      bankdir[0] = '/';
      itoa(bank_counter, &bankdir[1], 10);

      // try to open directory
      root = SD.open(bankdir);
      if (!root)
        break;

      // read filenames
      File entry = root.openNextFile();
      if (!entry.isDirectory())
      {
        strcpy(bank_names[bank_counter], entry.name());
#ifdef DEBUG
        Serial.print(F("Found bank ["));
        Serial.print(bank_names[bank_counter]);
        Serial.println(F("]"));
#endif
        bank_counter++;
      }
    } while (root);

    return (bank_counter);
  }
  else
    return (bank_counter);
}

bool load_sysex(uint8_t b, uint8_t v)
{
#if DEBUG
  bool found = false;
#endif
  v %= MAX_VOICES;
  b %= MAX_BANKS;

  if (sd_card_available)
  {
    File sysex;
    char sysex_file_name[4 + VOICE_NAME_LEN];
    uint8_t data[128];

    create_sysex_filename(b, sysex_file_name);

    sysex = SD.open(sysex_file_name);
    if (!sysex)
    {
#ifdef DEBUG
      Serial.print(F("E : Cannot open "));
      Serial.print(sysex_file_name);
      Serial.println(F("from SD."));
#endif
      return (false);
    }

    if (get_sysex_voice(sysex, v, data))
    {
#ifdef DEBUG
      char n[11];

      strncpy(n, (char*)&data[118], 10);
      Serial.print("Loading sysex ");
      Serial.print(sysex_file_name);
      Serial.print(F(" ["));
      Serial.print(voice_names[v]);
      Serial.println(F("]"));
#endif
      return (dexed->loadSysexVoice(data));
    }
#ifdef DEBUG
    else
      Serial.println(F("E : Cannot load voice data"));
#endif
  }
#ifdef DEBUG
  if (found == false)
    Serial.println(F("E : File not found."));
#endif

  return (false);
}

bool get_sysex_voice(File sysex, uint8_t voice_number, uint8_t* data)
{
  uint16_t n;
  int32_t bulk_checksum_calc = 0;
  int8_t bulk_checksum;

  if (sysex.size() != 4104) // check sysex size
  {
#ifdef DEBUG
    Serial.println(F("E : SysEx file size wrong."));
#endif
    return (false);
  }
  if (sysex.read() != 0xf0)  // check sysex start-byte
  {
#ifdef DEBUG
    Serial.println(F("E : SysEx start byte not found."));
#endif
    return (false);
  }
  if (sysex.read() != 0x43)  // check sysex vendor is Yamaha
  {
#ifdef DEBUG
    Serial.println(F("E : SysEx vendor not Yamaha."));
#endif
    return (false);
  }
  sysex.seek(4103);
  if (sysex.read() != 0xf7)  // check sysex end-byte
  {
#ifdef DEBUG
    Serial.println(F("E : SysEx end byte not found."));
#endif
    return (false);
  }
  sysex.seek(3);
  if (sysex.read() != 0x09)  // check for sysex type (0x09=32 voices)
  {
#ifdef DEBUG
    Serial.println(F("E : SysEx type not 32 voices."));
#endif
    return (false);
  }
  sysex.seek(4102); // Bulk checksum
  bulk_checksum = sysex.read();

  sysex.seek(6); // start of bulk data
  for (n = 0; n < 4096; n++)
  {
    uint8_t d = sysex.read();
    if (n >= voice_number * 128 && n < (voice_number + 1) * 128)
    {
      data[n - (voice_number * 128)] = d;
    }
    bulk_checksum_calc -= d;
  }
  bulk_checksum_calc &= 0x7f;

#ifdef DEBUG
  Serial.print(F("Bulk checksum : 0x"));
  Serial.print(bulk_checksum_calc, HEX);
  Serial.print(F(" [0x"));
  Serial.print(bulk_checksum, HEX);
  Serial.println(F("]"));
#endif

  if (bulk_checksum_calc != bulk_checksum)
  {
#ifdef DEBUG
    Serial.print(F("E : Bulk checksum mismatch : 0x"));
    Serial.print(bulk_checksum_calc, HEX);
    Serial.print(F(" != 0x"));
    Serial.println(bulk_checksum, HEX);
#endif
    return (false);
  }

  render_time_max = 0;

  return (true);
}
