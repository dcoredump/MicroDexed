/**

   Copyright (c) 2016-2017 Holger Wirtz <dcoredump@googlemail.com>

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

#include "dexed.h"
#include "EngineMkI.h"
#include "EngineOpl.h"
#include "fm_core.h"
#include "exp2.h"
#include "sin.h"
#include "freqlut.h"
#include "controllers.h"
//#include "PluginFx.h"
#include <unistd.h>
#include <limits.h>
#include <math.h>

Dexed::Dexed(uint16_t num_samples)
{
  uint8_t i;

  Exp2::init();
  Tanh::init();
  Sin::init();

  _rate = num_samples;

  Freqlut::init(_rate);
  Lfo::init(_rate);
  PitchEnv::init(_rate);
  Env::init_sr(_rate);
  //fx.init(_rate);

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
  controllers.values_[kControllerPitch] = 0x2000;
  controllers.values_[kControllerPitchRange] = 0;
  controllers.values_[kControllerPitchStep] = 0;
  controllers.modwheel_cc = 0;
  controllers.foot_cc = 0;
  controllers.breath_cc = 0;
  controllers.aftertouch_cc = 0;
  controllers.masterTune = 0;
  controllers.opSwitch = 0x3f; // enable all operators

  lfo.reset(data + 137);

  setMonoMode(false);

  sustain = false;

  memset(&voiceStatus, 0, sizeof(VoiceStatus));

  setEngineType(DEXED_ENGINE_MARKI);
}

Dexed::~Dexed()
{
  currentNote = -1;

  for (uint8_t note = 0; note < MAX_ACTIVE_NOTES; ++note)
  {
    if ( voices[note].dx7_note != NULL )
    {
      delete voices[note].dx7_note;
      voices[note].dx7_note = NULL;
    }
    voices[note].keydown = false;
    voices[note].sustained = false;
    voices[note].live = false;
  }

  if (engineMsfa)
    delete(engineMkI);
  if (engineOpl)
    delete(engineMkI);
  if (engineMkI)
    delete(engineMkI);
}

void Dexed::activate(void)
{
  panic();
  controllers.values_[kControllerPitchRange] = data[155];
  controllers.values_[kControllerPitchStep] = data[156];
}

void Dexed::deactivate(void)
{
  ; // Nothing to do - please reboot ;-)
}

void Dexed::GetSamples(uint32_t n_samples, int16_t* buffer)
{
  uint32_t i;

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

    for (uint32_t j = 0; j < _N_; ++j) {
      audiobuf.get()[j] = 0;
      sumbuf[j] = 0.0;
    }

    int32_t lfovalue = lfo.getsample();
    int32_t lfodelay = lfo.getdelay();

    for (uint8_t note = 0; note < max_notes; ++note) {

      if (voices[note].live) {
        voices[note].dx7_note->compute(audiobuf.get(), lfovalue, lfodelay, &controllers);

        for (uint32_t j = 0; j < _N_; ++j) {

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
    for (uint32_t j = 0; j < _N_; ++j) {
      buffer[j + (i * _N_)] = sumbuf[j];
    }
  }

  // mark unused voice as not live
  if (++_k_rate_counter % 32 && !monoMode)
  {
    uint8_t op_carrier = controllers.core->get_carrier_operators(data[134]); // look for carriers

    for (i = 0; i < max_notes; i++)
    {
      if (voices[i].live == true)
      {
        uint8_t op_amp = 0;
        uint8_t op_carrier_num = 0;

        voices[i].dx7_note->peekVoiceStatus(voiceStatus);

        for (uint8_t op = 0; op < 6; op++)
        {
          uint8_t op_bit = static_cast<uint8_t>(pow(2, op));

          if ((op_carrier & op_bit) > 0)
          {
            // this voice is a carrier!
            op_carrier_num++;

            if (voiceStatus.amp[op] <= 1069 && voiceStatus.ampStep[op] == 4) // this voice produces no audio output
              op_amp++;
          }
        }
        if (op_amp == op_carrier_num)
        {
          // all carrier-operators are silent -> disable the voice
          voices[i].live = false;
          voices[i].sustained = false;
          voices[i].keydown = false;
        }
      }
    }
  }
}

bool Dexed::ProcessMidiMessage(uint8_t cmd, uint8_t data1, uint8_t data2)
{
  switch (cmd & 0xf0) {
    case 0x80 :
      // TRACE("MIDI keyup event: %d",buf[1]);
      keyup(data1);
      return (false);
      break;
    case 0x90 :
      keydown(data1, data2);
      return (false);
      break;
    case 0xb0 : {
        switch (data1) {
          case 1:
            // TRACE("MIDI modwheel event: %d %d",ctrl,value);
            controllers.modwheel_cc = data2;
            controllers.refresh();
            break;
          case 2:
            // TRACE("MIDI breath event: %d %d",ctrl,value);
            controllers.breath_cc = data2;
            controllers.refresh();
            break;
          case 4:
            // TRACE("MIDI footsw event: %d %d",ctrl,value);
            controllers.foot_cc = data2;
            controllers.refresh();
            break;
          case 64:
            // TRACE("MIDI sustain event: %d %d",ctrl,value);
            sustain = data2 > 63;
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
            // TRACE("MIDI all-sound-off: %d %d",ctrl,value);
            panic();
            return (true);
            break;
          case 123:
            // TRACE("MIDI all-notes-off: %d %d",ctrl,value);
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
      voices[note].dx7_note->init(data, pitch, velo);
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

    if ( highNote != -1 ) {
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

/*void Dexed::onParam(uint8_t param_num,float param_val)
  {
  int32_t tune;

  if(param_val!=data_float[param_num])
  {
    _param_change_counter++;

    if(param_num==144 || param_num==134 || param_num==172)
      panic();

    refreshVoice=true;
    data[param_num]=static_cast<uint8_t>(param_val);
    data_float[param_num]=param_val;

    switch(param_num)
    {
      case 155:
        controllers.values_[kControllerPitchRange]=data[param_num];
        break;
      case 156:
        controllers.values_[kControllerPitchStep]=data[param_num];
        break;
      case 157:
	// TRACE("wheel.setRange(%d)",data[param_num]);
        controllers.wheel.setRange(data[param_num]);
        break;
      case 158:
        controllers.wheel.setTarget(data[param_num]);
        break;
      case 159:
        controllers.foot.setRange(data[param_num]);
        break;
      case 160:
        controllers.foot.setTarget(data[param_num]);
        break;
      case 161:
        controllers.breath.setRange(data[param_num]);
        break;
      case 162:
        controllers.breath.setTarget(data[param_num]);
        break;
      case 163:
        controllers.at.setRange(data[param_num]);
        break;
      case 164:
        controllers.at.setTarget(data[param_num]);
        break;
      case 165:
        tune=param_val*0x4000;
        controllers.masterTune=(tune<<11)*(1.0/12);
        break;
      case 166:
      case 167:
      case 168:
      case 169:
      case 170:
      case 171:
        controllers.opSwitch=(data[166]<<5)|(data[167]<<4)|(data[168]<<3)|(data[169]<<2)|(data[170]<<1)|data[171];
        break;
      case 172:
        max_notes=data[param_num];
	break;
    }
  }
  }
*/

uint8_t Dexed::getEngineType() {
  return engineType;
}

void Dexed::setEngineType(uint8_t tp) {
  if (engineType == tp && controllers.core != NULL)
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

void Dexed::notes_off(void) {
  for (uint8_t i = 0; i < MAX_ACTIVE_NOTES; i++) {
    if (voices[i].live == true && voices[i].keydown == true) {
      voices[i].keydown = false;
    }
  }
}

/*
  void Dexed::set_params(void)
  {
  _param_change_counter=0;

  bool polymono=bool(*p(p_polymono));
  uint8_t engine=uint8_t(*p(p_engine));
  float f_gain=*p(p_output);
  float f_cutoff=*p(p_cutoff);
  float f_reso=*p(p_resonance);

  // Dexed-Unisono
  if(isMonoMode()!=polymono)
    setMonoMode(polymono);

  // Dexed-Engine
  if(controllers.core==NULL || getEngineType()!=engine)
  {
    setEngineType(engine);
    refreshVoice=true;
  }

  // Dexed-Filter
  if(fx.uiCutoff!=f_cutoff)
  {
    fx.uiCutoff=f_cutoff;
    refreshVoice=true;
  }
  if(fx.uiReso!=f_reso)
  {
    fx.uiReso=f_reso;
    refreshVoice=true;
  }
  if(fx.uiGain!=f_gain)
  {
    fx.uiGain=f_gain;
    refreshVoice=true;
  }

  // OP6
  onParam(0,*p(p_op6_eg_rate_1));
  onParam(1,*p(p_op6_eg_rate_2));
  onParam(2,*p(p_op6_eg_rate_3));
  onParam(3,*p(p_op6_eg_rate_4));
  onParam(4,*p(p_op6_eg_level_1));
  onParam(5,*p(p_op6_eg_level_2));
  onParam(6,*p(p_op6_eg_level_3));
  onParam(7,*p(p_op6_eg_level_4));
  onParam(8,*p(p_op6_kbd_lev_scl_brk_pt));
  onParam(9,*p(p_op6_kbd_lev_scl_lft_depth));
  onParam(10,*p(p_op6_kbd_lev_scl_rht_depth));
  onParam(11,*p(p_op6_kbd_lev_scl_lft_curve));
  onParam(12,*p(p_op6_kbd_lev_scl_rht_curve));
  onParam(13,*p(p_op6_kbd_rate_scaling));
  onParam(14,*p(p_op6_amp_mod_sensitivity));
  onParam(15,*p(p_op6_key_vel_sensitivity));
  onParam(16,*p(p_op6_operator_output_level));
  onParam(17,*p(p_op6_osc_mode));
  onParam(18,*p(p_op6_osc_freq_coarse));
  onParam(19,*p(p_op6_osc_freq_fine));
  onParam(20,*p(p_op6_osc_detune)+7);
  // OP5
  onParam(21,*p(p_op5_eg_rate_1));
  onParam(22,*p(p_op5_eg_rate_2));
  onParam(23,*p(p_op5_eg_rate_3));
  onParam(24,*p(p_op5_eg_rate_4));
  onParam(25,*p(p_op5_eg_level_1));
  onParam(26,*p(p_op5_eg_level_2));
  onParam(27,*p(p_op5_eg_level_3));
  onParam(28,*p(p_op5_eg_level_4));
  onParam(29,*p(p_op5_kbd_lev_scl_brk_pt));
  onParam(30,*p(p_op5_kbd_lev_scl_lft_depth));
  onParam(31,*p(p_op5_kbd_lev_scl_rht_depth));
  onParam(32,*p(p_op5_kbd_lev_scl_lft_curve));
  onParam(33,*p(p_op5_kbd_lev_scl_rht_curve));
  onParam(34,*p(p_op5_kbd_rate_scaling));
  onParam(35,*p(p_op5_amp_mod_sensitivity));
  onParam(36,*p(p_op5_key_vel_sensitivity));
  onParam(37,*p(p_op5_operator_output_level));
  onParam(38,*p(p_op5_osc_mode));
  onParam(39,*p(p_op5_osc_freq_coarse));
  onParam(40,*p(p_op5_osc_freq_fine));
  onParam(41,*p(p_op5_osc_detune)+7);
  // OP4
  onParam(42,*p(p_op4_eg_rate_1));
  onParam(43,*p(p_op4_eg_rate_2));
  onParam(44,*p(p_op4_eg_rate_3));
  onParam(45,*p(p_op4_eg_rate_4));
  onParam(46,*p(p_op4_eg_level_1));
  onParam(47,*p(p_op4_eg_level_2));
  onParam(48,*p(p_op4_eg_level_3));
  onParam(49,*p(p_op4_eg_level_4));
  onParam(50,*p(p_op4_kbd_lev_scl_brk_pt));
  onParam(51,*p(p_op4_kbd_lev_scl_lft_depth));
  onParam(52,*p(p_op4_kbd_lev_scl_rht_depth));
  onParam(53,*p(p_op4_kbd_lev_scl_lft_curve));
  onParam(54,*p(p_op4_kbd_lev_scl_rht_curve));
  onParam(55,*p(p_op4_kbd_rate_scaling));
  onParam(56,*p(p_op4_amp_mod_sensitivity));
  onParam(57,*p(p_op4_key_vel_sensitivity));
  onParam(58,*p(p_op4_operator_output_level));
  onParam(59,*p(p_op4_osc_mode));
  onParam(60,*p(p_op4_osc_freq_coarse));
  onParam(61,*p(p_op4_osc_freq_fine));
  onParam(62,*p(p_op4_osc_detune)+7);
  // OP3
  onParam(63,*p(p_op3_eg_rate_1));
  onParam(64,*p(p_op3_eg_rate_2));
  onParam(65,*p(p_op3_eg_rate_3));
  onParam(66,*p(p_op3_eg_rate_4));
  onParam(67,*p(p_op3_eg_level_1));
  onParam(68,*p(p_op3_eg_level_2));
  onParam(69,*p(p_op3_eg_level_3));
  onParam(70,*p(p_op3_eg_level_4));
  onParam(71,*p(p_op3_kbd_lev_scl_brk_pt));
  onParam(72,*p(p_op3_kbd_lev_scl_lft_depth));
  onParam(73,*p(p_op3_kbd_lev_scl_rht_depth));
  onParam(74,*p(p_op3_kbd_lev_scl_lft_curve));
  onParam(75,*p(p_op3_kbd_lev_scl_rht_curve));
  onParam(76,*p(p_op3_kbd_rate_scaling));
  onParam(77,*p(p_op3_amp_mod_sensitivity));
  onParam(78,*p(p_op3_key_vel_sensitivity));
  onParam(79,*p(p_op3_operator_output_level));
  onParam(80,*p(p_op3_osc_mode));
  onParam(81,*p(p_op3_osc_freq_coarse));
  onParam(82,*p(p_op3_osc_freq_fine));
  onParam(83,*p(p_op3_osc_detune)+7);
  // OP2
  onParam(84,*p(p_op2_eg_rate_1));
  onParam(85,*p(p_op2_eg_rate_2));
  onParam(86,*p(p_op2_eg_rate_3));
  onParam(87,*p(p_op2_eg_rate_4));
  onParam(88,*p(p_op2_eg_level_1));
  onParam(89,*p(p_op2_eg_level_2));
  onParam(90,*p(p_op2_eg_level_3));
  onParam(91,*p(p_op2_eg_level_4));
  onParam(92,*p(p_op2_kbd_lev_scl_brk_pt));
  onParam(93,*p(p_op2_kbd_lev_scl_lft_depth));
  onParam(94,*p(p_op2_kbd_lev_scl_rht_depth));
  onParam(95,*p(p_op2_kbd_lev_scl_lft_curve));
  onParam(96,*p(p_op2_kbd_lev_scl_rht_curve));
  onParam(97,*p(p_op2_kbd_rate_scaling));
  onParam(98,*p(p_op2_amp_mod_sensitivity));
  onParam(99,*p(p_op2_key_vel_sensitivity));
  onParam(100,*p(p_op2_operator_output_level));
  onParam(101,*p(p_op2_osc_mode));
  onParam(102,*p(p_op2_osc_freq_coarse));
  onParam(103,*p(p_op2_osc_freq_fine));
  onParam(104,*p(p_op2_osc_detune)+7);
  // OP1
  onParam(105,*p(p_op1_eg_rate_1));
  onParam(106,*p(p_op1_eg_rate_2));
  onParam(107,*p(p_op1_eg_rate_3));
  onParam(108,*p(p_op1_eg_rate_4));
  onParam(109,*p(p_op1_eg_level_1));
  onParam(110,*p(p_op1_eg_level_2));
  onParam(111,*p(p_op1_eg_level_3));
  onParam(112,*p(p_op1_eg_level_4));
  onParam(113,*p(p_op1_kbd_lev_scl_brk_pt));
  onParam(114,*p(p_op1_kbd_lev_scl_lft_depth));
  onParam(115,*p(p_op1_kbd_lev_scl_rht_depth));
  onParam(116,*p(p_op1_kbd_lev_scl_lft_curve));
  onParam(117,*p(p_op1_kbd_lev_scl_rht_curve));
  onParam(118,*p(p_op1_kbd_rate_scaling));
  onParam(119,*p(p_op1_amp_mod_sensitivity));
  onParam(120,*p(p_op1_key_vel_sensitivity));
  onParam(121,*p(p_op1_operator_output_level));
  onParam(122,*p(p_op1_osc_mode));
  onParam(123,*p(p_op1_osc_freq_coarse));
  onParam(124,*p(p_op1_osc_freq_fine));
  onParam(125,*p(p_op1_osc_detune)+7);
  // Global for all OPs
  onParam(126,*p(p_pitch_eg_rate_1));
  onParam(127,*p(p_pitch_eg_rate_2));
  onParam(128,*p(p_pitch_eg_rate_3));
  onParam(129,*p(p_pitch_eg_rate_4));
  onParam(130,*p(p_pitch_eg_level_1));
  onParam(131,*p(p_pitch_eg_level_2));
  onParam(132,*p(p_pitch_eg_level_3));
  onParam(133,*p(p_pitch_eg_level_4));
  onParam(134,*p(p_algorithm_num)-1);
  onParam(135,*p(p_feedback));
  onParam(136,*p(p_oscillator_sync));
  onParam(137,*p(p_lfo_speed));
  onParam(138,*p(p_lfo_delay));
  onParam(139,*p(p_lfo_pitch_mod_depth));
  onParam(140,*p(p_lfo_amp_mod_depth));
  onParam(141,*p(p_lfo_sync));
  onParam(142,*p(p_lfo_waveform));
  onParam(143,*p(p_pitch_mod_sensitivity));
  onParam(144,*p(p_transpose));
  // 10 bytes (145-154) are the name of the patch
  // Controllers (added at the end of the data[])
  onParam(155,*p(p_pitch_bend_range));
  onParam(156,*p(p_pitch_bend_step));
  onParam(157,*p(p_mod_wheel_range));
  onParam(158,*p(p_mod_wheel_assign));
  onParam(159,*p(p_foot_ctrl_range));
  onParam(160,*p(p_foot_ctrl_assign));
  onParam(161,*p(p_breath_ctrl_range));
  onParam(162,*p(p_breath_ctrl_assign));
  onParam(163,*p(p_aftertouch_range));
  onParam(164,*p(p_aftertouch_assign));
  onParam(165,*p(p_master_tune));
  onParam(166,*p(p_op1_enable));
  onParam(167,*p(p_op2_enable));
  onParam(168,*p(p_op3_enable));
  onParam(169,*p(p_op4_enable));
  onParam(170,*p(p_op5_enable));
  onParam(171,*p(p_op6_enable));
  onParam(172,*p(p_number_of_voices));

  if(_param_change_counter>PARAM_CHANGE_LEVEL)
  {
    panic();
    controllers.refresh();
  }
  }
*/

