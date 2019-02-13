/*
   MicroMDAEPiano

   MicroMDAEPiano is a port of the MDA-EPiano sound engine
   (https://sourceforge.net/projects/mda-vst/) for the Teensy-3.5/3.6 with audio shield.

   (c)2019 H. Wirtz <wirtz@parasitstudio.de>

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

#ifndef MIDI_DEVICES_H
#define MIDI_DEVICES_H

#include "config.h"

#if defined(MIDI_DEVICE_USB)
#include <midi_UsbTransport.h>
#endif
#ifdef MIDI_DEVICE_USB_HOST
#include <USBHost_t36.h>
#endif

#ifdef MIDI_DEVICE_DIN
MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_DEVICE_DIN, midi_serial);
#endif
#ifdef MIDI_DEVICE_USB_HOST
USBHost usb_host;
MIDIDevice midi_usb(usb_host);
#endif
#ifdef MIDI_DEVICE_USB
static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;
UsbTransport sUsbTransport;
MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, midi_onboard_usb);
#endif

void handleNoteOn(byte inChannel, byte inNumber, byte inVelocity);
void handleNoteOff(byte inChannel, byte inNumber, byte inVelocity);
void handleControlChange(byte inChannel, byte inData1, byte inData2);
void handleAfterTouch(byte inChannel, byte inPressure);
void handlePitchBend(byte inChannel, int inPitch);
void handleProgramChange(byte inChannel, byte inProgram);
void handleSystemExclusive(byte *data, uint len);
//void handleSystemExclusiveChunk(const byte *data, uint len, bool last);
void handleTimeCodeQuarterFrame(byte data);
void handleAfterTouchPoly(byte inChannel, byte inNumber, byte inVelocity);
void handleSongSelect(byte inSong);
void handleTuneRequest(void);
void handleClock(void);
void handleStart(void);
void handleContinue(void);
void handleStop(void);
void handleActiveSensing(void);
void handleSystemReset(void);
//void handleRealTimeSystem(void);

/*****************************************
   MIDI_DEVICE_DIN
 *****************************************/
#ifdef MIDI_DEVICE_DIN
void handleNoteOn_MIDI_DEVICE_DIN(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOn(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] NoteOn"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleNoteOff_MIDI_DEVICE_DIN(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOff(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] NoteOff"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleControlChange_MIDI_DEVICE_DIN(byte inChannel, byte inData1, byte inData2)
{
  handleControlChange(inChannel, inData1, inData2);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] CC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouch_MIDI_DEVICE_DIN(byte inChannel, byte inPressure)
{
  handleAfterTouch(inChannel, inPressure);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] AT"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handlePitchBend_MIDI_DEVICE_DIN(byte inChannel, int inPitch)
{
  handlePitchBend(inChannel, inPitch);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] PB"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleProgramChange_MIDI_DEVICE_DIN(byte inChannel, byte inProgram)
{
  handleProgramChange(inChannel, inProgram);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] PC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemExclusive_MIDI_DEVICE_DIN(byte *data, uint len)
{
  handleSystemExclusive(data, len);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] SysEx"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handleSystemExclusiveChunk_MIDI_DEVICE_DIN(byte *data, uint len, bool last)
{
  handleSystemExclusiveChunk(data, len, last);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] SysExChunk"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */

void handleTimeCodeQuarterFrame_MIDI_DEVICE_DIN(byte data)
{
  handleTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] TimeCodeQuarterFrame"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendTimeCodeQuarterFrame(0xF1, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouchPoly_MIDI_DEVICE_DIN(byte inChannel, byte inNumber, byte inVelocity)
{
  handleAfterTouchPoly(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] AT-Poly"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSongSelect_MIDI_DEVICE_DIN(byte inSong)
{
  handleSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] SongSelect"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleTuneRequest_MIDI_DEVICE_DIN(void)
{
  handleTuneRequest();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] TuneRequest"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleClock_MIDI_DEVICE_DIN(void)
{
  handleClock();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] Clock"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStart_MIDI_DEVICE_DIN(void)
{
  handleStart();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] Start"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleContinue_MIDI_DEVICE_DIN(void)
{
  handleContinue();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] Continue"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStop_MIDI_DEVICE_DIN(void)
{
  handleStop();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] Stop"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleActiveSensing_MIDI_DEVICE_DIN(void)
{
  handleActiveSensing();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] ActiveSensing"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemReset_MIDI_DEVICE_DIN(void)
{
  handleSystemReset();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] SystemReset"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handlRealTimeSysteme_MIDI_DEVICE_DIN(byte inRealTime)
{
  handleRealTimeSystem();
#ifdef DEBUG
  Serial.print(F("[MIDI_DIN] RealTimeSystem"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(inRealTime);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  //midi_onboard_usb.sendRealTime(inRealTIme);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB[NOTSUPPORTED]"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */
#endif // MIDI_DEVICE_DIN

/*****************************************
   MIDI_DEVICE_USB_HOST
 *****************************************/
#ifdef MIDI_DEVICE_USB_HOST
void handleNoteOn_MIDI_DEVICE_USB_HOST(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOn(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] NoteOn"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleNoteOff_MIDI_DEVICE_USB_HOST(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOff(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] NoteOff"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleControlChange_MIDI_DEVICE_USB_HOST(byte inChannel, byte inData1, byte inData2)
{
  handleControlChange(inChannel, inData1, inData2);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] CC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouch_MIDI_DEVICE_USB_HOST(byte inChannel, byte inPressure)
{
  handleAfterTouch(inChannel, inPressure);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] AT"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handlePitchBend_MIDI_DEVICE_USB_HOST(byte inChannel, int inPitch)
{
  handlePitchBend(inChannel, inPitch);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] PB"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleProgramChange_MIDI_DEVICE_USB_HOST(byte inChannel, byte inProgram)
{
  handleProgramChange(inChannel, inProgram);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] PC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemExclusive_MIDI_DEVICE_USB_HOST(byte *data, uint len)
{
  handleSystemExclusive(data, len);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] SysEx"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handleSystemExclusiveChunk_MIDI_DEVICE_USB_HOST(byte *data, uint len, bool last)
{
  handleSystemExclusiveChunk(data, len, last);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] SysExChunk"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */

void handleTimeCodeQuarterFrame_MIDI_DEVICE_USB_HOST(midi::DataByte data)
{
  handleTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] TimeCodeQuarterFrame"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouchPoly_MIDI_DEVICE_USB_HOST(byte inChannel, byte inNumber, byte inVelocity)
{
  handleAfterTouchPoly(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] AT-Poly"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSongSelect_MIDI_DEVICE_USB_HOST(byte inSong)
{
  handleSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] SongSelect"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleTuneRequest_MIDI_DEVICE_USB_HOST(void)
{
  handleTuneRequest();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] TuneRequest"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleClock_MIDI_DEVICE_USB_HOST(void)
{
  handleClock();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] Clock"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStart_MIDI_DEVICE_USB_HOST(void)
{
  handleStart();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] Start"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleContinue_MIDI_DEVICE_USB_HOST(void)
{
  handleContinue();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] Continue"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStop_MIDI_DEVICE_USB_HOST(void)
{
  handleStop();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] Stop"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleActiveSensing_MIDI_DEVICE_USB_HOST(void)
{
  handleActiveSensing();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] ActiveSensing"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemReset_MIDI_DEVICE_USB_HOST(void)
{
  handleSystemReset();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] SystemReset"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handlRealTimeSysteme_MIDI_DEVICE_USB_HOST(midi::MidiType inRealTime)
{
  handleRealTimeSystem();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB_HOST] RealTimeSystem"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(inRealTime);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.sendRealTime(inRealTime);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */
#endif // MIDI_DEVICE_USB_HOST

/*****************************************
   MIDI_DEVICE_USB
 *****************************************/
#ifdef MIDI_DEVICE_USB
void handleNoteOn_MIDI_DEVICE_USB(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOn(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] NoteOn"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendNoteOn(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleNoteOff_MIDI_DEVICE_USB(byte inChannel, byte inNumber, byte inVelocity)
{
  handleNoteOff(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] NoteOff"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendNoteOff(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleControlChange_MIDI_DEVICE_USB(byte inChannel, byte inData1, byte inData2)
{
  handleControlChange(inChannel, inData1, inData2);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] CC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendControlChange(inData1, inData2, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouch_MIDI_DEVICE_USB(byte inChannel, byte inPressure)
{
  handleAfterTouch(inChannel, inPressure);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] AT"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendAfterTouch(inPressure, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handlePitchBend_MIDI_DEVICE_USB(byte inChannel, int inPitch)
{
  handlePitchBend(inChannel, inPitch);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] PB"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendPitchBend(inPitch, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleProgramChange_MIDI_DEVICE_USB(byte inChannel, byte inProgram)
{
  handleProgramChange(inChannel, inProgram);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] PC"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendProgramChange(inProgram, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemExclusive_MIDI_DEVICE_USB(byte *data, uint len)
{
  handleSystemExclusive(data, len);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] SysEx"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSysEx(len, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handleSystemExclusiveChunk_MIDI_DEVICE_USB(byte *data, uint len, bool last)
{
  handleSystemExclusiveChunk(data, len, last);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] SysExChunk"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSysEx(len, data, last);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */

void handleTimeCodeQuarterFrame_MIDI_DEVICE_USB(midi::DataByte data)
{
  handleTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] TimeCodeQuarterFrame"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendTimeCodeQuarterFrame(data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendTimeCodeQuarterFrame(0xF1, data);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleAfterTouchPoly_MIDI_DEVICE_USB(byte inChannel, byte inNumber, byte inVelocity)
{
  handleAfterTouchPoly(inChannel, inNumber, inVelocity);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] AT-Poly"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendAfterTouch(inNumber, inVelocity, inChannel);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSongSelect_MIDI_DEVICE_USB(byte inSong)
{
  handleSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] SongSelect"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendSongSelect(inSong);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleTuneRequest_MIDI_DEVICE_USB(void)
{
  handleTuneRequest();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] TuneRequest"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendTuneRequest();
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleClock_MIDI_DEVICE_USB(void)
{
  handleClock();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] Clock"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Clock);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStart_MIDI_DEVICE_USB(void)
{
  handleStart();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] Start"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Start);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleContinue_MIDI_DEVICE_USB(void)
{
  handleContinue();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] Continue"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Continue);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleStop_MIDI_DEVICE_USB(void)
{
  handleStop();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] Stop"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::Stop);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleActiveSensing_MIDI_DEVICE_USB(void)
{
  handleActiveSensing();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] ActiveSensing"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::ActiveSensing);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

void handleSystemReset_MIDI_DEVICE_USB(void)
{
  handleSystemReset();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] SystemReset"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(midi::SystemReset);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
}

/* void handleRealTimeSystem_MIDI_DEVICE_USB(byte inRealTime)
{
  handleRealTimeSystem();
#ifdef DEBUG
  Serial.print(F("[MIDI_USB] RealTimeSystem"));
#endif
#ifdef MIDI_MERGE_THRU
#ifdef MIDI_DEVICE_DIN
  midi_serial.sendRealTime((midi::MidiType)inRealTime);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_DIN"));
#endif
#endif
#ifdef MIDI_DEVICE_USB_HOST
  midi_usb.sendRealTime(inRealTime);
#ifdef DEBUG
  Serial.print(F(" THRU->MIDI_USB_HOST"));
#endif
#endif
#ifdef DEBUG
  Serial.println();
#endif
#endif
} */
#endif // MIDI_DEVICE_USB

/*****************************************
   HELPER FUCNTIONS
 *****************************************/
void setup_midi_devices(void)
{
#ifdef MIDI_DEVICE_DIN
  // Start serial MIDI
  midi_serial.begin(DEFAULT_MIDI_CHANNEL);
  midi_serial.setHandleNoteOn(handleNoteOn_MIDI_DEVICE_DIN);
  midi_serial.setHandleNoteOff(handleNoteOff_MIDI_DEVICE_DIN);
  midi_serial.setHandleControlChange(handleControlChange_MIDI_DEVICE_DIN);
  midi_serial.setHandleAfterTouchChannel(handleAfterTouch_MIDI_DEVICE_DIN);
  midi_serial.setHandlePitchBend(handlePitchBend_MIDI_DEVICE_DIN);
  midi_serial.setHandleProgramChange(handleProgramChange_MIDI_DEVICE_DIN);
  midi_serial.setHandleSystemExclusive(handleSystemExclusive_MIDI_DEVICE_DIN);
  //midi_serial.setHandleSystemExclusiveChunk(handleSystemExclusiveChunk_MIDI_DEVICE_DIN);
  midi_serial.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame_MIDI_DEVICE_DIN);
  midi_serial.setHandleAfterTouchPoly(handleAfterTouchPoly_MIDI_DEVICE_DIN);
  midi_serial.setHandleSongSelect(handleSongSelect_MIDI_DEVICE_DIN);
  midi_serial.setHandleTuneRequest(handleTuneRequest_MIDI_DEVICE_DIN);
  midi_serial.setHandleClock(handleClock_MIDI_DEVICE_DIN);
  midi_serial.setHandleStart(handleStart_MIDI_DEVICE_DIN);
  midi_serial.setHandleContinue(handleContinue_MIDI_DEVICE_DIN);
  midi_serial.setHandleStop(handleStop_MIDI_DEVICE_DIN);
  midi_serial.setHandleActiveSensing(handleActiveSensing_MIDI_DEVICE_DIN);
  midi_serial.setHandleSystemReset(handleSystemReset_MIDI_DEVICE_DIN);
  //midi_serial.setHandleRealTimeSystem(handleRealTimeSystem_MIDI_DEVICE_DIN);
  Serial.println(F("MIDI_DEVICE_DIN enabled"));
#endif

  // start up USB host
#ifdef MIDI_DEVICE_USB_HOST
  usb_host.begin();
  midi_usb.setHandleNoteOn(handleNoteOn_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleNoteOff(handleNoteOff_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleControlChange(handleControlChange_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleAfterTouchChannel(handleAfterTouch_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandlePitchChange(handlePitchBend_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleProgramChange(handleProgramChange_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleSystemExclusive(handleSystemExclusive_MIDI_DEVICE_USB_HOST);
  //midi_usb.setHandleSystemExclusiveChunk(handleSystemExclusiveChunk_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleAfterTouchPoly(handleAfterTouchPoly_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleSongSelect(handleSongSelect_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleTuneRequest(handleTuneRequest_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleClock(handleClock_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleStart(handleStart_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleContinue(handleContinue_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleStop(handleStop_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleActiveSensing(handleActiveSensing_MIDI_DEVICE_USB_HOST);
  midi_usb.setHandleSystemReset(handleSystemReset_MIDI_DEVICE_USB_HOST);
  //midi_usb.setHandleRealTimeSystem(handleRealTimeSystem_MIDI_DEVICE_USB_HOST);
  Serial.println(F("MIDI_DEVICE_USB_HOST enabled."));
#endif

  // check for onboard USB-MIDI
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.begin();
  midi_onboard_usb.setHandleNoteOn(handleNoteOn_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleNoteOff(handleNoteOff_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleControlChange(handleControlChange_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleAfterTouchChannel(handleAfterTouch_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandlePitchBend(handlePitchBend_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleProgramChange(handleProgramChange_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleSystemExclusive(handleSystemExclusive_MIDI_DEVICE_USB);
  //midi_onboard_usb.setHandleSystemExclusiveChunk(handleSystemExclusiveChunk_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleAfterTouchPoly(handleAfterTouchPoly_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleSongSelect(handleSongSelect_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleTuneRequest(handleTuneRequest_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleClock(handleClock_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleStart(handleStart_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleContinue(handleContinue_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleStop(handleStop_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleActiveSensing(handleActiveSensing_MIDI_DEVICE_USB);
  midi_onboard_usb.setHandleSystemReset(handleSystemReset_MIDI_DEVICE_USB);
  //midi_onboard_usb.setHandleRealTimeSystem(handleRealTimeSystem_MIDI_DEVICE_USB);
  Serial.println(F("MIDI_DEVICE_USB enabled."));
#endif
}

void check_midi_devices(void)
{
#ifdef MIDI_DEVICE_DIN
  midi_serial.read();
#endif
#ifdef MIDI_DEVICE_USB_HOST
  usb_host.Task();
  midi_usb.read();
#endif
#ifdef MIDI_DEVICE_USB
  midi_onboard_usb.read();
#endif
}
#endif // MIDI_DEVICES_H
