/*
   MicroDexed

   MicroDexed is a port of the Dexed sound engine
   (https://github.com/asb2m10/dexed) for the Teensy-3.5/3.6 with audio shield.
   Dexed ist heavily based on https://github.com/google/music-synthesizer-for-android

   (c)2018,2019 H. Wirtz <wirtz@parasitstudio.de>

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
#include "PluginFx.h"
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
  fx.init(rate);

  engineMkI = new EngineMkI;
  engineOpl = new EngineOpl;
  engineMsfa = new FmCore;

  for (i = 0; i < MAX_ACTIVE_NOTES; i++) {
    voices[i].dx7_note = new Dx7Note;
    voices[i].keydown = false;
    voices[i].sustained = false;
    voices[i].live = false;
  }

  max_notes = MAX_NOTES;
  currentNote = 0;
  resetControllers();
  controllers.masterTune = 0;
  controllers.opSwitch = 0x3f; // enable all operators
  //controllers.opSwitch=0x00;

  lfo.reset(data + 137);

  setMonoMode(false);

  sustain = false;

  setEngineType(DEXED_ENGINE);
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
  float sumbuf[n_samples];

  if (refreshVoice)
  {
    for (i = 0; i < max_notes; i++)
    {
      if ( voices[i].live )
        voices[i].dx7_note->update(data, voices[i].midi_note, voices[i].velocity);
    }

    lfo.reset(data + 137);
    refreshVoice = false;
  }

  for (i = 0; i < n_samples; i += _N_)
  {
    AlignedBuf<int32_t, _N_> audiobuf;

    for (uint8_t j = 0; j < _N_; ++j)
    {
      audiobuf.get()[j] = 0;
      sumbuf[i + j] = 0.0;
    }

    int32_t lfovalue = lfo.getsample();
    int32_t lfodelay = lfo.getdelay();

    for (uint8_t note = 0; note < max_notes; ++note)
    {
      if (voices[note].live)
      {
        voices[note].dx7_note->compute(audiobuf.get(), lfovalue, lfodelay, &controllers);

        for (uint8_t j = 0; j < _N_; ++j)
        {
          int32_t val = audiobuf.get()[j];
          val = val >> 4;
#ifdef USE_TEENSY_DSP
          int32_t clip_val = signed_saturate_rshift(val, 24, 9);
#else
          int32_t clip_val = val < -(1 << 24) ? 0x8000 : val >= (1 << 24) ? 0x7fff : val >> 9;
#endif

          float f = static_cast<float>(clip_val >> REDUCE_LOUDNESS) / 0x8000;
          if (f > 1.0)
          {
            f = 1.0;
            overload++;
          }
          else if (f < -1.0)
          {
            f = -1.0;
            overload++;
          }
          sumbuf[i + j] += f;
          audiobuf.get()[j] = 0;
        }
      }
    }
  }

  fx.process(sumbuf, n_samples);

  for (i = 0; i < n_samples; ++i)
    buffer[i] = static_cast<int16_t>(sumbuf[i] * 0x8000);
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
  pitch += data[144] - TRANSPOSE_FIX;

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

void Dexed::setSustain(bool s)
{
  if (sustain == s)
    return;

  sustain = s;
}

bool Dexed::getSustain(void)
{
  return sustain;
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

uint8_t Dexed::getMaxNotes(void)
{
  return max_notes;
}

uint8_t Dexed::getNumNotesPlaying(void)
{
  uint8_t op_carrier = controllers.core->get_carrier_operators(data[134]); // look for carriers
  uint8_t i;
  uint8_t count_playing_voices = 0;

  for (i = 0; i < max_notes; i++)
  {
    if (voices[i].live == true)
    {
      uint8_t op_amp = 0;
      uint8_t op_carrier_num = 0;

      memset(&voiceStatus, 0, sizeof(VoiceStatus));
      voices[i].dx7_note->peekVoiceStatus(voiceStatus);

      for (uint8_t op = 0; op < 6; op++)
      {
        if ((op_carrier & (1 << op)))
        {
          // this voice is a carrier!
          op_carrier_num++;
          if (voiceStatus.amp[op] <= 1069 && voiceStatus.ampStep[op] == 4)
          {
            // this voice produces no audio output
            op_amp++;
          }
        }
      }

      if (op_amp == op_carrier_num)
      {
        // all carrier-operators are silent -> disable the voice
        voices[i].live = false;
        voices[i].sustained = false;
        voices[i].keydown = false;
#ifdef DEBUG
        Serial.print(F("Shutdown voice: "));
        Serial.println(i, DEC);
#endif
      }
      else
        count_playing_voices++;
    }
  }
  return (count_playing_voices);
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

  strncpy(voice_name, (char *)&data[145], sizeof(voice_name) - 1);

#ifdef DEBUG
  //char voicename[11];
  //memset(voicename, 0, sizeof(voicename));
  //strncpy(voicename, (char *)&data[145], sizeof(voicename) - 1);

  Serial.print(F("Voice ["));
  //Serial.print(voicename);
  Serial.print(voice_name);
  Serial.println(F("] loaded."));
#endif

  return (true);
}
