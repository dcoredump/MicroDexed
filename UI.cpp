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

#include <Arduino.h>
#include <limits.h>
#include "config.h"
#include "dexed.h"
#include "dexed_sysex.h"
#include "UI.h"

#ifdef I2C_DISPLAY // selecting sounds by encoder, button and display

elapsedMillis ui_back_to_main;

void handle_ui(void)
{
  if (ui_back_to_main >= UI_AUTO_BACK_MS && (ui_state != UI_MAIN && ui_state != UI_EFFECTS_FILTER && ui_state != UI_EFFECTS_DELAY))
  {
    enc[0].write(map(vol * 100, 0, 100, 0, ENC_VOL_STEPS));
    enc_val[0] = enc[0].read();
    ui_show_main();
  }

  if (autostore >= AUTOSTORE_MS && (ui_main_state == UI_MAIN_VOICE_SELECTED || ui_main_state == UI_MAIN_BANK_SELECTED))
  {
    ui_show_main();
    switch (ui_main_state)
    {
      case UI_MAIN_VOICE_SELECTED:
        ui_main_state = UI_MAIN_VOICE;
        break;
      case UI_MAIN_BANK_SELECTED:
        ui_main_state = UI_MAIN_BANK;
        break;
    }
  }

  for (uint8_t i = 0; i < NUM_ENCODER; i++)
  {
    but[i].update();

    if (but[i].fallingEdge())
      long_button_pressed = 0;

    if (but[i].risingEdge())
    {
      uint32_t button_released = long_button_pressed;

      if (button_released > LONG_BUTTON_PRESS)
      {
        // long pressing of button detected
#ifdef DEBUG
        Serial.print(F("Long button pressing detected for button "));
        Serial.println(i, DEC);

        switch (i)
        {
          case 0: // long press for left button
            break;
          case 1: // long press for right button
            switch (ui_state)
            {
              case UI_MAIN:
                ui_main_state = UI_MAIN_FILTER_FRQ;
                enc[i].write(effect_filter_frq);
                enc_val[i] = enc[i].read();
                ui_show_effects_filter();
                break;
              case UI_EFFECTS_FILTER:
                ui_main_state = UI_MAIN_DELAY_TIME;
                enc[i].write(effect_delay_time);
                enc_val[i] = enc[i].read();
                ui_show_effects_delay();
                break;
              case UI_EFFECTS_DELAY:
                ui_main_state = UI_MAIN_VOICE;
                enc[i].write(voice);
                enc_val[i] = enc[i].read();
                ui_show_main();
                break;
            }
            break;
        }
#endif
      }
      else
      {
        // Button pressed
        switch (i)
        {
          case 0: // left button pressed
            switch (ui_state)
            {
              case UI_MAIN:
                enc[i].write(map(vol * 100, 0, 100, 0, ENC_VOL_STEPS));
                enc_val[i] = enc[i].read();
                ui_show_volume();
                break;
              case UI_VOLUME:
                enc[i].write(midi_channel);
                enc_val[i] = enc[i].read();
                ui_show_midichannel();
                break;
              case UI_MIDICHANNEL:
                enc[i].write(map(vol * 100, 0, 100, 0, ENC_VOL_STEPS));
                enc_val[i] = enc[i].read();
                ui_show_main();
                break;
            }
            break;
          case 1: // right button pressed
            switch (ui_state)
            {
              case UI_MAIN:
                switch (ui_main_state)
                {
                  case UI_MAIN_BANK:
                  case UI_MAIN_BANK_SELECTED:
                    ui_main_state = UI_MAIN_VOICE;
                    enc[i].write(voice);
                    enc_val[i] = enc[i].read();
                    break;
                  case UI_MAIN_VOICE:
                  case UI_MAIN_VOICE_SELECTED:
                    ui_main_state = UI_MAIN_BANK;
                    enc[i].write(bank);
                    enc_val[i] = enc[i].read();
                    break;
                }
                ui_show_main();
                break;
              case UI_EFFECTS_FILTER:
              case UI_EFFECTS_DELAY:
                switch (ui_main_state)
                {
                  case UI_MAIN_FILTER_FRQ:
                    ui_main_state = UI_MAIN_FILTER_RES;
                    enc[i].write(effect_filter_resonance);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_filter();
                    break;
                  case UI_MAIN_FILTER_RES:
                    ui_main_state = UI_MAIN_FILTER_OCT;
                    enc[i].write(effect_filter_octave);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_filter();
                    break;
                  case UI_MAIN_FILTER_OCT:
                    ui_main_state = UI_MAIN_FILTER_FRQ;
                    enc[i].write(effect_filter_frq);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_filter();
                    break;
                  case UI_MAIN_DELAY_TIME:
                    ui_main_state = UI_MAIN_DELAY_FEEDBACK;
                    enc[i].write(effect_delay_feedback);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_delay();
                    break;
                  case UI_MAIN_DELAY_SYNC:
                    ui_main_state = UI_MAIN_DELAY_TIME;
                    enc[i].write(effect_delay_time);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_delay();
                    break;
                  case UI_MAIN_DELAY_FEEDBACK:
                    ui_main_state = UI_MAIN_DELAY_SYNC;
                    enc[i].write(effect_delay_sync);
                    enc_val[i] = enc[i].read();
                    ui_show_effects_delay();
                    break;
                }
                break;
            }
        }
      }
#ifdef DEBUG
      Serial.print(F("Button "));
      Serial.println(i, DEC);
#endif
    }

    if (enc[i].read() == enc_val[i])
      continue;
    else
    {
      switch (i)
      {
        case 0: // left encoder moved
          switch (ui_state)
          {
            case UI_MAIN:
            case UI_VOLUME:
              if (enc[i].read() <= 0)
                enc[i].write(0);
              else if (enc[i].read() >= ENC_VOL_STEPS)
                enc[i].write(ENC_VOL_STEPS);
              set_volume(float(map(enc[i].read(), 0, ENC_VOL_STEPS, 0, 100)) / 100, vol_left, vol_right);
              eeprom_write(EEPROM_UPDATE_VOL);
              ui_show_volume();
              break;
            case UI_MIDICHANNEL:
              if (enc[i].read() <= 0)
                enc[i].write(0);
              else if (enc[i].read() >= 16)
                enc[i].write(16);
              midi_channel = enc[i].read();
              eeprom_write(EEPROM_UPDATE_MIDICHANNEL);
              ui_show_midichannel();
              break;
          }
          break;
        case 1: // right encoder moved
          switch (ui_state)
          {
            case UI_MAIN:
              switch (ui_main_state)
              {
                case UI_MAIN_BANK:
                  ui_main_state = UI_MAIN_BANK_SELECTED;
                case UI_MAIN_BANK_SELECTED:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > max_loaded_banks - 1)
                    enc[i].write(max_loaded_banks - 1);
                  bank = enc[i].read();
                  get_voice_names_from_bank(bank);
                  load_sysex(bank, voice);
                  eeprom_write(EEPROM_UPDATE_BANK);
                  break;
                case UI_MAIN_VOICE:
                  ui_main_state = UI_MAIN_VOICE_SELECTED;
                case UI_MAIN_VOICE_SELECTED:
                  if (enc[i].read() <= 0)
                  {
                    if (bank > 0)
                    {
                      enc[i].write(MAX_VOICES - 1);
                      bank--;
                      get_voice_names_from_bank(bank);
                    }
                    else
                      enc[i].write(0);
                  }
                  else if (enc[i].read() > MAX_VOICES - 1)
                  {
                    if (bank < MAX_BANKS - 1)
                    {
                      enc[i].write(0);
                      bank++;
                      get_voice_names_from_bank(bank);
                    }
                    else
                      enc[i].write(MAX_VOICES - 1);
                  }
                  voice = enc[i].read();
                  load_sysex(bank, voice);
                  eeprom_write(EEPROM_UPDATE_VOICE);
                  break;
              }
              ui_show_main();
              break;
            case UI_EFFECTS_FILTER:
              switch (ui_main_state)
              {
                case UI_MAIN_FILTER_FRQ:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > ENC_FILTER_FRQ_STEPS)
                    enc[i].write(ENC_FILTER_FRQ_STEPS);
                  effect_filter_frq = expf((float)map(enc[i].read(), 0, ENC_FILTER_FRQ_STEPS, 0, 1024) / 150.0) * 10.0 + 80.0;
                  filter1.frequency(effect_filter_frq);
                  break;
                case UI_MAIN_FILTER_RES:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > ENC_FILTER_RES_STEPS)
                    enc[i].write(ENC_FILTER_RES_STEPS);
                  effect_filter_resonance = enc[i].read();
                  filter1.resonance(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0));
                  break;
                case UI_MAIN_FILTER_OCT:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > ENC_FILTER_OCT_STEPS)
                    enc[i].write(ENC_FILTER_OCT_STEPS);
                  effect_filter_octave = enc[i].read();
                  filter1.octaveControl(mapfloat(enc[i].read(), 0, ENC_FILTER_OCT_STEPS, 0.0, 7.0));
                  break;
              }
              ui_show_effects_filter();
              break;
            case UI_EFFECTS_DELAY:
              switch (ui_main_state)
              {
                case UI_MAIN_DELAY_TIME:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > ENC_DELAY_TIME_STEPS)
                    enc[i].write(ENC_DELAY_TIME_STEPS);
                  effect_delay_time = enc[i].read();;
                  delay1.delay(0,map(effect_delay_feedback, 0, ENC_DELAY_TIME_STEPS, 0, DELAY_MAX_TIME));
                  break;
                case UI_MAIN_DELAY_FEEDBACK:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() > ENC_DELAY_FB_STEPS)
                    enc[i].write(ENC_DELAY_FB_STEPS);
                  effect_delay_feedback = enc[i].read();
                  mixer1.gain(1, mapfloat(effect_delay_feedback,0,99,0.0,1.0));
                  break;
                case UI_MAIN_DELAY_SYNC:
                  if (enc[i].read() <= 0)
                    enc[i].write(0);
                  else if (enc[i].read() >= 1)
                    enc[i].write(1);
                  effect_delay_sync = enc[i].read();
                  // Nothing to do here
                  break;
              }
              ui_show_effects_delay();
              break;
          }
          break;
      }
#ifdef DEBUG
      Serial.print(F("Encoder "));
      Serial.print(i, DEC);
      Serial.print(F(": "));
      Serial.println(enc[i].read(), DEC);
#endif
    }
    enc_val[i] = enc[i].read();
  }
}

void ui_show_main(void)
{
  if (ui_state != UI_MAIN)
  {
    lcd.clear();
  }

  lcd.show(0, 0, 2, bank);
  lcd.show(0, 2, 1, " ");
  strip_extension(bank_names[bank], bank_name);

  if (ui_main_state == UI_MAIN_BANK || ui_main_state == UI_MAIN_BANK_SELECTED)
  {
    lcd.show(0, 2, 1, "[");
    lcd.show(0, 3, 8, bank_name);
    lcd.show(0, 11, 1, "]");
  }
  else
  {
    lcd.show(0, 2, 1, " ");
    lcd.show(0, 3, 8, bank_name);
    lcd.show(0, 11, 1, " ");
  }

  lcd.show(1, 0, 2, voice + 1);
  lcd.show(1, 2, 1, " ");
  if (ui_main_state == UI_MAIN_VOICE || ui_main_state == UI_MAIN_VOICE_SELECTED)
  {
    lcd.show(1, 2, 1, "[");
    lcd.show(1, 3, 10, voice_names[voice]);
    lcd.show(1, 14, 1, "]");
  }
  else
  {
    lcd.show(1, 2, 1, " ");
    lcd.show(1, 3, 10, voice_names[voice]);
    lcd.show(1, 14, 1, " ");
  }

  ui_state = UI_MAIN;
}

void ui_show_volume(void)
{
  ui_back_to_main = 0;

  if (ui_state != UI_VOLUME)
  {
    lcd.clear();
    lcd.show(0, 0, LCD_CHARS, "Volume");
  }

  lcd.show(0, LCD_CHARS - 3, 3, vol * 100);
  if (vol == 0.0)
    lcd.show(1, 0, LCD_CHARS , " ");
  else
  {
    if (vol < (float(LCD_CHARS) / 100))
      lcd.show(1, 0, LCD_CHARS, "*");
    else
    {
      for (uint8_t i = 0; i < map(vol * 100, 0, 100, 0, LCD_CHARS); i++)
        lcd.show(1, i, 1, "*");
      for (uint8_t i = map(vol * 100, 0, 100, 0, LCD_CHARS); i < LCD_CHARS; i++)
        lcd.show(1, i, 1, " ");
    }
  }

  ui_state = UI_VOLUME;
}

void ui_show_midichannel(void)
{
  ui_back_to_main = 0;

  if (ui_state != UI_MIDICHANNEL)
  {
    lcd.clear();
    lcd.show(0, 0, LCD_CHARS, "MIDI Channel");
  }

  if (midi_channel == MIDI_CHANNEL_OMNI)
    lcd.show(1, 0, 4, "OMNI");
  else
  {
    lcd.show(1, 0, 2, midi_channel);
    if (midi_channel == 1)
      lcd.show(1, 2, 2, "  ");
  }

  ui_state = UI_MIDICHANNEL;
}

void ui_show_effects_filter(void)
{
  if (ui_state != UI_EFFECTS_FILTER)
  {
    lcd.clear();
    lcd.show(0, 0, LCD_CHARS, "Filter");
    lcd.show(0, 7, 2, "F:");
    lcd.show(1, 0, 4, "Res:");
    lcd.show(1, 8, 4, "Oct:");
  }

  lcd.show(0, 10, 3, effect_filter_frq);
  lcd.show(1, 5, 2, map(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0, 99));
  lcd.show(1, 13, 2, map(effect_filter_octave, 0, ENC_FILTER_OCT_STEPS, 0, 80));

  if (ui_main_state == UI_MAIN_FILTER_FRQ)
  {
    lcd.show(0, 9, 1, "[");
    lcd.show(0, 13, 1, "]");
  }
  else
  {
    lcd.show(0, 9, 1, " ");
    lcd.show(0, 13, 1, " ");
  }

  if (ui_main_state == UI_MAIN_FILTER_RES)
  {
    lcd.show(1, 4, 1, "[");
    lcd.show(1, 7, 1, "]");
  }
  else
  {
    lcd.show(1, 4, 1, " ");
    lcd.show(1, 7, 1, " ");
  }

  if (ui_main_state == UI_MAIN_FILTER_OCT)
  {
    lcd.show(1, 12, 1, "[");
    lcd.show(1, 15, 1, "]");
  }
  else
  {
    lcd.show(1, 12, 1, " ");
    lcd.show(1, 15, 1, " ");
  }

  ui_state = UI_EFFECTS_FILTER;
}

void ui_show_effects_delay(void)
{
  if (ui_state != UI_EFFECTS_DELAY)
  {
    lcd.clear();
    lcd.show(0, 0, 3, "Dly");
    lcd.show(0, 6, 2, "T:");
    lcd.show(0, 14, 2, "ms");
    lcd.show(1, 0, 3, "FB:");
    lcd.show(1, 8, 5, "Sync:");
  }

  lcd.show(0, 9, 4, map(effect_delay_time, 0, ENC_DELAY_TIME_STEPS, 0, 1200));
  lcd.show(1, 4, 2, map(effect_delay_feedback, 0, ENC_DELAY_TIME_STEPS, 0, 99));
  lcd.show(1, 14, 1, effect_delay_sync);

  if (ui_main_state == UI_MAIN_DELAY_TIME)
  {
    lcd.show(0, 8, 1, "[");
    lcd.show(0, 13, 1, "]");
  }
  else
  {
    lcd.show(0, 8, 1, " ");
    lcd.show(0, 13, 1, " ");
  }

  if (ui_main_state == UI_MAIN_DELAY_FEEDBACK)
  {
    lcd.show(1, 3, 1, "[");
    lcd.show(1, 6, 1, "]");
  }
  else
  {
    lcd.show(1, 3, 1, " ");
    lcd.show(1, 6, 1, " ");
  }

  if (ui_main_state == UI_MAIN_DELAY_SYNC)
  {
    lcd.show(1, 13, 1, "[");
    lcd.show(1, 15, 1, "]");
  }
  else
  {
    lcd.show(1, 13, 1, " ");
    lcd.show(1, 15, 1, " ");
  }

  ui_state = UI_EFFECTS_DELAY;
}

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

#endif
