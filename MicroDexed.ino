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
#include <TeensyThreads.h>
#include <QueueArray.h>
#include <MIDI.h>
#include <looper.h>
#include "dexed.h"

#define AUDIO_MEM 8
#define AUDIO_BUFFER_SIZE 128
#define SAMPLEAUDIO_BUFFER_SIZE 44100
#define MIDI_QUEUE_LOCK_TIMEOUT_MS 5
//#define INIT_AUDIO_QUEUE 1

#define TEST_MIDI 1
#define TEST_NOTE1 60
#define TEST_NOTE2 68

typedef struct
{
  uint8_t cmd;
  uint8_t data1;
  uint8_t data2;
} midi_queue_t;

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=266,484
AudioOutputI2S           i2s1;           //xy=739,486
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=384,610
// GUItool: end automatically generated code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLEAUDIO_BUFFER_SIZE);
QueueArray <midi_queue_t> midi_queue;
Threads::Mutex midi_queue_lock;
looper sched;

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
  sgtl5000_1.volume(0.4);

  // Initialize processor and memory measurements
  AudioProcessorUsageMaxReset();
  AudioMemoryUsageMaxReset();

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
  midi_queue_t m;
  m.cmd = 0x90;
  m.data1 = TEST_NOTE1;
  m.data2 = 100;
  midi_queue.enqueue(m);
  m.data1 = TEST_NOTE2;
  midi_queue.enqueue(m);
  m.cmd = 0xb0;
#endif

  threads.addThread(audio_thread, 1);

  sched.addJob(cpu_and_mem_usage, 1000);

  Serial.println(F("setup end"));
}

void loop()
{
#ifdef TEST_MIDI
  if (millis() > 3000 && millis() < 3050)
    dexed->ProcessMidiMessage(0x80, TEST_NOTE1, 0);
  if (millis() > 5000 && millis() < 5050)
    dexed->ProcessMidiMessage(0x80, TEST_NOTE2, 0);
#endif

  // process midi->audio
  while (MIDI.read())
  {
    midi_queue_t m;
    m.cmd = MIDI.getType();
    m.data1 = MIDI.getData1();
    m.data2 = MIDI.getData2();

    if (midi_queue_lock.lock(MIDI_QUEUE_LOCK_TIMEOUT_MS))
    {
      midi_queue.enqueue(m);
      midi_queue_lock.unlock();
    }
  }

  sched.scheduler();
}

void audio_thread(void)
{
  int16_t* audio_buffer; // pointer to 128 * int16_t
  bool break_for_calculation;

  Serial.println(F("audio thread start"));

  while (42 == 42) // Don't panic!
  {
    audio_buffer = queue1.getBuffer();
    if (audio_buffer == NULL)
    {
      Serial.println(F("audio_buffer allocation problems!"));
      continue;
    }
    while (!midi_queue.isEmpty())
    {
      if (midi_queue_lock.lock(MIDI_QUEUE_LOCK_TIMEOUT_MS))
      {
        midi_queue_t m = midi_queue.dequeue();
        break_for_calculation = dexed->ProcessMidiMessage(m.cmd, m.data1, m.data2);
        midi_queue_lock.unlock();
        if (break_for_calculation == true)
          break;
      }
      else
        break;
    }

    dexed->GetSamples(AUDIO_BUFFER_SIZE, audio_buffer);
    queue1.playBuffer();
  }
}

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
}

