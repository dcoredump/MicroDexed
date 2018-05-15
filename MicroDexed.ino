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
#include <SerialFlash.h>
#include <QueueArray.h>
#include <MIDI.h>
#include "dexed.h"

#define AUDIO_MEM 32
#define AUDIO_BUFFER_SIZE 128
#define SAMPLEAUDIO_BUFFER_SIZE 44100
#define INIT_AUDIO_QUEUE 1
#define SHOW_DEXED_TIMING 1
#define SHOW_CPU_LOAD
#define TEST_MIDI 1
#define TEST_NOTE 60

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=266,484
AudioOutputI2S           i2s1;           //xy=739,486
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=384,610
// GUItool: end automatically generated code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLEAUDIO_BUFFER_SIZE);
IntervalTimer sched;

void setup()
{
  while (!Serial) ; // wait for Arduino Serial Monitor
  Serial.begin(115200);
  Serial.println(F("MicroDexed based on https://github.com/asb2m10/dexed"));
  Serial.println(F("(c)2018 H. Wirtz"));
  Serial.println(F("setup start"));

  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(AUDIO_MEM);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.2);

  // Initialize processor and memory measurements
#ifdef SHOW_CPU_LOAD
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
      memset(audio_buffer, 0, sizeof(int16_t)*AUDIO_BUFFER_SIZE);
      queue1.playBuffer();
    }
  }
#endif

  dexed->activate();

#ifdef TEST_MIDI
  queue_midi_event(0x90, TEST_NOTE, 100);
  queue_midi_event(0x90, TEST_NOTE + 5, 100);
  queue_midi_event(0x90, TEST_NOTE + 8, 100);
  queue_midi_event(0x90, TEST_NOTE + 12, 100);
  queue_midi_event(0x90, TEST_NOTE + 12, 100);
  queue_midi_event(0x90, TEST_NOTE + 17, 100);
  queue_midi_event(0x90, TEST_NOTE + 20, 100);
  queue_midi_event(0x90, TEST_NOTE + 24, 100);
#endif

#ifdef SHOW_CPU_LOAD
  sched.begin(cpu_and_mem_usage, 1000000);
#endif
  Serial.println(F("setup end"));
}

void loop()
{
  int16_t* audio_buffer; // pointer to 128 * int16_t
  bool break_for_calculation;

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

#ifdef SHOW_DEXED_TIMING
  elapsedMicros t1;
#endif
  dexed->GetSamples(AUDIO_BUFFER_SIZE, audio_buffer);
#ifdef SHOW_DEXED_TIMING
  Serial.println(t1, DEC);
#endif

  queue1.playBuffer();
}

bool queue_midi_event(uint8_t type, uint8_t data1, uint8_t data2)
{
  return (dexed->ProcessMidiMessage(type, data1, data2));
}

#ifdef SHOW_CPU_LOAD
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

