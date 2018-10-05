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

#include "config.h"
#include <Bounce.h>
#include <EEPROM.h>
#include <MIDI.h>
#include "LiquidCrystalPlus_I2C.h"
#include "Encoder4.h"

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

extern Encoder4 enc[2];
extern int32_t enc_val[2];
extern Bounce but[2];
extern float vol;
extern float vol_left;
extern float vol_right;
extern LiquidCrystalPlus_I2C lcd;
extern uint8_t bank;
extern uint8_t max_loaded_banks;
extern uint8_t voice;
extern char bank_name[BANK_NAME_LEN];
extern char voice_name[VOICE_NAME_LEN];
extern uint8_t ui_state;
extern uint8_t ui_main_state;
extern uint8_t midi_channel;
extern void eeprom_write(uint8_t status);
extern void set_volume(float v, float vr, float vl);
extern elapsedMillis autostore;

void handle_ui(void);
void ui_show_main(void);
void ui_show_volume(void);
void ui_show_midichannel(void);

enum ui_states {UI_MAIN, UI_VOLUME, UI_MIDICHANNEL};
enum ui_main_states {UI_MAIN_BANK, UI_MAIN_VOICE, UI_MAIN_BANK_SELECTED, UI_MAIN_VOICE_SELECTED};

class MyEncoder : public Encoder
{
    int32_t read()
    {
      return (Encoder::read() / 4);
    }
    void write(int32_t p)
    {
      Encoder::write(p * 4);
    }
};
#endif
