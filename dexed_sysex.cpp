/*
 * MicroDexed
 *
 * MicroDexed is a port of the Dexed sound engine
 * (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield
 * 
 * (c)2018 H. Wirtz <wirtz@parasitstudio.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
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
            Serial.println(entry.name());
            check_sysex(entry);
            load_sysex_voice(entry, voice_number);
            entry.close();
            break;
          }
          entry.close();
        }
      }
    }
  }
}

bool load_sysex_voice(File sysex, uint8_t voice_number)
{
  File file;

  if (file = SD.open(sysex.name()))
  {
    uint8_t* p_data = dexed->data;
    uint8_t i;
    uint8_t tmp;

    dexed->notes_off();

    file.seek(6 + (voice_number * 128));
    for (i = 0; i < 6; i++)
    {
      file.read(p_data + (i * 21), 11); // R1, R2, R3, R4, L1, L2, L3, L4, LEV SCL BRK PT, SCL LEFT DEPTH, SCL RGHT DEPTH
      tmp = file.read();
      *(p_data + 11 + (i * 21)) = (tmp & 0x3);        // SCL LEFT CURVE
      *(p_data + 12 + (i * 21)) = (tmp & 0x0c) >> 2;  // SCL RGHT CURVE
      tmp = file.read();
      *(p_data + 20 + (i * 21)) = (tmp & 0x78) >> 3;  // DETUNE
      *(p_data + 13 + (i * 21)) = (tmp & 0x07);       // RS
      tmp = file.read();
      *(p_data + 15 + (i * 21)) = (tmp & 0x1c) >> 2;  // KVS
      *(p_data + 14 + (i * 21)) = (tmp & 0x03);       // AMS
      *(p_data + 16 + (i * 21)) = file.read();        // OUTPUT LVL
      tmp = file.read();
      *(p_data + 18 + (i * 21)) = (tmp & 0x3e) >> 1;  // FREQ_CORSE
      *(p_data + 17 + (i * 21)) = (tmp & 0x01);       // OP MODE
      file.read(p_data + 19 + (i * 21), 1);           // FREQ FINE
    }
    file.read(p_data + 125, 8); // PR1, PR2, PR3, PR4, PL1, PL2, PL3, PL4
    tmp = file.read();
    *(p_data + 134) = (tmp & 0x1f);         // ALG
    tmp = file.read();
    *(p_data + 136) = (tmp & 0x08) >> 3;    // OKS
    *(p_data + 135) = (tmp & 0x07);         // FB
    file.read(p_data + 137, 4);             // LFS, LFD, LPMD, LAMD
    tmp = file.read();
    *(p_data + 143) = (tmp & 0x30) >> 4;    // LFO PITCH MOD SENS
    *(p_data + 142) = (tmp & 0x0e) >> 1;    // LFO WAV
    *(p_data + 141) = (tmp & 0x01);         // LFO SYNC
    file.read(p_data + 144, 1);             // TRNSP
    file.read(p_data + 145, 10);            // NAME
    *(p_data + 155) = 1;        // PBEND RANGE
    *(p_data + 156) = 0;        // PBEND STEP
    *(p_data + 157) = 99;       // MOD RANGE
    *(p_data + 158) = 0;        // MOD ASSIGN
    *(p_data + 159) = 99;       // FOOT CTRL RANGE
    *(p_data + 160) = 0;        // FOOT CTRL ASSIGN
    *(p_data + 161) = 99;       // BREATH CTRL RANGE
    *(p_data + 162) = 0;        // BREATH CTRL ASSIGN
    *(p_data + 163) = 99;       // AT RANGE
    *(p_data + 164) = 0;        // AT ASSIGN
    *(p_data + 165) = 0;        // MASTER TUNE
    *(p_data + 166) = 1;        // OP1 ENABLE
    *(p_data + 167) = 1;        // OP2 ENABLE
    *(p_data + 168) = 1;        // OP3 ENABLE
    *(p_data + 169) = 1;        // OP4 ENABLE
    *(p_data + 170) = 1;        // OP5 ENABLE
    *(p_data + 171) = 1;        // OP6 ENABLE
    *(p_data + 172) = MAX_NOTES;

    dexed->setOPs((*(p_data + 166) << 5) | (*(p_data + 167) << 4) | (*(p_data + 168) << 3) | (*(p_data + 166) << 2) | (*(p_data + 170) << 1) | *(p_data + 171));
    dexed->setMaxNotes(*(p_data + 172));
    dexed->doRefreshVoice();
    dexed->activate();

#ifdef DEBUG
    char voicename[11];
    memset(voicename, 0, sizeof(voicename));
    strncpy(voicename, (char *)&dexed->data[145], sizeof(voicename) - 1);
    Serial.print(F("["));
    Serial.print(voicename);
    Serial.println(F("]"));
#endif

    return (true);
  }
  else
    return (false);
}

bool check_sysex(File sysex)
{
  File file;
  uint16_t i;
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
    for (i = 0; i < 4096; i++)
      calc_checksum += (file.read() & 0x7F); // calculate checksum
    calc_checksum = uint8_t(~calc_checksum + 1);
    if (calc_checksum != uint8_t(file.read()))
    {
      Serial.println(F("E: checksum mismatch."));
      return (false);
    }
  }
  file.close();
  return (true);
}
