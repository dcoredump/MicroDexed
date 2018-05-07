// MicroDexed

#include <MIDI.h>
#include "dexed.h"

#define RATE 128
#define TEENSY 1
#define TEST_MIDI 1

#ifdef TEENSY
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=811,259
AudioOutputI2S           i2s1;           //xy=1185,252
AudioConnection          patchCord1(queue1, 0, i2s1, 0);
AudioConnection          patchCord2(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=830,376
// GUItool: end automatically generated code
#endif

MIDI_CREATE_DEFAULT_INSTANCE();

Dexed* dexed = new Dexed(RATE);

void setup()
{
  Serial.begin(115200);
  //while (!Serial) ; // wait for Arduino Serial Monitor
  delay(200);
  Serial.println(F("MicroDexed"));

  MIDI.begin(MIDI_CHANNEL_OMNI);

#ifdef TEENSY
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(8);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.2);

  // Initialize processor and memory measurements
  //AudioProcessorUsageMaxReset();
  //AudioMemoryUsageMaxReset();

  // initial fill audio buffer
  while (queue1.available())
  {
    queue1.getBuffer();
    queue1.playBuffer();
  }
#endif

  dexed->activate();

#ifdef TEST_MIDI
  dexed->ProcessMidiMessage(0x90, 64, 100);
#endif

  Serial.println(dexed->getEngineType(), DEC);
  Serial.println("Go");
}

void loop()
{
  int16_t* audio_buffer; // pointer for 128 * int16_t

#ifdef TEENSY
  audio_buffer = queue1.getBuffer();
#endif

  // process midi->audio
  if (MIDI.read())
  {
    dexed->ProcessMidiMessage(MIDI.getType(), MIDI.getData1(), MIDI.getData2());
  }
  uint8_t i = 0;
  /*  Serial.println("Before:");
    for (i = 0; i < 128; i++)
    {
      Serial.print(audio_buffer[i]);
      Serial.print(",");
    }
    Serial.println();*/

  dexed->GetSamples(RATE, audio_buffer);

  for (i = 0; i < 128; i++)
  {
    if ((i % 16) == 0)
      Serial.println();

    if (i < 10)
      Serial.print("  ");
    if (i > 9 && i < 100)
      Serial.print(" ");
    Serial.print("[");
    Serial.print(i, DEC);
    Serial.print("]:");
    Serial.print(audio_buffer[i]);
    Serial.print(" ");
  }
  Serial.println();

#ifdef TEENSY
  // play the current buffer
  while (!queue1.available())
    Serial.println("Block");
  queue1.playBuffer();
#endif
  delay(500);
}

