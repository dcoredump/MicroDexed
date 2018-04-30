// DueDexed
//
// $HOME/.arduino15/packages/arduino/tools/arm-none-eabi-gcc/4.8.3-2014q1/arm-none-eabi/include/sys/unistd.h:
// //int     _EXFUN(link, (const char *__path1, const char *__path2 ));
//
#include "EngineMkI.h"
#include "EngineOpl.h"
#include "fm_core.h"
#include "exp2.h"
#include "sin.h"
#include "freqlut.h"
#include "lfo.h"
#include "pitchenv.h"
#include "env.h"
#include "controllers.h"
#include "PluginFx.h"
#include <unistd.h>
//#include <limits.h>

const uint8_t rate = 128;
PluginFx fx;

void setup()
{
  Tanh::init();
  Sin::init();
  Exp2::init();
  Freqlut::init(rate);
  Lfo::init(rate);
  PitchEnv::init(rate);
  Env::init_sr(rate);
  fx.init(rate);

}

void loop()
{

}

