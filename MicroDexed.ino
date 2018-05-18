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

#define VOLUME 0.5
#define SAMPLE_RATE 44100
//#define INIT_AUDIO_QUEUE 1
//#define SHOW_DEXED_TIMING 1
#define SHOW_XRUN 1
#define SHOW_CPU_LOAD_MSEC 5000
#define MAX_NOTES 10
//#define TEST_MIDI 1
#define TEST_NOTE 40
#define TEST_VEL_MIN 60
#define TEST_VEL_MAX 110
//#define ADD_EFFECT_CHORUS 1
#ifdef ADD_EFFECT_CHORUS
#define AUDIO_MEM 6
#else
#define AUDIO_MEM 2
#endif

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
#ifdef ADD_EFFECT_CHORUS
AudioEffectChorus        chorus1;        //xy=328,295
AudioConnection          patchCord1(queue1, chorus1);
AudioConnection          patchCord2(chorus1, 0, i2s1, 0);
AudioConnection          patchCord3(chorus1, 0, i2s1, 1);
#else
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
#endif
AudioControlSGTL5000     sgtl5000_1;     //xy=507,403
// GUItool: end automatically generated code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLE_RATE);
IntervalTimer sched;
bool sd_card_available = false;

#ifdef ADD_EFFECT_CHORUS
// Number of samples in each delay line
#define CHORUS_DELAY_LENGTH (16*AUDIO_BLOCK_SAMPLES)
// Allocate the delay lines for left and right channels
short delayline[CHORUS_DELAY_LENGTH];
#endif

void setup()
{
  //while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.begin(115200);
  delay(50);
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
  //load_sysex_file("ROM1A.SYX");

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

  dexed->activate();
  dexed->setMaxNotes(MAX_NOTES);

#ifdef ADD_EFFECT_CHORUS
  chorus1.begin(delayline, CHORUS_DELAY_LENGTH, 8);
#endif

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

#ifdef SHOW_CPU_LOAD_MSEC
  sched.begin(cpu_and_mem_usage, SHOW_CPU_LOAD_MSEC * 1000);
#endif
  Serial.print(F("AUDIO_BLOCK_SAMPLES="));
  Serial.println(AUDIO_BLOCK_SAMPLES);
  Serial.println(F("setup end"));
  cpu_and_mem_usage();
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

void load_sysex_file(char *name)
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
            load_sysex(entry, 2);
            entry.close();
            break;
          }
          entry.close();
        }
      }
    }
  }
}

bool load_sysex(File sysex, uint8_t voice_number)
{
  File file;
  char voice_name[11];

  if (file = SD.open(sysex.name()))
  {
    uint8_t* p_data = dexed->data;
    uint8_t i;
    uint8_t tmp;

    file.seek(6 + (voice_number * 128));
    for (i = 0; i < 6; i++)
    {
      file.read(p_data + (i * 17), 11); // R1, R2, R3, R4, L1, L2, L3, L4, LEV SCL BRK PT, SCL LEFT DEPTH, SCL RGHT DEPTH
      tmp = file.read();
      *(p_data + 11 + (i * 21)) = (tmp & 0x0c) >> 2;
      *(p_data + 12 + (i * 21)) = (tmp & 0x3);
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
      file.read(p_data + (i * 21), 1); // FREQ FINE
    }
    file.read(p_data + 125, 8); // PR1, PR2, PR3, PR4, PL1, PL2, PL3, PL4
    tmp = file.read();
    *(p_data + 133) = (tmp & 0x1f);
    tmp = file.read();
    *(p_data + 134) = (tmp & 0x08) >> 3;
    *(p_data + 135) = (tmp & 0x07);
    file.read(p_data + 136, 4); // LFS, LFD, LPMD, LAMD
    tmp = file.read();
    *(p_data + 140) = (tmp & 0x60) >> 5;
    *(p_data + 141) = (tmp & 0x1e) >> 1;
    *(p_data + 142) = (tmp & 0x01);
    file.read(p_data + 143, 1); // TRNSP
    file.read(p_data + 144, 10);
    strncpy(voice_name, (char*)(p_data + 144), 10);
    voice_name[10] = '\0';

    *(p_data + 166) = 1;
    *(p_data + 167) = 1;
    *(p_data + 168) = 1;
    *(p_data + 169) = 1;
    *(p_data + 170) = 1;
    *(p_data + 171) = 1;
    *(p_data + 172) = 16;
  }

  Serial.println(voice_name);
  return (true);
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


