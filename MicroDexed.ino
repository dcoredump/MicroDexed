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

#define AUDIO_MEM 2
#define SAMPLE_RATE 44100
//#define INIT_AUDIO_QUEUE 1
//#define SHOW_DEXED_TIMING 1
#define SHOW_XRUN 1
#define SHOW_CPU_LOAD_MSEC 5000
#define MAX_NOTES 10
#define TEST_MIDI 1
#define TEST_NOTE 40
#define TEST_VEL 127
//#define ADD_EFFECT_CHORUS 1

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
Sd2Card card;
SdVolume volume;

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
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018 H. Wirtz"));
  Serial.println(F("setup start"));

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (card.init(SPI_FULL_SPEED, SDCARD_CS_PIN)) {
    Serial.println(F("SD card found."));
  }
  else
  {
    Serial.println(F("No SD card found."));
  }

  switch (card.type())
  {
    case SD_CARD_TYPE_SD1:
    case SD_CARD_TYPE_SD2:
      Serial.println(F("Card type is SD"));
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println(F("Card type is SDHC"));
      break;
    default:
      Serial.println(F("Card is an unknown type (maybe SDXC?)"));
  }
  if (!volume.init(card)) {
    Serial.println(F("Unable to access the filesystem"));
  }

  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(AUDIO_MEM);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.2);

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
  queue_midi_event(0x90, TEST_NOTE, TEST_VEL);            // 1
  queue_midi_event(0x90, TEST_NOTE + 5, TEST_VEL);        // 2
  queue_midi_event(0x90, TEST_NOTE + 8, TEST_VEL);        // 3
  queue_midi_event(0x90, TEST_NOTE + 12, TEST_VEL);       // 4
  queue_midi_event(0x90, TEST_NOTE + 17, TEST_VEL);       // 5
  queue_midi_event(0x90, TEST_NOTE + 20, TEST_VEL);       // 6
  queue_midi_event(0x90, TEST_NOTE + 24, TEST_VEL);       // 7
  queue_midi_event(0x90, TEST_NOTE + 29, TEST_VEL);       // 8
  queue_midi_event(0x90, TEST_NOTE + 32, TEST_VEL);       // 9
  queue_midi_event(0x90, TEST_NOTE + 37, TEST_VEL);       // 10
  queue_midi_event(0x90, TEST_NOTE + 40, TEST_VEL);       // 11
  queue_midi_event(0x90, TEST_NOTE + 44, TEST_VEL);       // 12
  queue_midi_event(0x90, TEST_NOTE + 49, TEST_VEL);       // 13
  queue_midi_event(0x90, TEST_NOTE + 52, TEST_VEL);       // 14
  queue_midi_event(0x90, TEST_NOTE + 57, TEST_VEL);       // 15
  queue_midi_event(0x90, TEST_NOTE + 60, TEST_VEL);       // 16
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

    if(!queue1.available())
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

void spaces(int num) {
  for (int i = 0; i < num; i++) {
    Serial.print(" ");
  }
}
