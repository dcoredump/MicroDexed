// MicroDexed

#include <MIDI.h>
#include "dexed.h"

#define RATE 128
#define SAMPLERATE 44100
#define TEENSY 1
#define TEST_MIDI 1
#define TEST_NOTE1 59
#define TEST_NOTE2 60

#ifdef TEENSY
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=266,484
//AudioEffectReverb        reverb1;        //xy=486,545
AudioOutputI2S           i2s1;           //xy=739,486
//AudioConnection          patchCord1(queue1, reverb1);
//AudioConnection          patchCord2(reverb1, 0, i2s1, 0);
//AudioConnection          patchCord3(reverb1, 0, i2s1, 1);
AudioConnection          patchCord2(queue1, 0, i2s1, 0);
AudioConnection          patchCord3(queue1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=384,610
// GUItool: end automatically generated code

/*
  // GUItool: begin automatically generated code
  AudioPlayQueue           queue1;         //xy=811,259
  AudioOutputI2S           i2s1;           //xy=1185,252
  AudioConnection          patchCord1(queue1, 0, i2s1, 0);
  AudioConnection          patchCord2(queue1, 0, i2s1, 1);
  AudioControlSGTL5000     sgtl5000_1;     //xy=830,376
  // GUItool: end automatically generated code
*/

#endif

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

Dexed* dexed = new Dexed(SAMPLERATE);

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
  AudioMemory(16);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);

  // Initialize processor and memory measurements
  //AudioProcessorUsageMaxReset();
  //AudioMemoryUsageMaxReset();

  // initial fill audio buffer
  while (queue1.available())
  {
    int16_t* audio_buffer = queue1.getBuffer();
    if (audio_buffer != NULL)
    {
      memset(audio_buffer,0,RATE);
      queue1.playBuffer();
    }
  }
#endif

  dexed->activate();

#ifdef TEST_MIDI
  dexed->ProcessMidiMessage(0x90, TEST_NOTE1, 100);
  dexed->ProcessMidiMessage(0x90, TEST_NOTE2, 60);
#endif

  //reverb1.reverbTime(5.0);

  Serial.println("Go");
}

void loop()
{
  int16_t* audio_buffer; // pointer for 128 * int16_t

#ifdef TEST_MIDI
  if (millis() > 3000 && millis() < 3050)
    dexed->ProcessMidiMessage(0x80, TEST_NOTE1, 0);
  if (millis() > 5000 && millis() < 5050)
    dexed->ProcessMidiMessage(0x80, TEST_NOTE2, 0);
#endif

#ifdef TEENSY
  audio_buffer = queue1.getBuffer();
  if (audio_buffer == NULL)
  {
    Serial.println("audio_buffer allocation problems!");
    return;
  }
#endif

  // process midi->audio
  if (MIDI.read())
  {
    /* Serial.print("Type: ");
    Serial.print(MIDI.getType(), DEC);
    Serial.print(" Data1: ");
    Serial.print(MIDI.getData1(), DEC);
    Serial.print(" Data2: ");
    Serial.println(MIDI.getData2(), DEC); */
    dexed->ProcessMidiMessage(MIDI.getType(), MIDI.getData1(), MIDI.getData2());
  }
  dexed->GetSamples(RATE, audio_buffer);

  /*   uint8_t i = 0;
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
     Serial.println();*/

#ifdef TEENSY
  queue1.playBuffer();
#endif
}

