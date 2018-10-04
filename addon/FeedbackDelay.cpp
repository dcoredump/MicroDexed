// http://digitalmedia.risd.edu/pbadger/PhysComp/index.php?n=Devices.CombinedFilterAndFeedbackDelayCodeFromTheTutorials
/* Filter combined with Feedback (Echo) Delay from the
    Teensy Audio Library examples. Check the pins you use
    on pots. Inputs to the mixer are on inputs 0 (signal) and 1 (delay line)
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=192,121
AudioFilterStateVariable filter1;        //xy=396,132
AudioMixer4              mixer1;         //xy=584,134
AudioEffectDelay         delay1;         //xy=598,313
AudioOutputI2S           i2s1;           //xy=799,289
AudioConnection          patchCord1(playSdWav1, 0, filter1, 0);
AudioConnection          patchCord2(filter1, 1, mixer1, 0); // filter 1, 1 is bandbass -
// try filter 1, 0 for high pass
// try filter 1, 2 for high pass
AudioConnection          patchCord3(mixer1, delay1);
AudioConnection          patchCord4(delay1, 0, mixer1, 1);
AudioConnection          patchCord5(delay1, 0, i2s1, 0);
AudioConnection          patchCord6(delay1, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=597,539
// GUItool: end automatically generated code



#include <Bounce.h>

Bounce button0 = Bounce(17, 5);
float freq ;
int knob2;

void setup() {
  Serial.begin(57600);   // you may have to set serial monitor to higher speed
  pinMode(0, INPUT_PULLUP);
  AudioMemory(200);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.55);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  mixer1.gain(0, 0.7);
  mixer1.gain(1, 0.7);
  delay1.delay(0, 400);
  filter1.resonance(2.5); // values between 0.7 and 5.0 useful
  delay(1000);
}

void loop() {
  // uncomment for A3 knob to control the feedback level

  if (playSdWav1.isPlaying() == false) {
    Serial.println("Start playing");
    playSdWav1.play("SDTEST1.WAV");
    delay(20); // wait for library to parse WAV info
  }

  int knob = analogRead(16);
  float feedback = (float)knob / 1050.0;
  mixer1.gain(1, feedback);
  Serial.println(feedback);

  // uncomment for pin 0 button to double the feedback (allowing unstable)
  /*
    button0.update();
    if (button0.read() == LOW) {
      mixer1.gain(1, feedback * 2.0);
    }

  */


  // read the knob and adjust the filter frequency
  knob2 = analogRead(A1) + 20;
  // quick and dirty equation for exp scale frequency adjust
  freq =  expf((float)knob2 / 150.0) * 20.0 + 80.0;
  filter1.frequency(freq);
/*  //uncomment for freq debug
  Serial.print(knob2);
  Serial.print("\t");
  Serial.print("freq = ");
  Serial.println(freq);

  delay(5); */
} 
