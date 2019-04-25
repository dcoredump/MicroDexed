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

 // Idea from: https://playground.arduino.cc/Code/EEPROMWriteAnything/

#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

uint32_t crc32(uint8_t* calc_start, uint16_t calc_bytes);

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    uint8_t* p = (uint8_t*)(const void*)&value;
    uint16_t i;
    uint32_t checksum=crc32(p+4,sizeof(value)-4);

    *p=checksum;

    for (i = 0; i < sizeof(value); i++)
          EEPROM.update(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    uint8_t* p = (uint8_t*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}
