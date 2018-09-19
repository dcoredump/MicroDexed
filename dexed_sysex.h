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
#ifdef I2C_DISPLAY
#include <LiquidCrystalPlus_I2C.h>
#endif

extern bool sd_card_available;
extern Dexed* dexed;
extern uint16_t render_time_max;
extern uint8_t bank;
extern uint8_t voice;
extern char bank_name[10];
extern char voice_name[10];
extern LiquidCrystalPlus_I2C lcd;

bool load_sysex(uint8_t b, uint8_t v);
bool get_sysex_voice(char* dir, File sysex, uint8_t voice_number, uint8_t* data);
