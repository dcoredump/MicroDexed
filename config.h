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

#include "midinotes.h"

// ATTENTION! For better latency you have to redefine AUDIO_BLOCK_SAMPLES from
// 128 to 64 in <ARDUINO-IDE-DIR>/cores/teensy3/AudioStream.h


#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

// Initial values
#define VERSION 1.0
#define MIDI_DEVICE Serial1
#define USE_ONBOARD_USB_HOST 1
#define TEENSY_AUDIO_BOARD 1
#define VOLUME 0.6
#define DEFAULT_MIDI_CHANNEL MIDI_CHANNEL_OMNI
#define DEFAULT_SYSEXBANK 0
#define DEFAULT_SYSEXSOUND 0
//#define DEXED_ENGINE DEXED_ENGINE_MODERN
#ifndef TEENSY_AUDIO_BOARD
#define AUDIO_MEM 4
#else
#define AUDIO_MEM 2
#endif
#define SAMPLE_RATE 44100
#define MAX_BANKS 99
#define MAX_VOICES 32 // voices per bank
#define BANK_NAME_LEN 13 // FAT12 filenames (plus '\0')
#define VOICE_NAME_LEN 11 // 10 (plus '\0')

#if !defined(__MK66FX1M0__) // check for Teensy-3.6
#define MAX_NOTES 11        // No? 
#undef USE_ONBOARD_USB_HOST
#else
#define MAX_NOTES 16        // Yes
#endif

// Master key handling (comment for disabling)
#define MASTER_KEY_MIDI MIDI_C7
#define MASTER_NUM1 MIDI_C2

// Debug output
#define SERIAL_SPEED 38400
//#define DEBUG 1
#define SHOW_MIDI_EVENT 1
#define SHOW_XRUN 1
#define SHOW_CPU_LOAD_MSEC 5000

// Some optimizations
#define USE_TEENSY_DSP 1
#define SUM_UP_AS_INT 1
#define REDUCE_LOUDNESS 1

// Enable TEST_NOTE for adding code to drop some midi notes for testing without keyboard
//#define TEST_NOTE MIDI_E2
#define TEST_VEL_MIN 60
#define TEST_VEL_MAX 110

// Use these with the Teensy Audio Shield
//#define SDCARD_CS_PIN    10
//#define SDCARD_MOSI_PIN  7
//#define SDCARD_SCK_PIN   14
// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

// LCD Display
#define I2C_DISPLAY 1
// [I2C] SCL: Pin 19, SDA: Pin 18 (https://www.pjrc.com/teensy/td_libs_Wire.html)
#define LCD_I2C_ADDRESS 0x27
#define LCD_CHARS 16
#define LCD_LINES 2
#define UI_AUTO_BACK_MS 2000

// Encoder with button
#define ENC_VOL_STEPS 43
#define TIMER_UI_HANDLING_MS 100
#define NUM_ENCODER 2
#define ENC_L_PIN_A  3
#define ENC_L_PIN_B  2
#define BUT_L_PIN    4
#define INITIAL_ENC_L_VALUE 0
#define ENC_R_PIN_A  28
#define ENC_R_PIN_B  29
#define BUT_R_PIN    30
#define INITIAL_ENC_R_VALUE 0
#define BUT_DEBOUNCE_MS 20

// EEPROM address
#define EEPROM_OFFSET 0
#define EEPROM_DATA_LENGTH 5

#define EEPROM_CRC32_ADDR EEPROM.length()-sizeof(uint32_t)
#define EEPROM_BANK_ADDR 0
#define EEPROM_VOICE_ADDR 1
#define EEPROM_MASTER_VOLUME_ADDR 2
#define EEPROM_VOLUME_RIGHT_ADDR 3
#define EEPROM_VOLUME_LEFT_ADDR 4

#endif
