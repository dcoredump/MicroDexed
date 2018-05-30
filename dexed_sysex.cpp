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
    uint8_t op;
    uint8_t tmp;

    dexed->notes_off();

    file.seek(6 + (voice_number * 128));
    for (op = 0; op < 6; op++)
    {
      //  DEXED_OP_EG_R1,           // 0
      //	DEXED_OP_EG_R2,           // 1
      //	DEXED_OP_EG_R3,           // 2
      //	DEXED_OP_EG_R4,           // 3
      //	DEXED_OP_EG_L1,           // 4
      //	DEXED_OP_EG_L2,           // 5
      //	DEXED_OP_EG_L3,           // 6
      //	DEXED_OP_EG_L4,           // 7
      //	DEXED_OP_LEV_SCL_BRK_PT,  // 8
      //	DEXED_OP_SCL_LEFT_DEPTH,  // 9
      //	DEXED_OP_SCL_RGHT_DEPTH,  // 10
      file.read(p_data + (op * 21) + DEXED_OP_EG_R1, 11);
      tmp = file.read();
      *(p_data + DEXED_OP_SCL_LEFT_CURVE + (op * 21)) = (tmp & 0x3);
      *(p_data + DEXED_OP_SCL_RGHT_CURVE + (op * 21)) = (tmp & 0x0c) >> 2;
      tmp = file.read();
      *(p_data + DEXED_OP_OSC_DETUNE + (op * 21)) = (tmp & 0x78) >> 3;
      *(p_data + DEXED_OP_OSC_RATE_SCALE + (op * 21)) = (tmp & 0x07);
      tmp = file.read();
      *(p_data + DEXED_OP_KEY_VEL_SENS + (op * 21)) = (tmp & 0x1c) >> 2;
      *(p_data + DEXED_OP_AMP_MOD_SENS + (op * 21)) = (tmp & 0x03);
      *(p_data + DEXED_OP_OUTPUT_LEV + (op * 21)) = file.read();
      tmp = file.read();
      *(p_data + DEXED_OP_FREQ_COARSE + (op * 21)) = (tmp & 0x3e) >> 1;
      *(p_data + DEXED_OP_OSC_MODE + (op * 21)) = (tmp & 0x01);
      file.read(p_data + DEXED_OP_FREQ_FINE + (op * 21), 1);
    }
    //  DEXED_PITCH_EG_R1,        // 0
    //  DEXED_PITCH_EG_R2,        // 1
    //  DEXED_PITCH_EG_R3,        // 2
    //  DEXED_PITCH_EG_R4,        // 3
    //  DEXED_PITCH_EG_L1,        // 4
    //  DEXED_PITCH_EG_L2,        // 5
    //  DEXED_PITCH_EG_L3,        // 6
    //  DEXED_PITCH_EG_L4,        // 7
    file.read(p_data + DEXED_VOICE_OFFSET + DEXED_PITCH_EG_R1, 8);
    tmp = file.read();
    *(p_data + DEXED_VOICE_OFFSET + DEXED_ALGORITHM) = (tmp & 0x1f);
    tmp = file.read();
    *(p_data + DEXED_VOICE_OFFSET + DEXED_OSC_KEY_SYNC) = (tmp & 0x08) >> 3;
    *(p_data + DEXED_VOICE_OFFSET + DEXED_FEEDBACK) = (tmp & 0x07);
    //  DEXED_LFO_SPEED,          // 11
    //  DEXED_LFO_DELAY,          // 12
    //  DEXED_LFO_PITCH_MOD_DEP,  // 13
    //  DEXED_LFO_AMP_MOD_DEP,    // 14
    file.read(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_SPEED, 4);
    tmp = file.read();
    *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_PITCH_MOD_SENS) = (tmp & 0x30) >> 4;
    *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_WAVE) = (tmp & 0x0e) >> 1;
    *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_SYNC) = (tmp & 0x01);
    file.read(p_data + DEXED_VOICE_OFFSET + DEXED_TRANSPOSE, 1);
    file.read(p_data + DEXED_VOICE_OFFSET + DEXED_NAME, 10);
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_RANGE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_STEP) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_RANGE) = 99;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_ASSIGN) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_RANGE) = 99;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_ASSIGN) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_RANGE) = 99;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_ASSIGN) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_RANGE) = 99;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_ASSIGN) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MASTER_TUNE) = 0;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP1_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP2_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP3_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP4_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP5_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP6_ENABLE) = 1;
    *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MAX_NOTES) = MAX_NOTES;

    dexed->setOPs((*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP1_ENABLE) << 5) |
                  (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP2_ENABLE) << 4) |
                  (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP3_ENABLE) << 3) |
                  (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP4_ENABLE) << 2) |
                  (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP5_ENABLE) << 1) |
                  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP6_ENABLE ));
    dexed->setMaxNotes(*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MAX_NOTES));
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
