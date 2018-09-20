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
#include <limits.h>
#include "dexed.h"
#include "dexed_sysex.h"
#include "config.h"
#ifdef USE_ONBOARD_USB_HOST
#include <USBHost_t36.h>
#endif

#ifdef I2C_DISPLAY // selecting sounds by encoder, button and display
#include <Bounce.h>
#include <Encoder.h>
#endif
#ifdef I2C_DISPLAY
#include <LiquidCrystalPlus_I2C.h>
#endif

#ifdef I2C_DISPLAY
Encoder enc_l(ENC_L_PIN_A, ENC_L_PIN_B);
Bounce but_l = Bounce(BUT_L_PIN, 10);  // 10 ms debounce
Encoder enc_r(ENC_R_PIN_A, ENC_R_PIN_B);
Bounce but_r = Bounce(BUT_R_PIN, 10);  // 10 ms debounce
#endif

#ifdef I2C_DISPLAY
// [I2C] SCL: Pin 19, SDA: Pin 18 (https://www.pjrc.com/teensy/td_libs_Wire.html)
#define LCD_I2C_ADDRESS 0x27
#define LCD_CHARS 16
#define LCD_LINES 2
LiquidCrystalPlus_I2C lcd(LCD_I2C_ADDRESS, LCD_CHARS, LCD_LINES);
#endif

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=494,404
AudioAnalyzePeak         peak1;          //xy=695,491
#ifdef TEENSY_AUDIO_BOARD
AudioOutputI2S           i2s1;           //xy=1072,364
AudioConnection          patchCord1(queue1, peak1);
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=700,536
#else
AudioOutputPT8211        pt8211_1;       //xy=1079,320
AudioAmplifier           volume_master;           //xy=678,393
AudioAmplifier           volume_r;           //xy=818,370
AudioAmplifier           volume_l;           //xy=818,411
AudioConnection          patchCord1(queue1, peak1);
AudioConnection          patchCord2(queue1, volume_master);
AudioConnection          patchCord3(volume_master, volume_r);
AudioConnection          patchCord4(volume_master, volume_l);
AudioConnection          patchCord5(volume_r, 0, pt8211_1, 0);
AudioConnection          patchCord6(volume_l, 0, pt8211_1, 1);
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
uint8_t voice = 0;
float vol = VOLUME;
float vol_right = 1.0;
float vol_left = 1.0;
char bank_name[10];
char voice_name[10];
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

  enc_l.write(INITIAL_ENC_L_VALUE);
  enc_r.write(INITIAL_ENC_R_VALUE);
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
#endif

#ifdef MIDI_DEVICE
  // Start serial MIDI
  midi_serial.begin(DEFAULT_MIDI_CHANNEL);
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
#endif
  set_volume(vol, vol_left, vol_right);

  // start SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!SD.begin(SDCARD_CS_PIN))
  {
    Serial.println(F("SD card not accessable"));
    strcpy(bank_name, "Default");
    strcpy(voice_name, "FM-Piano");
  }
  else
  {
    Serial.println(F("SD card found."));
    sd_card_available = true;

    // load default SYSEX data
    load_sysex(bank, voice);
#ifdef I2C_DISPLAY
    enc_l.write(bank);
    enc_r.write(voice);
#endif
  }

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

  Serial.println(F("<setup end>"));

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
  show_cpu_and_mem_usage();
  cpu_mem_millis = 0;
#endif

#ifdef I2C_DISPLAY
  lcd.show(0, 0, 2, bank);
  lcd.show(0, 2, 1, " ");
  lcd.show(0, 3, 10, bank_name);
  lcd.show(1, 0, 2, voice);
  lcd.show(1, 2, 1, " ");
  lcd.show(1, 3, 10, voice_name);
#endif

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

  while (42 == 42) // DON'T PANIC!
  {
#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
    if (cpu_mem_millis > SHOW_CPU_LOAD_MSEC)
    {
      show_cpu_and_mem_usage();
      cpu_mem_millis = 0;
    }
#endif

    handle_input();

    audio_buffer = queue1.getBuffer();
    if (audio_buffer == NULL)
    {
      Serial.println(F("E: audio_buffer allocation problems!"));
    }

    if (!queue1.available())
      continue;

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
    for (uint8_t i = 0; i <= AUDIO_BLOCK_SAMPLES; i++)
      audio_buffer[i] *= vol;
#endif
    queue1.playBuffer();
  }
}

void handle_input(void)
{
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
    else if (queue_midi_event(midi_usb.getType(), midi_usb.getData1(), midi_usb.getData2()))
      return;
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
    else if (queue_midi_event(midi_serial.getType(), midi_serial.getData1(), midi_serial.getData2()))
      return;
  }
#endif

#ifndef MASTER_KEY_MIDI
  int enc1_val = enc1.read();

  if (but1.update())
    ;

  // place handling of encoder and showing values on lcd here
#endif
}

#ifdef DEBUG
#ifdef SHOW_MIDI_EVENT
void print_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  Serial.print(F("MIDI-Channel: "));
  if (midi_channel == MIDI_CHANNEL_OMNI)
    Serial.print(F("OMNI"));
  else
    Serial.print(midi_channel, DEC);
  Serial.print(F(", MIDI event type: 0x"));
  if (type < 16)
    Serial.print(F("0"));
  Serial.print(type, HEX);
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
        EEPROM.update(EEPROM_OFFSET + EEPROM_VOICE_ADDR, num);
        update_eeprom_checksum();
#ifdef I2C_DISPLAY
        lcd.show(1, 0, 2, voice);
        lcd.show(1, 2, 1, " ");
        lcd.show(1, 3, 10, voice_name);
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
      EEPROM.update(EEPROM_OFFSET + EEPROM_BANK_ADDR, bank);
      update_eeprom_checksum();
#ifdef DEBUG
      Serial.print(F("Bank switch to: "));
      Serial.println(bank, DEC);
#endif
#ifdef I2C_DISPLAY
if(get_bank_name(bank))
{
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
    if (c != midi_channel)
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
      ret = dexed->processMidiMessage(type, data1, data2);

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

  EEPROM.update(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR, uint8_t(vol * UCHAR_MAX));
  EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR, uint8_t(vol_right * UCHAR_MAX));
  EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR, uint8_t(vol_left * UCHAR_MAX));
  update_eeprom_checksum();

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
  sgtl5000_1.dacVolume(vol * vol_left, vol * vol_right);
#else
  volume_master.gain(vol);
  volume_r.gain(vr);
  volume_l.gain(vl);
#endif
}

void handle_sysex_parameter(const uint8_t* sysex, uint8_t len)
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
    EEPROM.update(EEPROM_OFFSET + EEPROM_BANK_ADDR, bank);
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOICE_ADDR, voice);
    EEPROM.update(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR, uint8_t(vol * UCHAR_MAX));
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR, uint8_t(vol_right * UCHAR_MAX));
    EEPROM.update(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR, uint8_t(vol_left * UCHAR_MAX));
    update_eeprom_checksum();
  }
  else
  {
    bank = EEPROM.read(EEPROM_OFFSET + EEPROM_BANK_ADDR);
    voice = EEPROM.read(EEPROM_OFFSET + EEPROM_VOICE_ADDR);
    vol = float(EEPROM.read(EEPROM_OFFSET + EEPROM_MASTER_VOLUME_ADDR)) / UCHAR_MAX;
    vol_right = float(EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_RIGHT_ADDR)) / UCHAR_MAX;
    vol_left = float(EEPROM.read(EEPROM_OFFSET + EEPROM_VOLUME_LEFT_ADDR)) / UCHAR_MAX;
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

#if defined (DEBUG) && defined (SHOW_CPU_LOAD_MSEC)
void show_cpu_and_mem_usage(void)
{
  Serial.print(F("CPU: "));
  Serial.print(AudioProcessorUsage(), DEC);
  Serial.print(F("   CPU MAX: "));
  Serial.print(AudioProcessorUsageMax(), DEC);
  Serial.print(F("  MEM: "));
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
  char voicename[11];

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

