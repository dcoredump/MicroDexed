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
extern LiquidCrystalPlus_I2C lcd;
extern config_t configuration;
extern uint8_t max_loaded_banks;
extern char bank_name[BANK_NAME_LEN];
extern char voice_name[VOICE_NAME_LEN];
extern uint8_t ui_state;
extern uint8_t ui_main_state;
extern void eeprom_write(void);
extern void set_volume(float v, float pan);
extern elapsedMillis autostore;
extern elapsedMillis long_button_pressed;
extern uint8_t effect_filter_frq;
extern uint8_t effect_filter_resonance;
extern uint8_t effect_filter_octave;
extern uint8_t effect_delay_time;
extern uint8_t effect_delay_feedback;
extern uint8_t effect_delay_volume;
extern bool effect_delay_sync;
extern AudioEffectDelay delay1;
extern AudioMixer4 mixer1;
extern AudioMixer4 mixer2;

void handle_ui(void);
void ui_show_main(void);
void ui_show_volume(void);
void ui_show_midichannel(void);
void ui_show_effects_filter(void);
void ui_show_effects_delay(void);
float mapfloat(float val, float in_min, float in_max, float out_min, float out_max);

enum ui_states {UI_MAIN, UI_VOLUME, UI_MIDICHANNEL, UI_EFFECTS_FILTER, UI_EFFECTS_DELAY};
enum ui_main_states {UI_MAIN_BANK, UI_MAIN_VOICE, UI_MAIN_BANK_SELECTED, UI_MAIN_VOICE_SELECTED, UI_MAIN_FILTER_FRQ, UI_MAIN_FILTER_RES, UI_MAIN_FILTER_OCT, UI_MAIN_DELAY_TIME, UI_MAIN_DELAY_FEEDBACK, UI_MAIN_DELAY_VOLUME};

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
