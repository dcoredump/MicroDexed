// DueDexed
//
// $HOME/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/arm-none-eabi/include/sys/unistd.h:
// //int     _EXFUN(link, (const char *__path1, const char *__path2 ));
//

#include <MIDI.h>
#include "dexed.h"

#define RATE 128

Dexed* dexed=new Dexed(RATE);

void setup()
{
  Serial.begin(115200);
  Serial.println("Dexed");
}

void loop()
{
  
}

