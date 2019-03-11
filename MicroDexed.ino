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
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
#include <EEPROM.h>
#include "midi_devices.hpp"
#include <limits.h>
#include "dexed.h"
#include "dexed_sysex.h"

#ifdef I2C_DISPLAY // selecting sounds by encoder, button and display
#include "UI.h"
#include <Bounce.h>
#include "Encoder4.h"
#include "LiquidCrystalPlus_I2C.h"
LiquidCrystalPlus_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);
Encoder4 enc[2] = {Encoder4(ENC_L_PIN_A, ENC_L_PIN_B), Encoder4(ENC_R_PIN_A, ENC_R_PIN_B)};
int32_t enc_val[2] = {INITIAL_ENC_L_VALUE, INITIAL_ENC_R_VALUE};
Bounce but[2] = {Bounce(BUT_L_PIN, BUT_DEBOUNCE_MS), Bounce(BUT_R_PIN, BUT_DEBOUNCE_MS)};
elapsedMillis master_timer;
uint8_t ui_state = UI_MAIN;
uint8_t ui_main_state = UI_MAIN_VOICE;
#endif

AudioPlayQueue           queue1;
AudioAnalyzePeak         peak1;
AudioFilterStateVariable filter1;
AudioEffectDelay         delay1;
AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioFilterBiquad        antialias;
AudioConnection          patchCord0(queue1, peak1);
AudioConnection          patchCord1(queue1, antialias);
AudioConnection          patchCord2(antialias, 0, filter1, 0);
AudioConnection          patchCord3(filter1, 0, delay1, 0);
AudioConnection          patchCord4(filter1, 0, mixer1, 0);
AudioConnection          patchCord5(filter1, 0, mixer2, 0);
AudioConnection          patchCord6(delay1, 0, mixer1, 1);
AudioConnection          patchCord7(delay1, 0, mixer2, 2);
AudioConnection          patchCord8(mixer1, delay1);
AudioConnection          patchCord9(queue1, 0, mixer1, 3); // for disabling the filter
AudioConnection          patchCord10(mixer1, 0, mixer2, 1);
#if defined(TEENSY_AUDIO_BOARD)
AudioOutputI2S           i2s1;
AudioConnection          patchCord111(mixer2, 0, i2s1, 0);
AudioConnection          patchCord112(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;
#elif defined(TGA_AUDIO_BOARD)
AudioOutputI2S           i2s1;
AudioAmplifier           volume_r;
AudioAmplifier           volume_l;
AudioConnection          patchCord11(mixer2, volume_r);
AudioConnection          patchCord12(mixer2, volume_l);
AudioConnection          patchCord13(volume_r, 0, i2s1, 1);
AudioConnection          patchCord14(volume_l, 0, i2s1, 0);
AudioControlWM8731master wm8731_1;
#else
AudioOutputPT8211        pt8211_1;
AudioAmplifier           volume_master;
AudioAmplifier           volume_r;
AudioAmplifier           volume_l;
AudioConnection          patchCord11(mixer2, 0, volume_master, 0);
AudioConnection          patchCord12(volume_master, volume_r);
AudioConnection          patchCord13(volume_master, volume_l);
AudioConnection          patchCord14(volume_r, 0, pt8211_1, 0);
AudioConnection          patchCord15(volume_l, 0, pt8211_1, 1);
#endif

Dexed* dexed = new Dexed(SAMPLE_RATE);
bool sd_card_available = false;
uint8_t midi_channel = DEFAULT_MIDI_CHANNEL;
uint32_t xrun = 0;
uint32_t overload = 0;
uint32_t peak = 0;
uint16_t render_time_max = 0;
uint8_t bank = 0;
uint8_t max_loaded_banks = 0;
uint8_t voice = 0;
float vol = VOLUME;
float pan = 0.5f;
char bank_name[BANK_NAME_LEN];
char voice_name[VOICE_NAME_LEN];
char bank_names[MAX_BANKS][BANK_NAME_LEN];
char voice_names[MAX_VOICES][VOICE_NAME_LEN];
elapsedMillis autostore;
uint8_t eeprom_update_status = 0;
uint16_t autostore_value = AUTOSTORE_MS;
uint8_t midi_timing_counter = 0; // 24 per qarter
elapsedMillis midi_timing_timestep;
uint16_t midi_timing_quarter = 0;
elapsedMillis long_button_pressed;
uint8_t effect_filter_frq = ENC_FILTER_FRQ_STEPS;
uint8_t effect_filter_resonance = 0;
uint8_t effect_filter_octave = (1.0 * ENC_FILTER_RES_STEPS / 8.0) + 0.5;
uint8_t effect_delay_time = 0;
uint8_t effect_delay_feedback = 0;
uint8_t effect_delay_volume = 0;
bool effect_delay_sync = 0;
elapsedMicros fill_audio_buffer;
elapsedMillis control_rate;

#ifdef SHOW_CPU_LOAD_MSEC
elapsedMillis cpu_mem_millis;
#endif

#ifdef TEST_NOTE
IntervalTimer sched_note_on;
IntervalTimer sched_note_off;
uint8_t _voice_counter = 0;
#endif

void setup()
{
  //while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.begin(SERIAL_SPEED);

#ifdef I2C_DISPLAY
  lcd.init();
  lcd.blink_off();
  lcd.cursor_off();
  lcd.backlight();
  lcd.noAutoscroll();
  lcd.clear();
  lcd.display();
  lcd.show(0, 0, 16, "MicroDexed");
  lcd.show(0, 11, 16, VERSION);
  lcd.show(1, 0, 16, "(c)parasiTstudio");

  pinMode(BUT_L_PIN, INPUT_PULLUP);
  pinMode(BUT_R_PIN, INPUT_PULLUP);
#endif

  delay(220);
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018,2019 H. Wirtz <wirtz@parasitstudio.de>"));
  Serial.println(F("https://github.com/dcoredump/MicroDexed"));
  Serial.println(F("<setup start>"));

  //init_eeprom();
  initial_values_from_eeprom();

  setup_midi_devices();

  // start audio card
  AudioNoInterrupts();
  AudioMemory(AUDIO_MEM);

#ifdef TEENSY_AUDIO_BOARD
  sgtl5000_1.enable();
  //sgtl5000_1.dacVolumeRamp();
  sgtl5000_1.dacVolumeRampLinear();
  //sgtl5000_1.dacVolumeRampDisable();
  sgtl5000_1.unmuteHeadphone();
  sgtl5000_1.unmuteLineout();
  sgtl5000_1.autoVolumeDisable(); // turn off AGC
  sgtl5000_1.volume(1.0, 1.0);
  sgtl5000_1.lineOutLevel(31);
  sgtl5000_1.audioPostProcessorEnable();
  sgtl5000_1.autoVolumeControl(1, 1, 1, 0.9, 0.01, 0.05);
  sgtl5000_1.autoVolumeEnable();
  Serial.println(F("Teensy-Audio-Board enabled."));
#elif defined(TGA_AUDIO_BOARD)
  wm8731_1.enable();
  wm8731_1.volume(1.0);
  Serial.println(F("TGA board enabled."));
#else
  Serial.println(F("PT8211 enabled."));
#endif

  set_volume(vol, pan);

  // start SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!SD.begin(SDCARD_CS_PIN))
  {
    Serial.println(F("SD card not accessable."));
    strcpy(bank_name, "Default");
    strcpy(voice_name, "Default");
  }
  else
  {
    Serial.println(F("SD card found."));
    sd_card_available = true;

    // read all bank names
    max_loaded_banks = get_bank_names();
    strip_extension(bank_names[bank], bank_name);

    // read all voice name for actual bank
    get_voice_names_from_bank(bank);
#ifdef DEBUG
    Serial.print(F("Bank ["));
    Serial.print(bank_names[bank]);
    Serial.print(F("/"));
    Serial.print(bank_name);
    Serial.println(F("]"));
    for (uint8_t n = 0; n < MAX_VOICES; n++)
    {
      if (n < 10)
        Serial.print(F(" "));
      Serial.print(F("   "));
      Serial.print(n, DEC);
      Serial.print(F("["));
      Serial.print(voice_names[n]);
      Serial.println(F("]"));
    }
#endif

    // Init effects
    antialias.setLowpass(0, 6000, 0.707);
    filter1.frequency(EXP_FUNC((float)map(effect_filter_frq, 0, ENC_FILTER_FRQ_STEPS, 0, 1024) / 150.0) * 10.0 + 80.0);
    //filter1.resonance(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0));
    filter1.resonance(EXP_FUNC(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0)) * 0.044 + 0.61);
    filter1.octaveControl(mapfloat(effect_filter_octave, 0, ENC_FILTER_OCT_STEPS, 0.0, 7.0));
    delay1.delay(0, mapfloat(effect_delay_feedback, 0, ENC_DELAY_TIME_STEPS, 0.0, DELAY_MAX_TIME));
    // mixer1 is the feedback-adding mixer, mixer2 the whole delay (with/without feedback) mixer
    mixer1.gain(0, 1.0); // original signal
    mixer1.gain(1, mapfloat(effect_delay_feedback, 0, ENC_DELAY_FB_STEPS, 0.0, 1.0)); // amount of feedback
    mixer1.gain(0, 0.0); // filtered signal off
    mixer1.gain(3, 1.0); // original signal on
    mixer2.gain(0, 1.0 - mapfloat(effect_delay_volume, 0, ENC_DELAY_VOLUME_STEPS, 0.0, 1.0)); // original signal
    mixer2.gain(1, mapfloat(effect_delay_volume, 0, ENC_DELAY_VOLUME_STEPS, 0.0, 1.0)); // delayed signal (including feedback)
    mixer2.gain(2, mapfloat(effect_delay_volume, 0, ENC_DELAY_VOLUME_STEPS, 0.0, 1.0)); // only delayed signal (without feedback)

    // load default SYSEX data
    load_sysex(bank, voice);
  }

#ifdef I2C_DISPLAY
  enc[0].write(map(vol * 100, 0, 100, 0, ENC_VOL_STEPS));
  enc_val[0] = enc[0].read();
  enc[1].write(voice);
  enc_val[1] = enc[1].read();
  but[0].update();
  but[1].update();
#endif

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
#endif

#ifdef DEBUG
  Serial.print(F("Bank/Voice from EEPROM ["));
  Serial.print(EEPROM.read(EEPROM_OFFSET + EEPROM_BANK_ADDR), DEC);
  Serial.print(F("/"));
  Serial.print(EEPROM.read(EEPROM_OFFSET +  EEPROM_VOICE_ADDR), DEC);
  Serial.println(F("]"));
  show_patch();
#endif

  Serial.print(F("AUDIO_BLOCK_SAMPLES="));
  Serial.print(AUDIO_BLOCK_SAMPLES);
  Serial.print(F(" (Time per block="));
  Serial.print(1000000 / (SAMPLE_RATE / AUDIO_BLOCK_SAMPLES));
  Serial.println(F("ms)"));

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
  show_cpu_and_mem_usage();
#endif

#ifdef I2C_DISPLAY
  lcd.clear();
  ui_show_main();
#endif

  AudioInterrupts();
  Serial.println(F("<setup end>"));
}

void loop()
{
  int16_t* audio_buffer; // pointer to AUDIO_BLOCK_SAMPLES * int16_t
  const uint16_t audio_block_time_us = 1000000 / (SAMPLE_RATE / AUDIO_BLOCK_SAMPLES);

  // Main sound calculation
  if (queue1.available() && fill_audio_buffer > audio_block_time_us - 10)
  {
    fill_audio_buffer = 0;

    audio_buffer = queue1.getBuffer();

    elapsedMicros t1;
    dexed->getSamples(AUDIO_BLOCK_SAMPLES, audio_buffer);
    if (t1 > audio_block_time_us) // everything greater 2.9ms is a buffer underrun!
      xrun++;
    if (t1 > render_time_max)
      render_time_max = t1;
    if (peak1.available())
    {
      if (peak1.read() > 0.99)
        peak++;
    }
#ifndef TEENSY_AUDIO_BOARD
    for (uint8_t i = 0; i < AUDIO_BLOCK_SAMPLES; i++)
      audio_buffer[i] *= vol;
#endif
    queue1.playBuffer();
  }

  // EEPROM update handling
  if (eeprom_update_status > 0 && autostore >= autostore_value)
  {
    autostore = 0;
    eeprom_update();
  }

  // MIDI input handling
  check_midi_devices();

  // Shutdown unused voices
  if (control_rate > CONTROL_RATE_MS)
  {
    control_rate = 0;
    uint8_t shutdown_voices = dexed->getNumNotesPlaying();
  }

#ifdef I2C_DISPLAY
  // UI
  if (master_timer >= TIMER_UI_HANDLING_MS)
  {
    master_timer -= TIMER_UI_HANDLING_MS;

    handle_ui();
  }
#endif

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
  if (cpu_mem_millis >= SHOW_CPU_LOAD_MSEC)
  {
    cpu_mem_millis -= SHOW_CPU_LOAD_MSEC;
    show_cpu_and_mem_usage();
  }
#endif
}

/******************************************************************************
   MIDI MESSAGE HANDLER
 ******************************************************************************/
void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity)
{
  if (checkMidiChannel(inChannel))
  {
    dexed->keydown(inNumber, inVelocity);
  }
}

void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity)
{
  if (checkMidiChannel(inChannel))
  {
    dexed->keyup(inNumber);
  }
}

void handleControlChange(byte inChannel, byte inCtrl, byte inValue)
{
  if (checkMidiChannel(inChannel))
  {
#ifdef DEBUG
    Serial.print(F("CC#"));
    Serial.print(inCtrl, DEC);
    Serial.print(F(":"));
    Serial.println(inValue, DEC);
#endif

    switch (inCtrl) {
      case 0:
        if (inValue < MAX_BANKS)
        {
          bank = inValue;
          handle_ui();
        }
        break;
      case 1:
        dexed->controllers.modwheel_cc = inValue;
        dexed->controllers.refresh();
        break;
      case 2:
        dexed->controllers.breath_cc = inValue;
        dexed->controllers.refresh();
        break;
      case 4:
        dexed->controllers.foot_cc = inValue;
        dexed->controllers.refresh();
        break;
      case 7: // Volume
        vol = float(inValue) / 0x7f;
        set_volume(vol, pan);
        break;
      case 10: // Pan
        pan = float(inValue) / 128;
        set_volume(vol, pan);
        break;
      case 32: // BankSelect LSB
        bank = inValue;
        break;
      case 64:
        dexed->setSustain(inValue > 63);
        if (!dexed->getSustain()) {
          for (uint8_t note = 0; note < dexed->getMaxNotes(); note++) {
            if (dexed->voices[note].sustained && !dexed->voices[note].keydown) {
              dexed->voices[note].dx7_note->keyup();
              dexed->voices[note].sustained = false;
            }
          }
        }
        break;
      case 102:  // CC 102: filter frequency
        effect_filter_frq = map(inValue, 0, 127, 0, ENC_FILTER_FRQ_STEPS);
        if (effect_filter_frq == ENC_FILTER_FRQ_STEPS)
        {
          // turn "off" filter
          mixer1.gain(0, 0.0); // filtered signal off
          mixer1.gain(3, 1.0); // original signal on
        }
        else
        {
          // turn "on" filter
          mixer1.gain(0, 1.0); // filtered signal on
          mixer1.gain(3, 0.0); // original signal off
        }
        filter1.frequency(EXP_FUNC((float)map(effect_filter_frq, 0, ENC_FILTER_FRQ_STEPS, 0, 1024) / 150.0) * 10.0 + 80.0);
        handle_ui();
        break;
      case 103:  // CC 103: filter resonance
        effect_filter_resonance = map(inValue, 0, 127, 0, ENC_FILTER_RES_STEPS);
        filter1.resonance(EXP_FUNC(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0)) * 0.044 + 0.61);
        handle_ui();
        break;
      case 104:  // CC 104: filter octave
        effect_filter_octave = map(inValue, 0, 127, 0, ENC_FILTER_OCT_STEPS);
        filter1.octaveControl(mapfloat(effect_filter_octave, 0, ENC_FILTER_OCT_STEPS, 0.0, 7.0));
        handle_ui();
        break;
      case 105:  // CC 105: delay time
        effect_delay_time = map(inValue, 0, 127, 0, ENC_DELAY_TIME_STEPS);
        delay1.delay(0, mapfloat(effect_delay_time, 0, ENC_DELAY_TIME_STEPS, 0.0, DELAY_MAX_TIME));
        handle_ui();
        break;
      case 106:  // CC 106: delay feedback
        effect_delay_feedback = map(inValue, 0, 127, 0, ENC_DELAY_FB_STEPS);
        mixer1.gain(1, mapfloat(float(effect_delay_feedback), 0, ENC_DELAY_FB_STEPS, 0.0, 1.0));
        handle_ui();
        break;
      case 107:  // CC 107: delay volume
        effect_delay_volume = map(inValue, 0, 127, 0, ENC_DELAY_VOLUME_STEPS);
        mixer2.gain(1, mapfloat(effect_delay_volume, 0, ENC_DELAY_VOLUME_STEPS, 0.0, 1.0)); // delay tap1 signal (with added feedback)
        handle_ui();
        break;
      case 120:
        dexed->panic();
        break;
      case 121:
        dexed->resetControllers();
        break;
      case 123:
        dexed->notesOff();
        break;
      case 126:
        dexed->setMonoMode(true);
        break;
      case 127:
        dexed->setMonoMode(false);
        break;
    }
  }
}

void handleAfterTouch(byte inChannel, byte inPressure)
{
  dexed->controllers.aftertouch_cc = inPressure;
  dexed->controllers.refresh();
}

void handlePitchBend(byte inChannel, int inPitch)
{
  dexed->controllers.values_[kControllerPitch] = inPitch + 0x2000; // -8192 to +8191 --> 0 to 16383
}

void handleProgramChange(byte inChannel, byte inProgram)
{
  if (inProgram < MAX_VOICES)
  {
    load_sysex(bank, inProgram);
    handle_ui();
  }
}

void handleSystemExclusive(byte *sysex, uint len)
{
#ifdef DEBUG
  Serial.print(F("SYSEX-Data["));
  Serial.print(len, DEC);
  Serial.print(F("]"));
  for (uint8_t i = 0; i < len; i++)
  {
    Serial.print(F(" "));
    Serial.print(sysex[i], DEC);
  }
  Serial.println();
#endif

  if (sysex[1] != 0x43) // check for Yamaha sysex
  {
#ifdef DEBUG
    Serial.println(F("E: SysEx vendor not Yamaha."));
#endif
    return;
  }

  // parse parameter change
  if (len == 7)
  {
    if ((sysex[3] & 0x7c) != 0 || (sysex[3] & 0x7c) != 2)
    {
#ifdef DEBUG
      Serial.println(F("E: Not a SysEx parameter or function parameter change."));
#endif
      return;
    }
    if (sysex[6] != 0xf7)
    {
#ifdef DEBUG
      Serial.println(F("E: SysEx end status byte not detected."));
#endif
      return;
    }
    if ((sysex[3] & 0x7c) == 0)
    {
      dexed->notesOff();
      dexed->data[sysex[4]] = sysex[5]; // set parameter
      dexed->doRefreshVoice();
    }
    else
    {
      dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET - 63 + sysex[4]] = sysex[5]; // set function parameter
      dexed->controllers.values_[kControllerPitchRange] = dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_RANGE];
      dexed->controllers.values_[kControllerPitchStep] = dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_STEP];
      dexed->controllers.wheel.setRange(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_RANGE]);
      dexed->controllers.wheel.setTarget(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_ASSIGN]);
      dexed->controllers.foot.setRange(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_RANGE]);
      dexed->controllers.foot.setTarget(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_ASSIGN]);
      dexed->controllers.breath.setRange(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_RANGE]);
      dexed->controllers.breath.setTarget(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_ASSIGN]);
      dexed->controllers.at.setRange(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_RANGE]);
      dexed->controllers.at.setTarget(dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_ASSIGN]);
      dexed->controllers.masterTune = (dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MASTER_TUNE] * 0x4000 << 11) * (1.0 / 12);
      dexed->controllers.refresh();
    }
#ifdef DEBUG
    Serial.print(F("SysEx"));
    if ((sysex[3] & 0x7c) == 0)
      Serial.print(F(" function"));
    Serial.print(F(" parameter "));
    Serial.print(sysex[4], DEC);
    Serial.print(F(" = "));
    Serial.println(sysex[5], DEC);
#endif
  }
#ifdef DEBUG
  else
    Serial.println(F("E: SysEx parameter length wrong."));
#endif
}

void handleTimeCodeQuarterFrame(byte data)
{
  ;
}

void handleAfterTouchPoly(byte inChannel, byte inNumber, byte inVelocity)
{
  ;
}

void handleSongSelect(byte inSong)
{
  ;
}

void handleTuneRequest(void)
{
  ;
}

void handleClock(void)
{
  midi_timing_counter++;
  if (midi_timing_counter % 24 == 0)
  {
    midi_timing_quarter = midi_timing_timestep;
    midi_timing_counter = 0;
    midi_timing_timestep = 0;
    // Adjust delay control here
#ifdef DEBUG
    Serial.print(F("MIDI Clock: "));
    Serial.print(60000 / midi_timing_quarter, DEC);
    Serial.print(F("bpm ("));
    Serial.print(midi_timing_quarter, DEC);
    Serial.println(F("ms per quarter)"));
#endif
  }
}

void handleStart(void)
{
  ;
}

void handleContinue(void)
{
  ;
}

void handleStop(void)
{
  ;
}

void handleActiveSensing(void)
{
  ;
}

void handleSystemReset(void)
{
#ifdef DEBUG
  Serial.println(F("MIDI SYSEX RESET"));
#endif
  dexed->notesOff();
  dexed->panic();
  dexed->resetControllers();
}

/******************************************************************************
   END OF MIDI MESSAGE HANDLER
 ******************************************************************************/

bool checkMidiChannel(byte inChannel)
{
  // check for MIDI channel
  if (midi_channel == MIDI_CHANNEL_OMNI)
  {
    return (true);
  }
  else if (inChannel != midi_channel)
  {
#ifdef DEBUG
    Serial.print(F("Ignoring MIDI data on channel "));
    Serial.print(inChannel);
    Serial.print(F("(listening on "));
    Serial.print(midi_channel);
    Serial.println(F(")"));
#endif
    return (false);
  }
  return (true);
}

void set_volume(float v, float p)
{
  vol = v;
  pan = p;

#ifdef DEBUG
  uint8_t tmp;
  Serial.print(F("Setting volume: VOL="));
  Serial.print(v, DEC);
  Serial.print(F("["));
  tmp = EEPROM.read(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR);
  Serial.print(tmp, DEC);
  Serial.print(F("/"));
  Serial.print(float(tmp) / UCHAR_MAX, DEC);
  Serial.print(F("] PAN="));
  Serial.print(F("["));
  tmp = EEPROM.read(EEPROM_OFFSET + EEPROM_PAN_ADDR);
  Serial.print(tmp, DEC);
  Serial.print(F("/"));
  Serial.print(float(tmp) / SCHAR_MAX, DEC);
  Serial.print(F("] "));
  Serial.print(pow(v * sinf(p * PI / 2), VOLUME_CURVE), 3);
  Serial.print(F("/"));
  Serial.println(pow(v * cosf(p * PI / 2), VOLUME_CURVE), 3);
#endif

  // http://files.csound-tutorial.net/floss_manual/Release03/Cs_FM_03_ScrapBook/b-panning-and-spatialization.html
#ifdef TEENSY_AUDIO_BOARD
  sgtl5000_1.dacVolume(pow(v * sinf(p * PI / 2), VOLUME_CURVE), pow(v * cosf(p * PI / 2), VOLUME_CURVE));
#else
  volume_master.gain(VOLUME_CURVE);
  volume_r.gain(sinf(p * PI / 2));
  volume_l.gain(cosf(p * PI / 2));
#endif
}

// https://www.dr-lex.be/info-stuff/volumecontrols.html#table1
inline float logvol(float x)
{
  return (0.001 * expf(6.908 * x));
}

void initial_values_from_eeprom(void)
{
  uint32_t crc_eeprom = read_eeprom_checksum();
  uint32_t crc = eeprom_crc32(EEPROM_OFFSET, EEPROM_DATA_LENGTH);

#ifdef DEBUG
  Serial.print(F("EEPROM checksum: 0x"));
  Serial.print(crc_eeprom, HEX);
  Serial.print(F(" / 0x"));
  Serial.print(crc, HEX);
#endif
  if (crc_eeprom != crc)
  {
#ifdef DEBUG
    Serial.print(F(" - mismatch -> initializing EEPROM!"));
#endif
    eeprom_write(EEPROM_UPDATE_BANK + EEPROM_UPDATE_VOICE + EEPROM_UPDATE_VOL + EEPROM_UPDATE_PAN + EEPROM_UPDATE_MIDICHANNEL);
  }
  else
  {
    bank = EEPROM.read(EEPROM_OFFSET + EEPROM_BANK_ADDR);
    voice = EEPROM.read(EEPROM_OFFSET + EEPROM_VOICE_ADDR);
    vol = float(EEPROM.read(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR)) / UCHAR_MAX;
    pan = float(EEPROM.read(EEPROM_OFFSET + EEPROM_PAN_ADDR)) / SCHAR_MAX;
    midi_channel = EEPROM.read(EEPROM_OFFSET + EEPROM_MIDICHANNEL_ADDR);
    if (midi_channel > 16)
      midi_channel = MIDI_CHANNEL_OMNI;
  }
#ifdef DEBUG
  Serial.println();
#endif
}

uint32_t read_eeprom_checksum(void)
{
  return (EEPROM.read(EEPROM_CRC32_ADDR) << 24 | EEPROM.read(EEPROM_CRC32_ADDR + 1) << 16 | EEPROM.read(EEPROM_CRC32_ADDR + 2) << 8 | EEPROM.read(EEPROM_CRC32_ADDR + 3));
}

void update_eeprom_checksum(void)
{
  write_eeprom_checksum(eeprom_crc32(EEPROM_OFFSET, EEPROM_DATA_LENGTH)); // recalculate crc and write to eeprom
}

void write_eeprom_checksum(uint32_t crc)
{
  EEPROM.update(EEPROM_CRC32_ADDR, (crc & 0xff000000) >> 24);
  EEPROM.update(EEPROM_CRC32_ADDR + 1, (crc & 0x00ff0000) >> 16);
  EEPROM.update(EEPROM_CRC32_ADDR + 2, (crc & 0x0000ff00) >> 8);
  EEPROM.update(EEPROM_CRC32_ADDR + 3, crc & 0x000000ff);
}

uint32_t eeprom_crc32(uint16_t calc_start, uint16_t calc_bytes) // base code from https://www.arduino.cc/en/Tutorial/EEPROMCrc
{
  const uint32_t crc_table[16] =
  {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };
  uint32_t crc = ~0L;

  if (calc_start + calc_bytes > EEPROM.length())
    calc_bytes = EEPROM.length() - calc_start;

  for (uint16_t index = calc_start ; index < (calc_start + calc_bytes) ; ++index)
  {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }

  return (crc);
}

void eeprom_write(uint8_t status)
{
  eeprom_update_status |= status;
  if (eeprom_update_status != 0)
    autostore = 0;
#ifdef DEBUG
  Serial.print(F("Updating EEPROM state to: "));
  Serial.println(eeprom_update_status, DEC);
#endif
}

void eeprom_update(void)
{
  autostore_value = AUTOSTORE_FAST_MS;

  if (eeprom_update_status & EEPROM_UPDATE_BANK)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_BANK_ADDR, bank);
#ifdef DEBUG
    Serial.println(F("Bank written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_BANK;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_VOICE)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOICE_ADDR, voice);
#ifdef DEBUG
    Serial.println(F("Voice written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_VOICE;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_VOL)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR, uint8_t(vol * UCHAR_MAX));
#ifdef DEBUG
    Serial.println(F("Volume written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_VOL;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_PAN)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_PAN_ADDR, uint8_t(pan * SCHAR_MAX));
#ifdef DEBUG
    Serial.println(F("Panorama written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_PAN;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_MIDICHANNEL )
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_MIDICHANNEL_ADDR, midi_channel);
    update_eeprom_checksum();
#ifdef DEBUG
    Serial.println(F("MIDI channel written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_MIDICHANNEL;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_CHECKSUM)
  {
    update_eeprom_checksum();
#ifdef DEBUG
    Serial.println(F("Checksum written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_CHECKSUM;
    autostore_value = AUTOSTORE_MS;
    return;
  }

  if (eeprom_update_status == 0)
    eeprom_update_status |= EEPROM_UPDATE_CHECKSUM;
}

void init_eeprom(void)
{
  for (uint8_t i = 0; i < EEPROM_DATA_LENGTH; i++)
  {
    EEPROM.update(EEPROM_OFFSET + i, 0);
  }
}

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
void show_cpu_and_mem_usage(void)
{
  Serial.print(F("CPU: "));
  Serial.print(AudioProcessorUsage(), 2);
  Serial.print(F("%   CPU MAX: "));
  Serial.print(AudioProcessorUsageMax(), 2);
  Serial.print(F("%  MEM: "));
  Serial.print(AudioMemoryUsage(), DEC);
  Serial.print(F("   MEM MAX: "));
  Serial.print(AudioMemoryUsageMax(), DEC);
  Serial.print(F("   RENDER_TIME_MAX: "));
  Serial.print(render_time_max, DEC);
  Serial.print(F("   XRUN: "));
  Serial.print(xrun, DEC);
  Serial.print(F("   OVERLOAD: "));
  Serial.print(overload, DEC);
  Serial.print(F("   PEAK: "));
  Serial.print(peak, DEC);
  Serial.print(F(" BLOCKSIZE: "));
  Serial.print(AUDIO_BLOCK_SAMPLES, DEC);
  Serial.println();
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
  render_time_max = 0;
}
#endif

#ifdef DEBUG
void show_patch(void)
{
  uint8_t i;
  char voicename[VOICE_NAME_LEN];

  memset(voicename, 0, sizeof(voicename));
  for (i = 0; i < 6; i++)
  {
    Serial.print(F("OP"));
    Serial.print(6 - i, DEC);
    Serial.println(F(": "));
    Serial.println(F("R1 | R2 | R3 | R4 | L1 | L2 | L3 | L4 LEV_SCL_BRK_PT | SCL_LEFT_DEPTH | SCL_RGHT_DEPTH"));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_R1], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_R2], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_R3], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_R4], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_L1], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_L2], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_L3], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_EG_L4], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_LEV_SCL_BRK_PT], DEC);
    Serial.print(F("             "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_SCL_LEFT_DEPTH], DEC);
    Serial.print(F("             "));
    Serial.println(dexed->data[(i * 21) + DEXED_OP_SCL_RGHT_DEPTH], DEC);
    Serial.println(F("SCL_L_CURVE | SCL_R_CURVE | RT_SCALE | AMS | KVS | OUT_LEV | OP_MOD | FRQ_C | FRQ_F | DETUNE"));
    Serial.print(F("      "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_SCL_LEFT_CURVE], DEC);
    Serial.print(F("         "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_SCL_RGHT_CURVE], DEC);
    Serial.print(F("         "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_OSC_RATE_SCALE], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_AMP_MOD_SENS], DEC);
    Serial.print(F("     "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_KEY_VEL_SENS], DEC);
    Serial.print(F("      "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_OUTPUT_LEV], DEC);
    Serial.print(F("      "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_OSC_MODE], DEC);
    Serial.print(F("    "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_FREQ_COARSE], DEC);
    Serial.print(F("     "));
    Serial.print(dexed->data[(i * 21) + DEXED_OP_FREQ_FINE], DEC);
    Serial.print(F("     "));
    Serial.println(dexed->data[(i * 21) + DEXED_OP_OSC_DETUNE], DEC);
  }
  Serial.println(F("PR1 | PR2 | PR3 | PR4 | PL1 | PL2 | PL3 | PL4"));
  Serial.print(F(" "));
  for (i = 0; i < 8; i++)
  {
    Serial.print(dexed->data[DEXED_VOICE_OFFSET + i], DEC);
    Serial.print(F("  "));
  }
  Serial.println();
  Serial.print(F("ALG: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_ALGORITHM], DEC);
  Serial.print(F("FB: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_FEEDBACK], DEC);
  Serial.print(F("OKS: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_OSC_KEY_SYNC], DEC);
  Serial.print(F("LFO SPD: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_SPEED], DEC);
  Serial.print(F("LFO_DLY: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_DELAY], DEC);
  Serial.print(F("LFO PMD: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_PITCH_MOD_DEP], DEC);
  Serial.print(F("LFO_AMD: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_AMP_MOD_DEP], DEC);
  Serial.print(F("LFO_SYNC: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_SYNC], DEC);
  Serial.print(F("LFO_WAVEFRM: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_WAVE], DEC);
  Serial.print(F("LFO_PMS: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_LFO_PITCH_MOD_SENS], DEC);
  Serial.print(F("TRNSPSE: "));
  Serial.println(dexed->data[DEXED_VOICE_OFFSET + DEXED_TRANSPOSE], DEC);
  Serial.print(F("NAME: "));
  strncpy(voicename, (char *)&dexed->data[DEXED_VOICE_OFFSET + DEXED_NAME], sizeof(voicename) - 1);
  Serial.print(F("["));
  Serial.print(voicename);
  Serial.println(F("]"));
  for (i = DEXED_GLOBAL_PARAMETER_OFFSET; i <= DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MAX_NOTES; i++)
  {
    Serial.print(i, DEC);
    Serial.print(F(": "));
    Serial.println(dexed->data[i]);
  }

  Serial.println();
}
#endif
