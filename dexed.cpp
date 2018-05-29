/**

   Copyright (c) 2016-2018 Holger Wirtz <dcoredump@googlemail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

*/

#include "synth.h"
#include "dexed.h"
#include "EngineMkI.h"
#include "EngineOpl.h"
#include "fm_core.h"
#include "exp2.h"
#include "sin.h"
#include "freqlut.h"
#include "controllers.h"
#include "trace.h"
#include <unistd.h>
#include <limits.h>

Dexed::Dexed(int rate)
{
  uint8_t i;

  Exp2::init();
  Tanh::init();
  Sin::init();

  Freqlut::init(rate);
  Lfo::init(rate);
  PitchEnv::init(rate);
  Env::init_sr(rate);

  engineMkI = new EngineMkI;
  engineOpl = new EngineOpl;
  engineMsfa = new FmCore;

  /*
    if(!(engineMkI=new (std::nothrow) EngineMkI))
    TRACE("Cannot not create engine EngineMkI");
    if(!(engineOpl=new (std::nothrow) EngineOpl))
    {
    delete(engineMkI);
    TRACE("Cannot not create engine EngineOpl");
    }
    if(!(engineMsfa=new (std::nothrow) FmCore))
    {
    delete(engineMkI);
    delete(engineOpl);
    TRACE("Cannot create engine FmCore");
    }
  */

  for (i = 0; i < MAX_ACTIVE_NOTES; i++) {
    voices[i].dx7_note = new Dx7Note;
    voices[i].keydown = false;
    voices[i].sustained = false;
    voices[i].live = false;
  }

  max_notes = 16;
  currentNote = 0;
  controllers.values_[kControllerPitch] = 0x2000;
  controllers.values_[kControllerPitchRange] = 0;
  controllers.values_[kControllerPitchStep] = 0;
  controllers.modwheel_cc = 0;
  controllers.foot_cc = 0;
  controllers.breath_cc = 0;
  controllers.aftertouch_cc = 0;
  controllers.masterTune = 0;
  controllers.opSwitch = 0x3f; // enable all operators
  //controllers.opSwitch=0x00;

  lfo.reset(data + 137);

  setMonoMode(false);

  sustain = false;

  setEngineType(DEXED_ENGINE_MODERN);
}

Dexed::~Dexed()
{
  currentNote = -1;

  for (uint8_t note = 0; note < MAX_ACTIVE_NOTES; note++)
    delete voices[note].dx7_note;

  delete(engineMsfa);
  delete(engineOpl);
  delete(engineMkI);
}

void Dexed::activate(void)
{
  panic();
  controllers.values_[kControllerPitchRange] = data[155];
  controllers.values_[kControllerPitchStep] = data[156];
  controllers.refresh();
}

void Dexed::deactivate(void)
{
  panic();
}

void Dexed::getSamples(uint16_t n_samples, int16_t* buffer)
{
  uint16_t i;

  if (refreshVoice) {
    for (i = 0; i < max_notes; i++) {
      if ( voices[i].live )
        voices[i].dx7_note->update(data, voices[i].midi_note, voices[i].velocity);
    }
    lfo.reset(data + 137);
    refreshVoice = false;
  }

  for (i = 0; i < n_samples; i += _N_) {
    AlignedBuf<int32_t, _N_> audiobuf;
    float sumbuf[_N_];

    for (uint8_t j = 0; j < _N_; ++j) {
      audiobuf.get()[j] = 0;
      sumbuf[j] = 0.0;
    }

    int32_t lfovalue = lfo.getsample();
    int32_t lfodelay = lfo.getdelay();

    for (uint8_t note = 0; note < max_notes; ++note) {
      if (voices[note].live) {
        voices[note].dx7_note->compute(audiobuf.get(), lfovalue, lfodelay, &controllers);
        for (uint8_t j = 0; j < _N_; ++j) {
          int32_t val = audiobuf.get()[j];
          val = val >> 4;
          int32_t clip_val = val < -(1 << 24) ? 0x8000 : val >= (1 << 24) ? 0x7fff : val >> 9;
          float f = static_cast<float>(clip_val >> 1) / 0x8000;
          if (f > 1) f = 1;
          if (f < -1) f = -1;
          sumbuf[j] += f;
          audiobuf.get()[j] = 0;
        }
      }
    }

    for (uint8_t j = 0; j < _N_; ++j) {
      buffer[i + j] = static_cast<int16_t>(sumbuf[j] * 0x8000);
    }
  }
}

bool Dexed::processMidiMessage(uint8_t type, uint8_t data1, uint8_t data2)
{
  switch (type & 0xf0) {
    case 0x80 :
      //TRACE("MIDI keyup event: %d", data1);
      keyup(data1);
      return (false);
      break;
    case 0x90 :
      //TRACE("MIDI keydown event: %d %d", data1, data2);
      keydown(data1, data2);
      return (false);
      break;
    case 0xb0 : {
        uint8_t ctrl = data1;
        uint8_t value = data2;

        switch (ctrl) {
          case 1:
            //TRACE("MIDI modwheel event: %d %d", ctrl, value);
            controllers.modwheel_cc = value;
            controllers.refresh();
            break;
          case 2:
            //TRACE("MIDI breath event: %d %d", ctrl, value);
            controllers.breath_cc = value;
            controllers.refresh();
            break;
          case 4:
            //TRACE("MIDI footsw event: %d %d", ctrl, value);
            controllers.foot_cc = value;
            controllers.refresh();
            break;
          case 64:
            //TRACE("MIDI sustain event: %d %d", ctrl, value);
            sustain = value > 63;
            if (!sustain) {
              for (uint8_t note = 0; note < max_notes; note++) {
                if (voices[note].sustained && !voices[note].keydown) {
                  voices[note].dx7_note->keyup();
                  voices[note].sustained = false;
                }
              }
            }
            break;
          case 120:
            //TRACE("MIDI all-sound-off: %d %d", ctrl, value);
            panic();
            return (true);
            break;
          case 123:
            //TRACE("MIDI all-notes-off: %d %d", ctrl, value);
            notes_off();
            return (true);
            break;
        }
        break;
      }

    //        case 0xc0 :
    //            setCurrentProgram(data1);
    //            break;

    // channel aftertouch
    case 0xd0 :
      //TRACE("MIDI aftertouch 0xd0 event: %d %d", data1);
      controllers.aftertouch_cc = data1;
      controllers.refresh();
      break;
    // pitchbend
    case 0xe0 :
      //TRACE("MIDI pitchbend 0xe0 event: %d %d", data1, data2);
      controllers.values_[kControllerPitch] = data1 | (data2 << 7);
      break;

    default:
      //TRACE("MIDI event unknown: cmd=%d, val1=%d, val2=%d", cmd, data1, data2);
      break;
  }

  TRACE("Bye");
  return (false);
}

void Dexed::keydown(uint8_t pitch, uint8_t velo) {
  TRACE("Hi");
  TRACE("pitch=%d, velo=%d\n", pitch, velo);
  if ( velo == 0 ) {
    keyup(pitch);
    return;
  }

  pitch += data[144] - 24;

  uint8_t note = currentNote;
  uint8_t keydown_counter = 0;

  for (uint8_t i = 0; i < max_notes; i++) {
    if (!voices[note].keydown) {
      currentNote = (note + 1) % max_notes;
      voices[note].midi_note = pitch;
      voices[note].velocity = velo;
      voices[note].sustained = sustain;
      voices[note].keydown = true;
      voices[note].dx7_note->init(data, (int)pitch, (int)velo);
      if ( data[136] )
        voices[note].dx7_note->oscSync();
      break;
    }
    else
      keydown_counter++;

    note = (note + 1) % max_notes;
  }

  if (keydown_counter == 0)
    lfo.keydown();
  if ( monoMode ) {
    for (uint8_t i = 0; i < max_notes; i++) {
      if ( voices[i].live ) {
        // all keys are up, only transfer signal
        if ( ! voices[i].keydown ) {
          voices[i].live = false;
          voices[note].dx7_note->transferSignal(*voices[i].dx7_note);
          break;
        }
        if ( voices[i].midi_note < pitch ) {
          voices[i].live = false;
          voices[note].dx7_note->transferState(*voices[i].dx7_note);
          break;
        }
        return;
      }
    }
  }

  voices[note].live = true;
  TRACE("Bye");
}

void Dexed::keyup(uint8_t pitch) {
  TRACE("Hi");
  TRACE("pitch=%d\n", pitch);

  pitch += data[144] - 24;

  uint8_t note;
  for (note = 0; note < max_notes; ++note) {
    if ( voices[note].midi_note == pitch && voices[note].keydown ) {
      voices[note].keydown = false;
      break;
    }
  }

  // note not found ?
  if ( note >= max_notes ) {
    TRACE("note-off not found???");
    return;
  }

  if ( monoMode ) {
    int8_t highNote = -1;
    int8_t target = 0;
    for (int8_t i = 0; i < max_notes; i++) {
      if ( voices[i].keydown && voices[i].midi_note > highNote ) {
        target = i;
        highNote = voices[i].midi_note;
      }
    }

    if ( highNote != -1 && voices[note].live ) {
      voices[note].live = false;
      voices[target].live = true;
      voices[target].dx7_note->transferState(*voices[note].dx7_note);
    }
  }

  if ( sustain ) {
    voices[note].sustained = true;
  } else {
    voices[note].dx7_note->keyup();
  }
  TRACE("Bye");
}

void Dexed::doRefreshVoice(void)
{
  refreshVoice = true;
}
void Dexed::setOPs(uint8_t ops)
{
  controllers.opSwitch = ops;
}

/*
  void Dexed::onParam(uint8_t param_num, float param_val)
  {
  int32_t tune;

  if (param_val != data_float[param_num])
  {
    TRACE("Parameter %d change from %f to %f", param_num, data_float[param_num], param_val);
  #ifdef DEBUG
    uint8_t tmp = data[param_num];
  #endif

    _param_change_counter++;

    if (param_num == 144 || param_num == 134 || param_num == 172)
      panic();

    refreshVoice = true;
    data[param_num] = static_cast<uint8_t>(param_val);
    data_float[param_num] = param_val;

    switch (param_num)
    {
      case 155:
        controllers.values_[kControllerPitchRange] = data[param_num];
        break;
      case 156:
        controllers.values_[kControllerPitchStep] = data[param_num];
        break;
      case 157:
        controllers.wheel.setRange(data[param_num]);
        controllers.wheel.setTarget(data[param_num + 1]);
        controllers.refresh();
        break;
      case 158:
        controllers.wheel.setRange(data[param_num - 1]);
        controllers.wheel.setTarget(data[param_num]);
        controllers.refresh();
        break;
      case 159:
        controllers.foot.setRange(data[param_num]);
        controllers.foot.setTarget(data[param_num + 1]);
        controllers.refresh();
        break;
      case 160:
        controllers.foot.setRange(data[param_num - 1]);
        controllers.foot.setTarget(data[param_num]);
        controllers.refresh();
        break;
      case 161:
        controllers.breath.setRange(data[param_num]);
        controllers.breath.setTarget(data[param_num + 1]);
        controllers.refresh();
        break;
      case 162:
        controllers.breath.setRange(data[param_num - 1]);
        controllers.breath.setTarget(data[param_num]);
        controllers.refresh();
        break;
      case 163:
        controllers.at.setRange(data[param_num]);
        controllers.at.setTarget(data[param_num + 1]);
        controllers.refresh();
        break;
      case 164:
        controllers.at.setRange(data[param_num - 1]);
        controllers.at.setTarget(data[param_num]);
        controllers.refresh();
        break;
      case 165:
        tune = param_val * 0x4000;
        controllers.masterTune = (tune << 11) * (1.0 / 12);
        break;
      case 166:
      case 167:
      case 168:
      case 169:
      case 170:
      case 171:
        controllers.opSwitch = (data[166] << 5) | (data[167] << 4) | (data[168] << 3) | (data[169] << 2) | (data[170] << 1) | data[171];
        break;
      case 172:
        max_notes = data[param_num];
        break;
    }

    TRACE("Done: Parameter %d changed from %d to %d", param_num, tmp, data[param_num]);
  }
  }
*/

uint8_t Dexed::getEngineType() {
  return engineType;
}

void Dexed::setEngineType(uint8_t tp) {
  TRACE("settings engine %d", tp);

  if (engineType == tp)
    return;

  switch (tp)  {
    case DEXED_ENGINE_MARKI:
      TRACE("DEXED_ENGINE_MARKI:%d", DEXED_ENGINE_MARKI);
      controllers.core = engineMkI;
      break;
    case DEXED_ENGINE_OPL:
      TRACE("DEXED_ENGINE_OPL:%d", DEXED_ENGINE_OPL);
      controllers.core = engineOpl;
      break;
    default:
      TRACE("DEXED_ENGINE_MODERN:%d", DEXED_ENGINE_MODERN);
      controllers.core = engineMsfa;
      tp = DEXED_ENGINE_MODERN;
      break;
  }
  engineType = tp;
  panic();
  controllers.refresh();
}

bool Dexed::isMonoMode(void) {
  return monoMode;
}

void Dexed::setMonoMode(bool mode) {
  if (monoMode == mode)
    return;

  monoMode = mode;
}

void Dexed::panic(void) {
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++)
  {
    if (voices[i].live == true) {
      voices[i].keydown = false;
      voices[i].live = false;
      voices[i].sustained = false;
      if ( voices[i].dx7_note != NULL ) {
        voices[i].dx7_note->oscSync();
      }
    }
  }
}

void Dexed::notes_off(void) {
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].live == true && voices[i].keydown == true) {
      voices[i].keydown = false;
    }
  }
}

void Dexed::setMaxNotes(uint8_t n) {
  if (n <= MAX_ACTIVE_NOTES)
  {
    notes_off();
    max_notes = n;
    panic();
    controllers.refresh();
  }
}
