//
// MicroDexed
//
// A port of the Dexed sound engine (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield
// (c)2018 H. Wirtz <wirtz@parasitstudio.de>
//

#include <QueueArray.h>
#include <MIDI.h>
#include "dexed.h"

#define AUDIO_BUFFER_SIZE 128
#define SAMPLEAUDIO_BUFFER_SIZE 44100
#define TEST_MIDI 1
#define TEST_NOTE1 60
#define TEST_NOTE2 68

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <TeensyThreads.h>

typedef struct
{
  uint8_t cmd;
  uint8_t data1;
  uint8_t data2;
} midi_queue_t;

// GUItool: begin automatically geneAUDIO_BUFFER_SIZEd code
AudioPlayQueue           queue1;         //xy=266,484
AudioOutputI2S           i2s1;           //xy=739,486
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=384,610
// GUItool: end automatically geneAUDIO_BUFFER_SIZEd code

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
Dexed* dexed = new Dexed(SAMPLEAUDIO_BUFFER_SIZE);
QueueArray <midi_queue_t> midi_queue;
Threads::Mutex midi_queue_lock;

void setup()
{
  Serial.begin(115200);
  //while (!Serial) ; // wait for Arduino Serial Monitor
  delay(300);
  Serial.println(F("MicroDexed"));
  Serial.println(F("setup start"));

  MIDI.begin(MIDI_CHANNEL_OMNI);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(16);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  // Initialize processor and memory measurements
  //AudioProcessorUsageMaxReset();
  //AudioMemoryUsageMaxReset();

  // initial fill audio buffer
  while (queue1.available())
  {
    int16_t* audio_buffer = queue1.getBuffer();
    if (audio_buffer != NULL)
    {
      memset(audio_buffer, 0, AUDIO_BUFFER_SIZE);
      queue1.playBuffer();
    }
  }

  dexed->activate();

#ifdef TEST_MIDI
  dexed->ProcessMidiMessage(0x90, TEST_NOTE1, 100);
  dexed->ProcessMidiMessage(0x90, TEST_NOTE2, 60);
#endif

  threads.addThread(audio_thread, 1);

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

    while (!midi_queue_lock.try_lock());
    midi_queue.enqueue(m);
    midi_queue_lock.unlock();
  }
}

void audio_thread(void)
{
  int16_t* audio_buffer; // pointer for 128 * int16_t

  Serial.println(F("audio thread start"));

  while (42 == 42)
  {
    audio_buffer = queue1.getBuffer();
    if (audio_buffer == NULL)
    {
      Serial.println("audio_buffer allocation problems!");
      return;
    }
    while (!midi_queue.isEmpty ())
    {
      while (!midi_queue_lock.try_lock());
      midi_queue_t m = midi_queue.dequeue();
      dexed->ProcessMidiMessage(m.cmd, m.data1, m.data2);
      midi_queue_lock.unlock();
    }

    dexed->GetSamples(AUDIO_BUFFER_SIZE, audio_buffer);
    queue1.playBuffer();
  }
}
