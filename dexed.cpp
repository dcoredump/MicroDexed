/*
   MicroDexed

   MicroDexed is a port of the Dexed sound engine
   (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield.
   Dexed ist heavily based on https://github.com/google/music-synthesizer-for-android

   (c)2018 H. Wirtz <wirtz@parasitstudio.de>

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
#include "config.h"
#include "EngineMkI.h"
#include "EngineOpl.h"
#include "fm_core.h"
#include "exp2.h"
#include "sin.h"
#include "freqlut.h"
#include "controllers.h"
#include <unistd.h>
#include <limits.h>
#ifdef USE_TEENSY_DSP
#include <Audio.h>
#endif

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

  for (i = 0; i < MAX_ACTIVE_NOTES; i++) {
    voices[i].dx7_note = new Dx7Note;
    voices[i].keydown = false;
    voices[i].sustained = false;
    voices[i].live = false;
  }

  max_notes = 16;
  currentNote = 0;
  resetControllers();
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
#ifndef SUM_UP_AS_INT
    float sumbuf[_N_];
#endif

    for (uint8_t j = 0; j < _N_; ++j) {
      audiobuf.get()[j] = 0;
#ifndef SUM_UP_AS_INT
      sumbuf[j] = 0.0;
#else
      buffer[i + j] = 0;
#endif
    }

    int32_t lfovalue = lfo.getsample();
    int32_t lfodelay = lfo.getdelay();
#ifdef SUM_UP_AS_INT
    int32_t sum;
#endif

    for (uint8_t note = 0; note < max_notes; ++note) {
      if (voices[note].live) {
        voices[note].dx7_note->compute(audiobuf.get(), lfovalue, lfodelay, &controllers);
        for (uint8_t j = 0; j < _N_; ++j) {
          int32_t val = audiobuf.get()[j];
          val = val >> 4;
#ifdef USE_TEENSY_DSP
          int32_t clip_val = signed_saturate_rshift(val, 24, 9);
#else
          int32_t clip_val = val < -(1 << 24) ? 0x8000 : val >= (1 << 24) ? 0x7fff : val >> 9;
#endif
#ifdef SUM_UP_AS_INT
          sum = buffer[i + j] + (clip_val >> REDUCE_LOUDNESS);
          if (buffer[i + j] > 0 && clip_val > 0 && sum < 0)
          {
            sum = INT_MAX;
            overload++;
          }
          else if (buffer[i + j] < 0 && clip_val < 0 && sum > 0)
          {
            sum = INT_MIN;
            overload++;
          }
          buffer[i + j] = sum;
          audiobuf.get()[j] = 0;
#else
          float f = static_cast<float>(clip_val >> REDUCE_LOUDNESS) / 0x8000;
          if (f > 1)
          {
            f = 1;
            overload++;
          }
          else if (f < -1)
          {
            f = -1;
            overload++;
          }
          sumbuf[j] += f;
          audiobuf.get()[j] = 0;
#endif
        }
      }
    }
#ifndef SUM_UP_AS_INT
    for (uint8_t j = 0; j < _N_; ++j) {
      buffer[i + j] = static_cast<int16_t>(sumbuf[j] * 0x8000);
    }
#endif
  }
}

bool Dexed::processMidiMessage(uint8_t type, uint8_t data1, uint8_t data2)
{
  switch (type & 0xf0) {
    case 0x80 :
      keyup(data1);
      return (false);
      break;
    case 0x90 :
      keydown(data1, data2);
      return (false);
      break;
    case 0xb0 : {
        uint8_t ctrl = data1;
        uint8_t value = data2;

        switch (ctrl) {
          case 0: // BankSelect MSB
            break;
          case 1:
            controllers.modwheel_cc = value;
            controllers.refresh();
            break;
          case 2:
            controllers.breath_cc = value;
            controllers.refresh();
            break;
          case 4:
            controllers.foot_cc = value;
            controllers.refresh();
            break;
          case 7: // Volume
            vol = float(value) / 0x7f;
            sgtl5000_1.dacVolume(vol * vol_left, vol * vol_right);
            break;
          case 10: // Pan
            if (value < 64)
            {
              vol_left = 1.0;
              vol_right = float(value) / 0x40;
              set_volume(vol, vol_left, vol_right);
            }
            else if (value > 64)
            {
              vol_left = float(0x7f - value) / 0x40;
              vol_right = 1.0;
              set_volume(vol, vol_left, vol_right);
            }
            else
            {
              vol_left = 1.0;
              vol_right = 1.0;
              set_volume(vol, vol_left, vol_right);
            }
            break;
          case 32: // BankSelect LSB
            bank = data2;
            break;
          case 64:
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
            panic();
            return (true);
            break;
          case 121:
            resetControllers();
            return (true);
            break;
          case 123:
            notesOff();
            return (true);
            break;
          case 126:
            setMonoMode(true);
            return (true);
            break;
          case 127:
            setMonoMode(false);
            return (true);
            break;
        }
        break;
      }

    case 0xc0 : // ProgramChange
      load_sysex(bank, data1);
      break;

    // channel aftertouch
    case 0xd0 :
      controllers.aftertouch_cc = data1;
      controllers.refresh();
      break;
    // pitchbend
    case 0xe0 :
      controllers.values_[kControllerPitch] = data1 | (data2 << 7);
      break;

    default:
      break;
  }

  return (false);
}

void Dexed::keydown(uint8_t pitch, uint8_t velo) {
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
}

void Dexed::keyup(uint8_t pitch) {
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

  }
  }
*/

uint8_t Dexed::getEngineType() {
  return engineType;
}

void Dexed::setEngineType(uint8_t tp) {
  if (engineType == tp)
    return;

  switch (tp)  {
    case DEXED_ENGINE_MARKI:
      controllers.core = engineMkI;
      break;
    case DEXED_ENGINE_OPL:
      controllers.core = engineOpl;
      break;
    default:
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

void Dexed::resetControllers(void)
{
  controllers.values_[kControllerPitch] = 0x2000;
  controllers.values_[kControllerPitchRange] = 0;
  controllers.values_[kControllerPitchStep] = 0;
  controllers.modwheel_cc = 0;
  controllers.foot_cc = 0;
  controllers.breath_cc = 0;
  controllers.aftertouch_cc = 0;
  controllers.refresh();
}

void Dexed::notesOff(void) {
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].live == true && voices[i].keydown == true) {
      voices[i].keydown = false;
    }
  }
}

void Dexed::setMaxNotes(uint8_t n) {
  if (n <= MAX_ACTIVE_NOTES)
  {
    notesOff();
    max_notes = n;
    panic();
    controllers.refresh();
  }
}

bool Dexed::loadSysexVoice(uint8_t* new_data)
{
  uint8_t* p_data = data;
  uint8_t op;
  uint8_t tmp;

  //notesOff();

  for (op = 0; op < 6; op++)
  {
    //  DEXED_OP_EG_R1,           // 0
    //  DEXED_OP_EG_R2,           // 1
    //  DEXED_OP_EG_R3,           // 2
    //  DEXED_OP_EG_R4,           // 3
    //  DEXED_OP_EG_L1,           // 4
    //  DEXED_OP_EG_L2,           // 5
    //  DEXED_OP_EG_L3,           // 6
    //  DEXED_OP_EG_L4,           // 7
    //  DEXED_OP_LEV_SCL_BRK_PT,  // 8
    //  DEXED_OP_SCL_LEFT_DEPTH,  // 9
    //  DEXED_OP_SCL_RGHT_DEPTH,  // 10
    memcpy(&data[op * 21], &new_data[op * 17], 11);
    tmp = new_data[(op * 17) + 11];
    *(p_data + DEXED_OP_SCL_LEFT_CURVE + (op * 21)) = (tmp & 0x3);
    *(p_data + DEXED_OP_SCL_RGHT_CURVE + (op * 21)) = (tmp & 0x0c) >> 2;
    tmp = new_data[(op * 17) + 12];
    *(p_data + DEXED_OP_OSC_DETUNE + (op * 21)) = (tmp & 0x78) >> 3;
    *(p_data + DEXED_OP_OSC_RATE_SCALE + (op * 21)) = (tmp & 0x07);
    tmp = new_data[(op * 17) + 13];
    *(p_data + DEXED_OP_KEY_VEL_SENS + (op * 21)) = (tmp & 0x1c) >> 2;
    *(p_data + DEXED_OP_AMP_MOD_SENS + (op * 21)) = (tmp & 0x03);
    *(p_data + DEXED_OP_OUTPUT_LEV + (op * 21)) = new_data[(op * 17) + 14];
    tmp = new_data[(op * 17) + 15];
    *(p_data + DEXED_OP_FREQ_COARSE + (op * 21)) = (tmp & 0x3e) >> 1;
    *(p_data + DEXED_OP_OSC_MODE + (op * 21)) = (tmp & 0x01);
    *(p_data + DEXED_OP_FREQ_FINE + (op * 21)) = new_data[(op * 17) + 16];
  }
  //  DEXED_PITCH_EG_R1,        // 0
  //  DEXED_PITCH_EG_R2,        // 1
  //  DEXED_PITCH_EG_R3,        // 2
  //  DEXED_PITCH_EG_R4,        // 3
  //  DEXED_PITCH_EG_L1,        // 4
  //  DEXED_PITCH_EG_L2,        // 5
  //  DEXED_PITCH_EG_L3,        // 6
  //  DEXED_PITCH_EG_L4,        // 7
  memcpy(&data[DEXED_VOICE_OFFSET], &new_data[102], 8);
  tmp = new_data[110];
  *(p_data + DEXED_VOICE_OFFSET + DEXED_ALGORITHM) = (tmp & 0x1f);
  tmp = new_data[111];
  *(p_data + DEXED_VOICE_OFFSET + DEXED_OSC_KEY_SYNC) = (tmp & 0x08) >> 3;
  *(p_data + DEXED_VOICE_OFFSET + DEXED_FEEDBACK) = (tmp & 0x07);
  //  DEXED_LFO_SPEED,          // 11
  //  DEXED_LFO_DELAY,          // 12
  //  DEXED_LFO_PITCH_MOD_DEP,  // 13
  //  DEXED_LFO_AMP_MOD_DEP,    // 14
  memcpy(&data[DEXED_VOICE_OFFSET + DEXED_LFO_SPEED], &new_data[112], 4);
  tmp = new_data[116];
  *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_PITCH_MOD_SENS) = (tmp & 0x30) >> 4;
  *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_WAVE) = (tmp & 0x0e) >> 1;
  *(p_data + DEXED_VOICE_OFFSET + DEXED_LFO_SYNC) = (tmp & 0x01);
  *(p_data + DEXED_VOICE_OFFSET + DEXED_TRANSPOSE) = new_data[117];
  memcpy(&data[DEXED_VOICE_OFFSET + DEXED_NAME], &new_data[118], 10);
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_RANGE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_STEP) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_RANGE) = 99;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_ASSIGN) = 7;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_RANGE) = 99;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_ASSIGN) = 7;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_RANGE) = 99;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_ASSIGN) = 7;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_RANGE) = 99;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_ASSIGN) = 7;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MASTER_TUNE) = 0;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP1_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP2_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP3_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP4_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP5_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP6_ENABLE) = 1;
  *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MAX_NOTES) = MAX_NOTES;

  controllers.values_[kControllerPitchRange] = data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_RANGE];
  controllers.values_[kControllerPitchStep] = data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_PITCHBEND_STEP];
  controllers.wheel.setRange(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_RANGE]);
  controllers.wheel.setTarget(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MODWHEEL_ASSIGN]);
  controllers.foot.setRange(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_RANGE]);
  controllers.foot.setTarget(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_FOOTCTRL_ASSIGN]);
  controllers.breath.setRange(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_RANGE]);
  controllers.breath.setTarget(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_BREATHCTRL_ASSIGN]);
  controllers.at.setRange(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_RANGE]);
  controllers.at.setTarget(data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_AT_ASSIGN]);
  controllers.masterTune = (data[DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MASTER_TUNE] * 0x4000 << 11) * (1.0 / 12);
  controllers.refresh();

  setOPs((*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP1_ENABLE) << 5) |
         (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP2_ENABLE) << 4) |
         (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP3_ENABLE) << 3) |
         (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP4_ENABLE) << 2) |
         (*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP5_ENABLE) << 1) |
         *(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_OP6_ENABLE ));
  setMaxNotes(*(p_data + DEXED_GLOBAL_PARAMETER_OFFSET + DEXED_MAX_NOTES));
  //panic();
  doRefreshVoice();
  //activate();

#ifdef DEBUG
  char voicename[11];
  memset(voicename, 0, sizeof(voicename));
  strncpy(voicename, (char *)&data[145], sizeof(voicename) - 1);
  Serial.print(F("Voice ["));
  Serial.print(voicename);
  Serial.println(F("] loaded."));
#endif

  return (true);
}
