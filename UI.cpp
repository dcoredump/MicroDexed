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
  if (ui_back_to_main >= UI_AUTO_BACK_MS && ui_state != UI_MAIN)
  {
    enc[0].write(map(vol * 100, 0, 100, 0, ENC_VOL_STEPS));
    enc_val[0] = enc[0].read();
    /* switch (ui_main_state)
    {
      case UI_VOLUME:
        eeprom_write(EEPROM_UPDATE_VOL);
        break;
      case UI_MIDICHANNEL:
        eeprom_write(EEPROM_UPDATE_MIDICHANNEL);
        break;
    }*/
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

    if (but[i].risingEdge())
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
    lcd.show(0, 3, 10, bank_name);
    lcd.show(0, 14, 1, "]");
  }
  else
  {
    lcd.show(0, 2, 1, " ");
    lcd.show(0, 3, 10, bank_name);
    lcd.show(0, 14, 1, " ");
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

#endif
