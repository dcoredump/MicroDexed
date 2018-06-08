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

//#define TEST_MIDI 1
#define TEST_NOTE 40
#define TEST_VEL_MIN 60
#define TEST_VEL_MAX 110

#define DEBUG 1
#define SERIAL_SPEED 38400
#define VOLUME 0.1
#define SAMPLE_RATE 44100
#define DEXED_ENGINE DEXED_ENGINE_MODERN
//#define USE_ONBOARD_USB_HOST 1
//#define SHOW_DEXED_TIMING 1
#define SHOW_MIDI_EVENT 1
#define SHOW_XRUN 1
#define SHOW_CPU_LOAD_MSEC 5000
#define MAX_NOTES 16
#define AUDIO_MEM 2

#define DEFAULT_SYSEXBANK 0
#define DEFAULT_SYSEXSOUND 0

#define MASTER_KEY_MIDI 84      // C6
#define MASTER_NUM1 24          // C1
#define MASTER_BANK_SELECT 83   // B5

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14
// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used
