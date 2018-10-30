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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
#include <EEPROM.h>
#if defined(USBCON)
#include <midi_UsbTransport.h>
#endif
#include <limits.h>
#include "dexed.h"
#include "dexed_sysex.h"
#include "config.h"
#ifdef USE_ONBOARD_USB_HOST
#include <USBHost_t36.h>
#endif

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

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=179,325
AudioAnalyzePeak         peak1;          //xy=348,478
AudioFilterStateVariable filter1;        //xy=415,334
AudioEffectDelay         delay1;         //xy=732,485
AudioMixer4              mixer1;         //xy=734,245
AudioMixer4              mixer2;         //xy=1055,317
AudioConnection          patchCord1(queue1, peak1);
AudioConnection          patchCord2(queue1, 0, filter1, 0);
AudioConnection          patchCord3(filter1, 0, delay1, 0);
AudioConnection          patchCord4(filter1, 0, mixer1, 0);
AudioConnection          patchCord5(delay1, 0, mixer1, 1);
AudioConnection          patchCord6(delay1, 0, mixer2, 1);
AudioConnection          patchCord7(mixer1, delay1);
AudioConnection          patchCord8(queue1, 0, mixer1, 3); // for disabling the filter
AudioConnection          patchCord9(mixer1, 0, mixer2, 0);
#ifdef TEENSY_AUDIO_BOARD
AudioOutputI2S           i2s1;           //xy=1200,432
AudioControlSGTL5000     sgtl5000_1;     //xy=197,554
AudioConnection          patchCord10(mixer2, 0, i2s1, 0);
AudioConnection          patchCord11(mixer2, 0, i2s1, 1);
#else
AudioOutputPT8211        pt8211_1;       //xy=1079,320
AudioAmplifier           volume_master;           //xy=678,393
AudioAmplifier           volume_r;           //xy=818,370
AudioAmplifier           volume_l;           //xy=818,411
AudioConnection          patchCord10(mixer2, 0, volume_master, 0);
AudioConnection          patchCord11(volume_master, volume_r);
AudioConnection          patchCord12(volume_master, volume_l);
AudioConnection          patchCord13(volume_r, 0, pt8211_1, 0);
AudioConnection          patchCord14(volume_l, 0, pt8211_1, 1);
#endif
// GUItool: end automatically generated code

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
float vol_right = 1.0;
float vol_left = 1.0;
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

#ifdef MASTER_KEY_MIDI
bool master_key_enabled = false;
#endif

#ifdef SHOW_CPU_LOAD_MSEC
elapsedMillis cpu_mem_millis;
#endif

#ifdef MIDI_DEVICE
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_DEVICE, midi_serial);
#endif
#ifdef USE_ONBOARD_USB_HOST
USBHost usb_host;
MIDIDevice midi_usb(usb_host);
#endif
#if defined(USBCON)
static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;
UsbTransport sUsbTransport;
MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, midi_onboard_usb);
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
  lcd.show(0, 0, 16, "   MicroDexed");
  lcd.show(1, 0, 16, "(c)parasiTstudio");

  pinMode(BUT_L_PIN, INPUT_PULLUP);
  pinMode(BUT_R_PIN, INPUT_PULLUP);
#endif

  delay(220);
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018 H. Wirtz <wirtz@parasitstudio.de>"));
  Serial.println(F("https://github.com/dcoredump/MicroDexed"));
  Serial.println(F("<setup start>"));
  initial_values_from_eeprom();

  // start up USB host
#ifdef USE_ONBOARD_USB_HOST
  usb_host.begin();
  Serial.println(F("USB-MIDI enabled."));
#endif

  // check for onboard USB-MIDI
#if defined(USBCON)
  midi_onboard_usb.begin();
  Serial.println(F("Onboard USB-MIDI enabled."));
#endif

#ifdef MIDI_DEVICE
  // Start serial MIDI
  midi_serial.begin(DEFAULT_MIDI_CHANNEL);
  Serial.println(F("Serial MIDI enabled"));
#endif

  // start audio card
  AudioMemory(AUDIO_MEM);
#ifdef TEENSY_AUDIO_BOARD
  sgtl5000_1.enable();
  sgtl5000_1.dacVolumeRamp();
  //sgtl5000_1.dacVolumeRampLinear();
  sgtl5000_1.unmuteHeadphone();
  sgtl5000_1.unmuteLineout();
  sgtl5000_1.autoVolumeDisable(); // turn off AGC
  sgtl5000_1.volume(1.0, 1.0);
  sgtl5000_1.lineOutLevel(31);
  Serial.println(F("Teensy-Audio-Board enabled."));
#else
  Serial.println(F("PT8211 enabled."));
#endif
  set_volume(vol, vol_left, vol_right);

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
    filter1.frequency(EXP_FUNC((float)map(effect_filter_frq, 0, ENC_FILTER_FRQ_STEPS, 0, 1024) / 150.0) * 10.0 + 80.0);
    //filter1.resonance(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0));
    filter1.resonance(EXP_FUNC(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0)) * 0.044 + 0.61);
    filter1.octaveControl(mapfloat(effect_filter_octave, 0, ENC_FILTER_OCT_STEPS, 0.0, 7.0));
    delay1.delay(0, mapfloat(effect_delay_feedback, 0, ENC_DELAY_TIME_STEPS, 0.0, DELAY_MAX_TIME));
    // mixer1 is the feedback-adding mixer, mixer2 the whole delay (with/without feedback) mixer
    mixer1.gain(0, 1.0); // original signal
    mixer1.gain(1, mapfloat(effect_delay_feedback, 0, 99, 0.0, 1.0)); // amount of feedback
    mixer1.gain(0, 0.0); // filtered signal off
    mixer1.gain(3, 1.0); // original signal on
    mixer2.gain(0, 1.0); // original signal
    mixer2.gain(1, mapfloat(effect_delay_volume, 0, 99, 0.0, 1.0)); // delayed signal (including feedback)

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

#ifdef TEST_NOTE
  Serial.println(F("MIDI test enabled"));
  sched_note_on.begin(note_on, 2000000);
  sched_note_off.begin(note_off, 6333333);
#endif

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
  show_cpu_and_mem_usage();
#endif

#ifdef I2C_DISPLAY
  lcd.clear();
  ui_show_main();
#endif

  Serial.println(F("<setup end>"));

#ifdef TEST_NOTE
  //dexed->data[DEXED_VOICE_OFFSET+DEXED_LFO_PITCH_MOD_DEP] = 99;           // full pitch mod depth
  //dexed->data[DEXED_VOICE_OFFSET+DEXED_LFO_PITCH_MOD_SENS] = 99;          // full pitch mod sense
  //dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET+DEXED_MODWHEEL_ASSIGN] = 7;   // mod wheel assign all
  //dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET+DEXED_FOOTCTRL_ASSIGN] = 7;   // foot ctrl assign all
  //dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET+DEXED_BREATHCTRL_ASSIGN] = 7; // breath ctrl assign all
  //dexed->data[DEXED_GLOBAL_PARAMETER_OFFSET+AT_ASSIGN] = 7;               // at ctrl assign all
  //queue_midi_event(0xb0, 1, 99); // test mod wheel
  //queue_midi_event(0xb0, 2, 99); // test breath ctrl
  //queue_midi_event(0xb0, 4, 99); // test food switch
  //queue_midi_event(0xd0, 4, 99); // test at
  //queue_midi_event(0xe0, 0xff, 0xff); // test pitch bend
#endif
}

void loop()
{
  int16_t* audio_buffer; // pointer to AUDIO_BLOCK_SAMPLES * int16_t
  const uint16_t audio_block_time_ms = 1000000 / (SAMPLE_RATE / AUDIO_BLOCK_SAMPLES);

  // Main sound calculation
  if (queue1.available())
  {
    audio_buffer = queue1.getBuffer();

    elapsedMicros t1;
    dexed->getSamples(AUDIO_BLOCK_SAMPLES, audio_buffer);
    if (t1 > audio_block_time_ms) // everything greater 2.9ms is a buffer underrun!
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
  handle_input();
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

void handle_input(void)
{
#if defined(USBCON)
  while (midi_onboard_usb.read())
  {
#ifdef DEBUG
    Serial.println(F("[ONBOARD-MIDI-USB]"));
#endif
    if (midi_onboard_usb.getType() >= 0xf0) // SysEX
    {
      handle_sysex_parameter(midi_onboard_usb.getSysExArray(), midi_onboard_usb.getSysExArrayLength());
    }
    else
    {
      queue_midi_event(midi_onboard_usb.getType(), midi_onboard_usb.getData1(), midi_onboard_usb.getData2())
#ifdef MIDI_MERGE_THRU
      midi_serial.send(midi_serial.getType(), midi_serial.getData1(), midi_serial.getData2(), midi_serial.getChannel());
#endif
    }
  }
#endif
#ifdef USE_ONBOARD_USB_HOST
  usb_host.Task();
  while (midi_usb.read())
  {
#ifdef DEBUG
    Serial.println(F("[MIDI-USB]"));
#endif
    if (midi_usb.getType() >= 0xf0) // SysEX
    {
      handle_sysex_parameter(midi_usb.getSysExArray(), midi_usb.getSysExArrayLength());
    }
    else
    {
      queue_midi_event(midi_usb.getType(), midi_usb.getData1(), midi_usb.getData2());
#ifdef MIDI_MERGE_THRU
      midi_serial.send(midi_serial.getType(), midi_serial.getData1(), midi_serial.getData2(), midi_serial.getChannel());
#endif
    }
  }
#endif
#ifdef MIDI_DEVICE
  while (midi_serial.read())
  {
#ifdef DEBUG
    Serial.print(F("[MIDI-Serial] "));
#endif
    if (midi_serial.getType() >= 0xf0) // SYSEX
    {
      handle_sysex_parameter(midi_serial.getSysExArray(), midi_serial.getSysExArrayLength());
    }
    else
    {
      queue_midi_event(midi_serial.getType(), midi_serial.getData1(), midi_serial.getData2());
#ifdef MIDI_MERGE_THRU
      midi_serial.send(midi_serial.getType(), midi_serial.getData1(), midi_serial.getData2(), midi_serial.getChannel());
#endif
    }
  }
#endif
}

#ifdef DEBUG
#ifdef SHOW_MIDI_EVENT
void print_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  Serial.print(F("Listen MIDI-Channel: "));
  if (midi_channel == MIDI_CHANNEL_OMNI)
    Serial.print(F("OMNI"));
  else
    Serial.print(midi_channel, DEC);
  Serial.print(F(", MIDI event type: 0x"));
  if (type < 16)
    Serial.print(F("0"));
  Serial.print(type, HEX);
  Serial.print(F(", incoming MIDI channel: "));
  Serial.print((type & 0x0f) + 1, DEC);
  Serial.print(F(", data1: "));
  Serial.print(data1, DEC);
  Serial.print(F(", data2: "));
  Serial.println(data2, DEC);
}
#endif
#endif

#ifdef MASTER_KEY_MIDI
bool handle_master_key(uint8_t data)
{
  int8_t num = num_key_base_c(data);

#ifdef DEBUG
  Serial.print(F("Master-Key: "));
  Serial.println(num, DEC);
#endif

  if (num > 0)
  {
    // a white key!
    if (num <= 32)
    {
      if (load_sysex(bank, num))
      {
#ifdef DEBUG
        Serial.print(F("Loading voice number "));
        Serial.println(num, DEC);
#endif
        eeprom_write(EEPROM_UPDATE_VOICE);
#ifdef I2C_DISPLAY
        lcd.show(1, 0, 2, voice + 1);
        lcd.show(1, 2, 1, " ");
        lcd.show(1, 3, 10, voice_names[voice]);
#endif
      }
#ifdef DEBUG
      else
      {
        Serial.print(F("E: cannot load voice number "));
        Serial.println(num, DEC);
      }
#endif
    }
    return (true);
  }
  else
  {
    // a black key!
    num = abs(num);
    if (num <= 10)
    {
      set_volume(float(num * 0.1), vol_left, vol_right);
    }
    else if (num > 10 && num <= 20)
    {
      bank = num - 10;
#ifdef DEBUG
      Serial.print(F("Bank switch to: "));
      Serial.println(bank, DEC);
#endif
      eeprom_write(EEPROM_UPDATE_BANK);
#ifdef I2C_DISPLAY
      if (get_voice_names_from_bank(bank))
      {
        strip_extension(bank_names[bank], bank_name);
        lcd.show(0, 0, 2, bank);
        lcd.show(0, 2, 1, " ");
        lcd.show(0, 3, 10, bank_name);
      }
      else
      {
        lcd.show(0, 0, 2, bank);
        lcd.show(0, 2, 10, " *ERROR*");
      }
#endif
      return (true);
    }
  }
  return (false);
}
#endif

bool queue_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  bool ret = false;

#if defined(DEBUG) && defined(SHOW_MIDI_EVENT)
  print_midi_event(type, data1, data2);
#endif

  // check for MIDI channel
  if (midi_channel != MIDI_CHANNEL_OMNI)
  {
    uint8_t c = type & 0x0f;
    if (c != midi_channel - 1)
    {
#ifdef DEBUG
      Serial.print(F("Ignoring MIDI data on channel "));
      Serial.print(c);
      Serial.print(F("(listening on "));
      Serial.print(midi_channel);
      Serial.println(F(")"));
#endif
      return (false);
    }
  }

  // now throw away the MIDI channel information
  type &= 0xf0;

#ifdef MASTER_KEY_MIDI
  if (type == 0x80 && data1 == MASTER_KEY_MIDI) // Master key released
  {
    master_key_enabled = false;
#ifdef DEBUG
    Serial.println(F("Master key disabled"));
#endif
  }
  else if (type == 0x90 && data1 == MASTER_KEY_MIDI) // Master key pressed
  {
    master_key_enabled = true;
#ifdef DEBUG
    Serial.println(F("Master key enabled"));
#endif
  }
  else
  {
    if (master_key_enabled)
    {
      if (type == 0x80) // handle when note is released
      {
        dexed->notesOff();
        handle_master_key(data1);
      }
    }
    else
#endif
    {
      if (type == 0xb0)
      {
        switch (data1)
        {
          case 0x66:  // CC 102: filter frequency
            effect_filter_frq = map(data2, 0, 127, 0, ENC_FILTER_FRQ_STEPS);
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
            break;
          case 0x67:  // CC 103: filter resonance
            effect_filter_resonance = map(data2, 0, 127, 0, ENC_FILTER_RES_STEPS);
            filter1.resonance(EXP_FUNC(mapfloat(effect_filter_resonance, 0, ENC_FILTER_RES_STEPS, 0.7, 5.0)) * 0.044 + 0.61);
            break;
          case 0x68:  // CC 104: filter octave
            effect_filter_octave = map(data2, 0, 127, 0, ENC_FILTER_OCT_STEPS);
            filter1.octaveControl(mapfloat(effect_filter_octave, 0, ENC_FILTER_OCT_STEPS, 0.0, 7.0));
            break;
          case 0x69:  // CC 105: delay time
            effect_delay_time = map(data2, 0, 127, 0, ENC_DELAY_TIME_STEPS);
            delay1.delay(0, mapfloat(effect_delay_time, 0, ENC_DELAY_TIME_STEPS, 0.0, DELAY_MAX_TIME));
            break;
          case 0x6A:  // CC 106: delay feedback
            effect_delay_feedback = map(data2, 0, 127, 0, ENC_DELAY_FB_STEPS);
            mixer1.gain(1, mapfloat(float(effect_delay_feedback), 0, ENC_DELAY_FB_STEPS, 0.0, 1.0));
            break;
          case 0x6B:  // CC 107: delay volume
            effect_delay_volume = map(data2, 0, 127, 0, ENC_DELAY_VOLUME_STEPS);
            mixer2.gain(1, mapfloat(effect_delay_volume, 0, 99, 0.0, 1.0)); // delay tap1 signal (with added feedback)
            break;
          default:
            break;
        }
      }
      else
        ret = dexed->processMidiMessage(type, data1, data2);
    }

#ifdef MASTER_KEY_MIDI
  }
#endif
  return (ret);
}

#ifdef MASTER_KEY_MIDI
int8_t num_key_base_c(uint8_t midi_note)
{
  int8_t num = 0;

  switch (midi_note % 12)
  {
    // positive numbers are white keys, negative black ones
    case 0:
      num = 1;
      break;
    case 1:
      num = -1;
      break;
    case 2:
      num = 2;
      break;
    case 3:
      num = -2;
      break;
    case 4:
      num = 3;
      break;
    case 5:
      num = 4;
      break;
    case 6:
      num = -3;
      break;
    case 7:
      num = 5;
      break;
    case 8:
      num = -4;
      break;
    case 9:
      num = 6;
      break;
    case 10:
      num = -5;
      break;
    case 11:
      num = 7;
      break;
  }

  if (num > 0)
    return (num + (((midi_note - MASTER_NUM1) / 12) * 7));
  else
    return (num + ((((midi_note - MASTER_NUM1) / 12) * 5) * -1));
}
#endif

void set_volume(float v, float vr, float vl)
{
  vol = v;
  vol_right = vr;
  vol_left = vl;

#ifdef DEBUG
  uint8_t tmp;
  Serial.print(F("Setting volume: VOL="));
  Serial.print(v, DEC);
  Serial.print(F("["));
  tmp = EEPROM.read(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR);
  Serial.print(tmp, DEC);
  Serial.print(F("/"));
  Serial.print(float(tmp) / UCHAR_MAX, DEC);
  Serial.print(F("] VOL_L="));
  Serial.print(vl, DEC);
  Serial.print(F("["));
  tmp = EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR);
  Serial.print(tmp, DEC);
  Serial.print(F("/"));
  Serial.print(float(tmp) / UCHAR_MAX, DEC);
  Serial.print(F("] VOL_R="));
  Serial.print(vr, DEC);
  Serial.print(F("["));
  tmp = EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR);
  Serial.print(tmp, DEC);
  Serial.print(F("/"));
  Serial.print(float(tmp) / UCHAR_MAX, DEC);
  Serial.println(F("]"));
#endif

#ifdef TEENSY_AUDIO_BOARD
  //sgtl5000_1.dacVolume(vol * vol_left, vol * vol_right);
  sgtl5000_1.dacVolume(pow(vol * vol_left, 0.2), pow(vol * vol_right, 0.2));
#else
  volume_master.gain(pow(vol, 0.2));
  volume_r.gain(pow(vr, 0.2));
  volume_l.gain(pow(vl, 0.2));
#endif
}

void handle_sysex_parameter(const uint8_t* sysex, uint8_t len)
{
  if (sysex[0] != 240)
  {
    switch (sysex[0])
    {
      case 241: // MIDI Time Code Quarter Frame
        break;
      case 248: // Timing Clock (24 frames per quarter note)
        midi_timing_counter++;
        if (midi_timing_counter % 24 == 0)
        {
          midi_timing_quarter = midi_timing_timestep;
          midi_timing_counter = 0;
          midi_timing_timestep = 0;
          // Adjust delay control here
#ifdef DEBUG
          Serial.print(F("MIDI Timing: "));
          Serial.print(60000 / midi_timing_quarter, DEC);
          Serial.print(F("bpm ("));
          Serial.print(midi_timing_quarter, DEC);
          Serial.println(F("ms per quarter)"));
#endif
        }
        break;
      case 255: // Reset To Power Up
#ifdef DEBUG
        Serial.println(F("MIDI SYSEX RESET"));
#endif
        dexed->notesOff();
        dexed->panic();
        dexed->resetControllers();
        break;
    }
  }
  else
  {
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
    eeprom_write(EEPROM_UPDATE_BANK & EEPROM_UPDATE_VOICE & EEPROM_UPDATE_VOL & EEPROM_UPDATE_VOL_R & EEPROM_UPDATE_VOL_L & EEPROM_UPDATE_MIDICHANNEL);
  }
  else
  {
    bank = EEPROM.read(EEPROM_OFFSET + EEPROM_BANK_ADDR);
    voice = EEPROM.read(EEPROM_OFFSET + EEPROM_VOICE_ADDR);
    vol = float(EEPROM.read(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR)) / UCHAR_MAX;
    vol_right = float(EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR)) / UCHAR_MAX;
    vol_left = float(EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR)) / UCHAR_MAX;
    midi_channel = EEPROM.read(EEPROM_OFFSET + EEPROM_MIDICHANNEL_ADDR);
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
  Serial.print(F("Updating EEPROM to state to: "));
  Serial.println(eeprom_update_status);
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
  else if (eeprom_update_status & EEPROM_UPDATE_VOL_R)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR, uint8_t(vol_right * UCHAR_MAX));
#ifdef DEBUG
    Serial.println(F("Volume right written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_VOL_R;
  }
  else if (eeprom_update_status & EEPROM_UPDATE_VOL_L)
  {
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR, uint8_t(vol_left * UCHAR_MAX));
#ifdef DEBUG
    Serial.println(F("Volume left written to EEPROM"));
#endif
    eeprom_update_status &= ~EEPROM_UPDATE_VOL_L;
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

#ifdef TEST_NOTE
void note_on(void)
{
  randomSeed(analogRead(A0));
  queue_midi_event(0x90, TEST_NOTE, random(TEST_VEL_MIN, TEST_VEL_MAX));           // 1
  queue_midi_event(0x90, TEST_NOTE + 5, random(TEST_VEL_MIN, TEST_VEL_MAX));       // 2
  queue_midi_event(0x90, TEST_NOTE + 8, random(TEST_VEL_MIN, TEST_VEL_MAX));       // 3
  queue_midi_event(0x90, TEST_NOTE + 12, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 4
  queue_midi_event(0x90, TEST_NOTE + 17, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 5
  queue_midi_event(0x90, TEST_NOTE + 20, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 6
  queue_midi_event(0x90, TEST_NOTE + 24, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 7
  queue_midi_event(0x90, TEST_NOTE + 29, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 8
  queue_midi_event(0x90, TEST_NOTE + 32, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 9
  queue_midi_event(0x90, TEST_NOTE + 37, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 10
  queue_midi_event(0x90, TEST_NOTE + 40, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 11
  queue_midi_event(0x90, TEST_NOTE + 46, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 12
  queue_midi_event(0x90, TEST_NOTE + 49, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 13
  queue_midi_event(0x90, TEST_NOTE + 52, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 14
  queue_midi_event(0x90, TEST_NOTE + 57, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 15
  queue_midi_event(0x90, TEST_NOTE + 60, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 16
}

void note_off(void)
{
  queue_midi_event(0x80, TEST_NOTE, 0);           // 1
  queue_midi_event(0x80, TEST_NOTE + 5, 0);       // 2
  queue_midi_event(0x80, TEST_NOTE + 8, 0);       // 3
  queue_midi_event(0x80, TEST_NOTE + 12, 0);      // 4
  queue_midi_event(0x80, TEST_NOTE + 17, 0);      // 5
  queue_midi_event(0x80, TEST_NOTE + 20, 0);      // 6
  queue_midi_event(0x80, TEST_NOTE + 24, 0);      // 7
  queue_midi_event(0x80, TEST_NOTE + 29, 0);      // 8
  queue_midi_event(0x80, TEST_NOTE + 32, 0);      // 9
  queue_midi_event(0x80, TEST_NOTE + 37, 0);      // 10
  queue_midi_event(0x80, TEST_NOTE + 40, 0);      // 11
  queue_midi_event(0x80, TEST_NOTE + 46, 0);      // 12
  queue_midi_event(0x80, TEST_NOTE + 49, 0);      // 13
  queue_midi_event(0x80, TEST_NOTE + 52, 0);      // 14
  queue_midi_event(0x80, TEST_NOTE + 57, 0);      // 15
  queue_midi_event(0x80, TEST_NOTE + 60, 0);      // 16

  bool success = load_sysex(DEFAULT_SYSEXBANK, (++_voice_counter) - 1);
  if (success == false)
#ifdef DEBUG
    Serial.println(F("E: Cannot load SYSEX data"));
#endif
  else
    show_patch();
}
#endif
