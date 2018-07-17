# MicroDexed
## Dexed port for Teensy (3.5/3.6 with audio shield)
This is a port of the original Dexed/msfa engine (see https://github.com/asb2m10/dexed and https://github.com/google/music-synthesizer-for-android) to be used on a Teensy-3.5 or Teensy-3.6.

The current state is: work in progress... 

## License
MicroDexed is licensed on the GPL v3. The msfa component (acronym for music synthesizer for android, see https://github.com/google/music-synthesizer-for-android) stays on the Apache 2.0 license to able to collaborate between projects.

## Credits & thanks

* Dexed engine by Pascal Gauthier (asb2m10)
* DX Synth engine (as part of Dexed): Raph Levien and the msfa team
* PPPlay : Great OPL3 implementation, with documented code :D

## Dexed comes with 3 engine types :

* Modern : this is the original 24-bit music-synthesizer-for-android implementation.
* Mark I : Based on the OPL Series but at a higher resolution (LUT are 10-bits). The target of this engine is to be closest to the real DX7.
* OPL Series : this is an experimental implementation of the reversed engineered OPL family chips. 8-bit. Keep in mind that the envelopes stills needs tuning.
