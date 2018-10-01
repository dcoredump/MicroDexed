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
    lcd.clear();
    ui_show_main();
    EEPROM.update(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR, uint8_t(vol * UCHAR_MAX));
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR, uint8_t(vol_right * UCHAR_MAX));
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR, uint8_t(vol_left * UCHAR_MAX));
    update_eeprom_checksum();
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
          switch (ui_main_state)
          {
            case UI_MAIN_BANK:
            case UI_MAIN_BANK_SELECTED:
              ui_main_state = UI_MAIN_VOICE;
              enc[1].write(voice);
              enc_val[1] = enc[1].read();
              break;
            case UI_MAIN_VOICE:
            case UI_MAIN_VOICE_SELECTED:
              ui_main_state = UI_MAIN_BANK;
              enc[1].write(bank);
              enc_val[1] = enc[1].read();
              break;
          }
          break;
        case 1: // right button pressed
          if (ui_main_state == UI_MAIN_VOICE_SELECTED)
          {
            ui_main_state = UI_MAIN_VOICE;
            load_sysex(bank, voice);
            EEPROM.update(EEPROM_OFFSET + EEPROM_BANK_ADDR, bank);
            EEPROM.update(EEPROM_OFFSET + EEPROM_VOICE_ADDR, voice);
            update_eeprom_checksum();
          }
          break;
      }
      ui_show_main();
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
          if (enc[i].read() <= 0)
            enc[i].write(0);
          else if (enc[i].read() >= ENC_VOL_STEPS)
            enc[i].write(ENC_VOL_STEPS);
          set_volume(float(map(enc[i].read(), 0, ENC_VOL_STEPS, 0, 100)) / 100, vol_left, vol_right);
          ui_show_volume();
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
              break;
            case UI_MAIN_VOICE:
              ui_main_state = UI_MAIN_VOICE_SELECTED;
            case UI_MAIN_VOICE_SELECTED:
              if (enc[i].read() <= 0)
                enc[i].write(0);
              else if (enc[i].read() > MAX_VOICES - 1)
                enc[i].write(MAX_VOICES - 1);
              voice = enc[i].read();
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
  ui_state = UI_MAIN;

  lcd.show(0, 0, 2, bank + 1);
  lcd.show(0, 2, 1, " ");
  strip_extension(bank_names[bank], bank_name);

  if (ui_main_state == UI_MAIN_BANK)
  {
    lcd.show(0, 2, 1, "[");
    lcd.show(0, 3, 10, bank_name);
    lcd.show(0, 14, 1, "]");
  }
  else if (ui_main_state == UI_MAIN_BANK_SELECTED)
  {
    lcd.show(0, 2, 1, "<");
    lcd.show(0, 3, 10, bank_name);
    lcd.show(0, 14, 1, ">");
  }
  else
  {
    lcd.show(0, 2, 1, " ");
    lcd.show(0, 3, 10, bank_name);
    lcd.show(0, 14, 1, " ");
  }

  lcd.show(1, 0, 2, voice + 1);
  lcd.show(1, 2, 1, " ");
  if (ui_main_state == UI_MAIN_VOICE)
  {
    lcd.show(1, 2, 1, "[");
    lcd.show(1, 3, 10, voice_names[voice]);
    lcd.show(1, 14, 1, "]");
  }
  else if (ui_main_state == UI_MAIN_VOICE_SELECTED)
  {
    lcd.show(1, 2, 1, "<");
    lcd.show(1, 3, 10, voice_names[voice]);
    lcd.show(1, 14, 1, ">");
  }
  else
  {
    lcd.show(1, 2, 1, " ");
    lcd.show(1, 3, 10, voice_names[voice]);
    lcd.show(1, 14, 1, " ");
  }
}

void ui_show_volume(void)
{
  ui_back_to_main = 0;

  if (ui_state != UI_VOLUME)
    lcd.show(0, 0, LCD_CHARS, "Volume");

  lcd.show(0, LCD_CHARS - 3, 3, vol * 100);
  for (uint8_t i = 0; i < map(vol * 100, 0, 100, 0, LCD_CHARS); i++)
    lcd.show(1, i, 1, "*");
  for (uint8_t i = map(vol * 100, 0, 100, 0, LCD_CHARS); i < LCD_CHARS; i++)
    lcd.show(1, i, 1, " ");

  ui_state = UI_VOLUME;
}

#endif
