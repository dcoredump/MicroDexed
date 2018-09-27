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
#include <Encoder.h>
#include <LiquidCrystalPlus_I2C.h>

#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

extern Encoder enc[2];
extern int32_t enc_val[2];
extern Bounce but[2];

void handle_ui(void);
int32_t getEncPosition(uint8_t encoder_number);
void setEncPosition(uint8_t encoder_number, int32_t value);

#endif
