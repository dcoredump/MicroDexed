//
// MicroDexed
//
// A port of the Dexed sound engine (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <MIDI.h>
#include "dexed.h"

//#define TEST_MIDI 1
#define TEST_NOTE 40
#define TEST_VEL_MIN 60
#define TEST_VEL_MAX 110

//#define DEBUG 1
#define SERIAL_SPEED 38400
#define VOLUME 0.2
#define SAMPLE_RATE 44100
#define DEXED_ENGINE DEXED_ENGINE_MODERN
//#define INIT_AUDIO_QUEUE 1
//#define SHOW_DEXED_TIMING 1
#define SHOW_XRUN 1
#define SHOW_CPU_LOAD_MSEC 5000
#define MAX_NOTES 10
#define AUDIO_MEM 2

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14
// Use these with the Teensy 3.5 & 3.6 SD card
//#define SDCARD_CS_PIN    BUILTIN_SDCARD
//#define SDCARD_MOSI_PIN  11  // not actually used
//#define SDCARD_SCK_PIN   13  // not actually used

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=84,294
AudioOutputI2S           i2s1;           //xy=961,276
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=507,403
// GUItool: end automatically generated code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLE_RATE);
IntervalTimer sched;
bool sd_card_available = false;

void setup()
{
  //while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.begin(SERIAL_SPEED);
  delay(250);
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018 H. Wirtz"));
  Serial.println(F("setup start"));

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!SD.begin(SDCARD_CS_PIN))
  {
    Serial.println(F("SD card not accessable"));
  }
  else
  {
    sd_card_available = true;
  }

  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(AUDIO_MEM);

  sgtl5000_1.enable();
  sgtl5000_1.volume(VOLUME);

  // Initialize processor and memory measurements
#ifdef SHOW_CPU_LOAD_MSEC
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
#endif

#ifdef INIT_AUDIO_QUEUE
  // initial fill audio buffer with empty data
  while (queue1.available())
  {
    int16_t* audio_buffer = queue1.getBuffer();
    if (audio_buffer != NULL)
    {
      memset(audio_buffer, 0, sizeof(int16_t)*AUDIO_BLOCK_SAMPLES);
      queue1.playBuffer();
    }
  }
#endif

  load_sysex("ROM1A.SYX", 5);
#ifdef DEBUG
  show_patch();
#endif
  dexed->activate();
  dexed->setMaxNotes(MAX_NOTES);
  dexed->setEngineType(DEXED_ENGINE);

#ifdef SHOW_CPU_LOAD_MSEC
  sched.begin(cpu_and_mem_usage, SHOW_CPU_LOAD_MSEC * 1000);
#endif
  Serial.print(F("AUDIO_BLOCK_SAMPLES="));
  Serial.println(AUDIO_BLOCK_SAMPLES);
  Serial.println(F("setup end"));
  cpu_and_mem_usage();

#ifdef TEST_MIDI
  delay(200);
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
  queue_midi_event(0x90, TEST_NOTE + 44, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 12
  queue_midi_event(0x90, TEST_NOTE + 49, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 13
  queue_midi_event(0x90, TEST_NOTE + 52, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 14
  queue_midi_event(0x90, TEST_NOTE + 57, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 15
  queue_midi_event(0x90, TEST_NOTE + 60, random(TEST_VEL_MIN, TEST_VEL_MAX));      // 16
  delay(200);
#endif
}

void loop()
{
  int16_t* audio_buffer; // pointer to 128 * int16_t
  bool break_for_calculation;

  while (42 == 42) // DON'T PANIC!
  {
    audio_buffer = queue1.getBuffer();
    if (audio_buffer == NULL)
    {
      Serial.println(F("audio_buffer allocation problems!"));
    }

    while (MIDI.read())
    {
      break_for_calculation = dexed->ProcessMidiMessage(MIDI.getType(), MIDI.getData1(), MIDI.getData2());
      if (break_for_calculation == true)
        break;
    }

    if (!queue1.available())
      continue;

#if defined(SHOW_DEXED_TIMING) || defined(SHOW_XRUN)
    elapsedMicros t1;
#endif
    dexed->GetSamples(AUDIO_BLOCK_SAMPLES, audio_buffer);
#ifdef SHOW_XRUN
    uint32_t t2 = t1;
    if (t2 > 2900)
      Serial.println(F("xrun"));
#endif
#ifdef SHOW_DEXED_TIMING
    Serial.println(t1, DEC);
#endif
    queue1.playBuffer();
  }
}

bool queue_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  return (dexed->ProcessMidiMessage(type, data1, data2));
}

#ifdef SHOW_CPU_LOAD_MSEC
void cpu_and_mem_usage(void)
{
  Serial.print(F("CPU:"));
  Serial.print(AudioProcessorUsage(), DEC);
  Serial.print(F("   CPU MAX:"));
  Serial.print(AudioProcessorUsageMax(), DEC);
  Serial.print(F("  MEM:"));
  Serial.print(AudioMemoryUsage(), DEC);
  Serial.print(F("   MEM MAX:"));
  Serial.print(AudioMemoryUsageMax(), DEC);
  Serial.println();
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();
}
#endif

void load_sysex(char *name, uint8_t voice_number)
{
  File root;

  if (sd_card_available)
  {
    root = SD.open("/");
    while (42 == 42)
    {
      File entry = root.openNextFile();
      if (!entry)
        break;
      else
      {
        if (!entry.isDirectory())
        {
          if (strcmp(name, entry.name()) == 0)
          {
            Serial.println(entry.name());
            check_sysex(entry);
            load_sysex_voice(entry, voice_number);
            entry.close();
            break;
          }
          entry.close();
        }
      }
    }
  }
}

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
    Serial.print(dexed->data[(i * 21) + 0], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 1], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 2], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 3], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 4], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 5], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 6], DEC);
    Serial.print(F(" "));
    Serial.print(dexed->data[(i * 21) + 7], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + 8], DEC);
    Serial.print(F("             "));
    Serial.print(dexed->data[(i * 21) + 9], DEC);
    Serial.print(F("             "));
    Serial.println(dexed->data[(i * 21) + 10], DEC);
    Serial.println(F("SCL_L_CURVE|SCL_R_CURVE|OSC_DET|RT_SCALE|VEL_SENS|MOD_SENS|OUT_LEV|FRQ_C|FRQ_F|MOD"));
    Serial.print(F("      "));
    Serial.print(dexed->data[(i * 21) + 11], DEC);
    Serial.print(F("         "));
    Serial.print(dexed->data[(i * 21) + 12], DEC);
    Serial.print(F("         "));
    Serial.print(dexed->data[(i * 21) + 13], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + 14], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + 15], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + 16], DEC);
    Serial.print(F("        "));
    Serial.print(dexed->data[(i * 21) + 17], DEC);
    Serial.print(F("    "));
    Serial.print(dexed->data[(i * 21) + 18], DEC);
    Serial.print(F("     "));
    Serial.print(dexed->data[(i * 21) + 19], DEC);
    Serial.print(F("     "));
    Serial.println(dexed->data[(i * 21) + 20], DEC);
  }
  Serial.println(F("PR1|PR2|PR3|PR4|PL1|PL2|PL3|PL4"));
  Serial.print(F(" "));
  for (i = 0; i < 8; i++)
  {
    Serial.print(dexed->data[125 + i], DEC);
    Serial.print(F("  "));
  }
  Serial.println();
  Serial.print(F("ALG: "));
  Serial.println(dexed->data[133], DEC);
  Serial.print(F("OSC_SYNC: "));
  Serial.println(dexed->data[134], DEC);
  Serial.print(F("FB: "));
  Serial.println(dexed->data[135], DEC);
  Serial.print(F("LFO SPD: "));
  Serial.println(dexed->data[136], DEC);
  Serial.print(F("LFO_DLY: "));
  Serial.println(dexed->data[137], DEC);
  Serial.print(F("LFO PMD: "));
  Serial.println(dexed->data[138], DEC);
  Serial.print(F("LFO_AMD: "));
  Serial.println(dexed->data[139], DEC);
  Serial.print(F("PMS: "));
  Serial.println(dexed->data[140], DEC);
  Serial.print(F("LFO_WAVEFRM: "));
  Serial.println(dexed->data[141], DEC);
  Serial.print(F("LFO_SYNC: "));
  Serial.println(dexed->data[142], DEC);
  Serial.print(F("TRNSPSE: "));
  Serial.println(dexed->data[143], DEC);
  Serial.print(F("NAME: "));
  strncpy(voicename, (char *)&dexed->data[144], sizeof(voicename) - 1);
  Serial.print(F("["));
  Serial.print(voicename);
  Serial.println(F("]"));
  for (i = 155; i < 173; i++)
  {
    Serial.print(i, DEC);
    Serial.print(F(": "));
    Serial.println(dexed->data[i]);
  }

  Serial.println();
}
#endif

bool load_sysex_voice(File sysex, uint8_t voice_number)
{
  File file;

  if (file = SD.open(sysex.name()))
  {
    uint8_t* p_data = dexed->data;
    uint8_t i;
    uint8_t tmp;

    dexed->notes_off();

    file.seek(6 + (voice_number * 128));
    for (i = 0; i < 6; i++)
    {
      file.read(p_data + (i * 21), 11); // R1, R2, R3, R4, L1, L2, L3, L4, LEV SCL BRK PT, SCL LEFT DEPTH, SCL RGHT DEPTH
      tmp = file.read();
      *(p_data + 11 + (i * 21)) = (tmp & 0x3);
      *(p_data + 12 + (i * 21)) = (tmp & 0x0c) >> 2;
      tmp = file.read();
      *(p_data + 13 + (i * 21)) = (tmp & 0x78) >> 3;
      *(p_data + 14 + (i * 21)) = (tmp & 0x07);
      tmp = file.read();
      *(p_data + 15 + (i * 21)) = (tmp & 0x1c) >> 2;
      *(p_data + 16 + (i * 21)) = (tmp & 0x03);
      *(p_data + 17 + (i * 21)) = file.read();
      tmp = file.read();
      *(p_data + 18 + (i * 21)) = (tmp & 0x3e) >> 1;
      *(p_data + 19 + (i * 21)) = (tmp & 0x01);
      file.read(p_data + 20 + (i * 21), 1); // FREQ FINE
    }
    file.read(p_data + 125, 8); // PR1, PR2, PR3, PR4, PL1, PL2, PL3, PL4
    tmp = file.read();
    *(p_data + 133) = (tmp & 0x1f);
    tmp = file.read();
    *(p_data + 134) = (tmp & 0x08) >> 3;
    *(p_data + 135) = (tmp & 0x07);
    file.read(p_data + 136, 4); // LFS, LFD, LPMD, LAMD
    tmp = file.read();
    *(p_data + 140) = (tmp & 0x30) >> 4;
    *(p_data + 141) = (tmp & 0x0e) >> 1;
    *(p_data + 142) = (tmp & 0x01);
    file.read(p_data + 143, 1); // TRNSP
    file.read(p_data + 144, 10); // TRNSP
    *(p_data + 166) = 1;
    *(p_data + 167) = 1;
    *(p_data + 168) = 1;
    *(p_data + 169) = 1;
    *(p_data + 170) = 1;
    *(p_data + 171) = 1;
    *(p_data + 172) = MAX_NOTES;

    dexed->panic();
    dexed->setOPs((*(p_data + 166) << 5) | (*(p_data + 167) << 4) | (*(p_data + 168) << 3) | (*(p_data + 166) << 2) | (*(p_data + 170) << 1) | *(p_data + 171));
    dexed->setMaxNotes(*(p_data + 172));
    dexed->doRefreshVoice();
    dexed->activate();

    char voicename[11];
    memset(voicename, 0, sizeof(voicename));
    strncpy(voicename, (char *)&dexed->data[144], sizeof(voicename) - 1);
    Serial.print(F("["));
    Serial.print(voicename);
    Serial.println(F("]"));

    return (true);
  }
  else
    return (false);
}

bool check_sysex(File sysex)
{
  File file;
  uint16_t i;
  uint32_t calc_checksum = 0;

  if (sysex.size() != 4104) // check sysex size
    return (false);
  if (file = SD.open(sysex.name()))
  {
    if (file.read() != 0xf0)  // check sysex start-byte
    {
      Serial.println(F("E: SysEx start byte not found."));
      return (false);
    }
    if (file.read() != 0x43)  // check sysex vendor is Yamaha
    {
      Serial.println(F("E: SysEx vendor not Yamaha."));
      return (false);
    }
    file.seek(4103);
    if (file.read() != 0xf7)  // check sysex end-byte
    {
      Serial.println(F("E: SysEx end byte not found."));
      return (false);
    }
    file.seek(3);
    if (file.read() != 0x09)  // check for sysex type (0x09=32 voices)
    {
      Serial.println(F("E: SysEx type not 32 voices."));
      return (false);
    }
    file.seek(6);             // start of 32*128 (=4096) bytes data
    for (i = 0; i < 4096; i++)
      calc_checksum += (file.read() & 0x7F); // calculate checksum
    calc_checksum = uint8_t(~calc_checksum + 1);
    if (calc_checksum != uint8_t(file.read()))
    {
      Serial.println(F("E: checksum mismatch."));
      return (false);
    }
  }
  file.close();
  return (true);
}


