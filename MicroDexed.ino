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

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
#include "dexed.h"
#include "dexed_sysex.h"
#include "config.h"
#ifdef USE_ONBOARD_USB_HOST
#include <USBHost_t36.h>
#endif

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=84,294
AudioOutputI2S           i2s1;           //xy=961,276
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=507,403
// GUItool: end automatically generated code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLE_RATE);
bool sd_card_available = false;
uint8_t bank = DEFAULT_SYSEXBANK;
uint32_t xrun = 0;
uint32_t overload = 0;

#ifdef MASTER_KEY_MIDI
bool master_key_enabled = false;
#endif

#ifdef SHOW_CPU_LOAD_MSEC
IntervalTimer sched_show_cpu_usage;
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
  delay(200);
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018 H. Wirtz"));
  Serial.println(F("setup start"));

  // start up USB host
#ifdef USE_ONBOARD_USB_HOST
  usb_host.begin();
#endif

  // start MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // start audio card
  AudioMemory(AUDIO_MEM);
  sgtl5000_1.enable();
  sgtl5000_1.volume(VOLUME);

  // start SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!SD.begin(SDCARD_CS_PIN))
  {
    Serial.println(F("SD card not accessable"));
  }
  else
  {
    Serial.println(F("SD card found."));
    sd_card_available = true;
  }

#ifdef SHOW_CPU_LOAD_MSEC
  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
  sched_show_cpu_usage.begin(show_cpu_and_mem_usage, SHOW_CPU_LOAD_MSEC * 1000);
#endif

  // load default SYSEX data
  load_sysex(bank, DEFAULT_SYSEXSOUND);

#ifdef DEBUG
  show_patch();
#endif

  Serial.print(F("AUDIO_BLOCK_SAMPLES="));
  Serial.println(AUDIO_BLOCK_SAMPLES);

#ifdef TEST_NOTE
  Serial.println(F("MIDI test enabled"));
  sched_note_on.begin(note_on, 2000000);
  sched_note_off.begin(note_off, 6333333);
#endif

  Serial.println(F("setup end"));
  show_cpu_and_mem_usage();

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
  int16_t* audio_buffer; // pointer to 128 * int16_t (=256 bytes!)

  while (42 == 42) // DON'T PANIC!
  {
    handle_midi_input();

    audio_buffer = queue1.getBuffer();
    if (audio_buffer == NULL)
    {
      Serial.println(F("E: audio_buffer allocation problems!"));
    }

    if (!queue1.available())
      continue;

    elapsedMicros t1;
    dexed->getSamples(AUDIO_BLOCK_SAMPLES, audio_buffer);
    uint32_t t2 = t1;
    if (t2 > 2900) // everything greater 2.9ms is a buffer underrun!
      xrun++;
#ifdef SHOW_DEXED_TIMING
    Serial.println(t1, DEC);
#endif
    queue1.playBuffer();
  }
}

void handle_midi_input(void)
{
#ifdef USE_ONBOARD_USB_HOST
  usb_host.Task();
  while (midi_usb.read())
  {
    if (MIDI.getType() == 0xF0) // SysEX
    {
      handle_sysex_parameter(MIDI.getSysExArray(), MIDI.getSysExArrayLength());
    }
    else if (queue_midi_event(midi_usb.getType(), midi_usb.getData1(), midi_usb.getData2()))
      return;
  }
#endif

  while (MIDI.read())
  {
    if (MIDI.getType() == 0xF0) // SYSEX
    {
      handle_sysex_parameter(MIDI.getSysExArray(), MIDI.getSysExArrayLength());
    }
    else
    {
      if (queue_midi_event(MIDI.getType(), MIDI.getData1(), MIDI.getData2()))
        return;
    }
  }
}

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
    Serial.println(F("E: Cannot load SYSEX data"));
  else
    show_patch();
}
#endif

#ifdef SHOW_MIDI_EVENT
void print_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  Serial.print(F("MIDI event type: 0x"));
  if (type < 16)
    Serial.print(F("0"));
  Serial.print(type, HEX);
  Serial.print(F(" data1: "));
  Serial.print(data1, DEC);
  Serial.print(F(" data2: "));
  Serial.println(data2, DEC);
}
#endif

#ifdef MASTER_KEY_MIDI
bool handle_master_key(uint8_t data)
{
  int8_t num = num_key_base_c(data);

  Serial.print(F("Master-Key: "));
  Serial.println(num, DEC);

  if (num > 0)
  {
    // a white key!
    if (num <= 32)
    {
      if (!load_sysex(bank, num))
      {
        Serial.print(F("E: cannot load voice number "));
        Serial.println(num, DEC);
      }
      else
      {
        Serial.print(F("Loading voice number "));
        Serial.println(num, DEC);
      }
    }
    return (true);
  }
  else
  {
    // a black key!
    num = abs(num);
    if (num <= 10)
    {
      sgtl5000_1.volume(num * 0.1);
      Serial.print(F("Volume changed to: "));
      Serial.println(num * 0.1, DEC);
    }
    else if (num > 10 && num <= 20)
    {
      bank = num - 10;
      Serial.print(F("Bank switch to: "));
      Serial.println(bank, DEC);
    }
  }
  return (false);
}
#endif

bool queue_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  bool ret = false;

#ifdef SHOW_MIDI_EVENT
  print_midi_event(type, data1, data2);
#endif

#ifdef MASTER_KEY_MIDI
  if (type == 0x80 && data1 == MASTER_KEY_MIDI) // Master key released
  {
    master_key_enabled = false;
    Serial.println(F("Master key disabled"));
  }
  else if (type == 0x90 && data1 == MASTER_KEY_MIDI) // Master key pressed
  {
    master_key_enabled = true;
    Serial.println(F("Master key enabled"));
  }
  else
  {
    if (master_key_enabled)
    {
      if (type == 0x80) // handle when note is released
        handle_master_key(data1);
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

void handle_sysex_parameter(const uint8_t* sysex, uint8_t len)
{
  // parse parameter change
  if (len == 7)
  {
    if (sysex[1] != 0x43) // check for Yamaha sysex
    {
      Serial.println(F("E: SysEx vendor not Yamaha."));
      return;
    }
    if ((sysex[3] & 0x7c) != 0 || (sysex[3] & 0x7c) != 2)
    {
      Serial.println(F("E: Not a SysEx parameter or function parameter change."));
      return;
    }
    if (sysex[6] != 0xf7)
    {
      Serial.println(F("E: SysEx end status byte not detected."));
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

    Serial.print(F("SysEx"));
    if ((sysex[3] & 0x7c) == 0)
      Serial.print(F(" function"));
    Serial.print(F(" parameter "));
    Serial.print(sysex[4], DEC);
    Serial.print(F("="));
    Serial.println(sysex[5], DEC);
  }
  else
    Serial.println(F("E: SysEx parameter length wrong."));
}

#ifdef SHOW_CPU_LOAD_MSEC
void show_cpu_and_mem_usage(void)
{
  Serial.print(F("CPU:"));
  Serial.print(AudioProcessorUsage(), DEC);
  Serial.print(F("   CPU MAX:"));
  Serial.print(AudioProcessorUsageMax(), DEC);
  Serial.print(F("  MEM:"));
  Serial.print(AudioMemoryUsage(), DEC);
  Serial.print(F("   MEM MAX:"));
  Serial.print(AudioMemoryUsageMax(), DEC);
  Serial.print(F("   XRUN:"));
  Serial.print(xrun, DEC);
  Serial.print(F("   OVERLOAD:"));
  Serial.print(overload, DEC);
  Serial.println();
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
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
    Serial.println(F(":"));
    Serial.println(F("R1|R2|R3|R4|L1|L2|L3|L4 LEV_SCL_BRK_PT|SCL_LEFT_DEPTH|SCL_RGHT_DEPTH"));
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
    Serial.println(F("SCL_L_CURVE|SCL_R_CURVE|RT_SCALE| AMS | KVS |OUT_LEV|OP_MOD|FRQ_C|FRQ_F|DETUNE"));
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
  Serial.println(F("PR1|PR2|PR3|PR4|PL1|PL2|PL3|PL4"));
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
