// MicroDexed

#include <MIDI.h>
#include "dexed.h"

#define RATE 128

Dexed* dexed=new Dexed(RATE);

void setup()
{
  Serial.begin(115200);
  Serial.println(F("MicroDexed"));
}

void loop()
{
  
}

