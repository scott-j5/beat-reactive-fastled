// Global Sample Rate, 5000hz
#define SAMPLEPERIODUS 200

#include <FastLED.h>

#define LED_PIN     7
#define MIC_PIN     5
#define NUM_LEDS    589
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  100

CRGB leds[NUM_LEDS];

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() {
    delay(1000);
    LEDS.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
  
    // Set ADC to 77khz, max for 10bit
    sbi(ADCSRA,ADPS2);
    cbi(ADCSRA,ADPS1);
    cbi(ADCSRA,ADPS0);
}

//Audio signal filtering referenced from Damian Peckett
//https://damian.pecke.tt/2015/03/02/beat-detection-on-the-arduino.html

// 20 - 200hz Single Pole Bandpass IIR Filter
float bassFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2]; 
    xv[2] = sample / 9.1f;
    yv[0] = yv[1]; yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
    return yv[2];
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilter(float sample) { //10hz low pass
    static float xv[2] = {0,0}, yv[2] = {0,0};
    xv[0] = xv[1]; 
    xv[1] = sample / 160.f;
    yv[0] = yv[1]; 
    yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
    return yv[1];
}

// 1.7 - 3.0hz Single Pole Bandpass IIR Filter
float beatFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2]; 
    xv[2] = sample / 7.015f;
    yv[0] = yv[1]; yv[1] = yv[2]; 
    yv[2] = (xv[2] - xv[0])
        + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
    return yv[2];
}

void loop() {
    unsigned long time = micros(); // Used to track rate
    float sample, value, envelope, beat, thresh;
    unsigned char i;
    bool onBeat = false;
    int beatCount = 0;
    int offBeatCount = 0;
    int animation = 3;
    int hue = 0;
    int clr = 0;

    intro(1);

    for(i = 0;;++i){
        // Read ADC and center so +-512
        sample = (float)analogRead(MIC_PIN)-503.f;

        // Filter only bass component
        value = bassFilter(sample);

        // Take signal amplitude and filter
        if(value < 0)value=-value;
        envelope = envelopeFilter(value);

        // Every 120 samples filter the envelope 
        if(i == 120) {
                // Filter out repeating bass sounds 100 - 180bpm
                beat = beatFilter(envelope);

                // Threshold value set based on potentiometer on AN1
                //thresh = 0.02f * (float)analogRead(1);
                
                //If no potentiometer set to appropriate value
                thresh = 1.82f;

                // If we are above threshold, trigger led animations
                if(beat > thresh){
                      //Reset the counter that records cycles since last beat
                      offBeatCount = 0;
                      if (! onBeat){
                        onBeat = true;

                        // Change animation every 16 beats else update color for animation
                        if(beatCount >= 16){
                          beatCount = 0;
                          animation = random(5);
                        }else{
                          hue = random(255);
                          beatCount ++; 
                        }
                      }
                      FastLED.clear();
                      switch (animation){
                        case 0:
                          //Clr value to determine off beat lighting style. 2 = Fade out, 1 = Do nothing, 0 = Instantly clear leds to black
                          clr = 2;
                          fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
                          break;
                        case 1:
                          clr = 0;
                          rainbow(beatCount);
                          break;
                        case 2:
                          clr = 2;
                          alternatingStrobe(hue, beatCount);
                          break;
                        case 3:
                          clr = 0;
                          alternatingLights(hue, beatCount);
                          break;
                        case 4:
                          clr = 0;
                          stepIn(hue, beatCount);
                          break;
                      }
                      FastLED.show();
                }else{
                    onBeat = false;
                    if (! (offBeatCount >= 30)){
                      offBeatCount ++;
                    }
                    if (offBeatCount >= 25){
                      //Animation played after 25 cycles with no beat
                      offBeat1();
                    }else{
                      switch (clr){
                        case 0:
                          break;
                        case 1:
                          FastLED.clear();
                          break;
                        case 2:
                          fadeLightBy (leds, NUM_LEDS, 100);
                          break;
                      }
                      FastLED.show();
                    }
                }
                //Reset sample counter
                i = 0;
        }

        // Consume excess clock cycles, to keep at 5000 hz
        for(unsigned long up = time+SAMPLEPERIODUS; time > 20 && time < up; time = micros());
    }  
}


//INTRO EFFECT ******************************************
void intro(int speed){
  FastLED.clear();

  for(int i=0; i<round(NUM_LEDS/2); i++){
    FastLED.clear();
    for(int j = i; j < i + 3; j ++){
      leds[j].setHSV((i*3 % 255), 255, 255);
    }
    for(int j = NUM_LEDS - i; j > i - 3; j--){
      leds[j].setHSV((i*3 % 255), 255, 255);
    }
    FastLED.show();
    delay(speed);
  }

  for(int i=round(NUM_LEDS/2); i<NUM_LEDS; i++){
    leds[i].setHSV((i*3 % 255), 255, 255);
    leds[NUM_LEDS - i].setHSV((i*3 % 360), 255, 255);
    FastLED.show();
    delay(speed);
  }
}


//Offbeat EFFECT ******************************************
void offBeat1(){
  fadeToBlackBy(leds, NUM_LEDS, 10);
  //changing this variable will increase the chance of a "star" popping up
  addStar(100);
  FastLED.show();
}

//Star effect
void addStar( fract8 chanceOfStar) {
  if( random8() < chanceOfStar) {
    leds[ random16(NUM_LEDS) ].setHSV(random(255), 255, 255);
  }
}


//ALTERNATING STROBE EFFECT ******************************************
void alternatingStrobe(int hue, int beatCount){
    if(beatCount > 8){
        if(beatCount % 2 == 0){
            for(int j=0; j<round(NUM_LEDS/4); j++){
              leds[j].setHSV(hue, 255, 255);
            }
            for(int j=(NUM_LEDS - round(NUM_LEDS/4)); j<NUM_LEDS; j++){
              leds[j].setHSV(hue, 255, 255);
            }
            for(int j=round(NUM_LEDS/4); j<(NUM_LEDS - round(NUM_LEDS/4)); j++){
              leds[j].setRGB(0,0,0);
            }
        }else{
            for(int j=round(NUM_LEDS/4); j<(NUM_LEDS - round(NUM_LEDS/4)); j++){
              leds[j].setHSV(hue, 255, 255);
            }
            for(int j=0; j<round(NUM_LEDS/4); j++){
              leds[j].setRGB(0,0,0);
            }
            for(int j=(NUM_LEDS - round(NUM_LEDS/4)); j<NUM_LEDS; j++){
              leds[j].setRGB(0,0,0);
            }
        }
    }else{
        if(beatCount % 2 == 0){
            for(int j=0; j<round(NUM_LEDS/2); j++){
              leds[j].setHSV(hue, 255, 255);
            }
            for(int j=round(NUM_LEDS/2); j<NUM_LEDS; j++){
              leds[j].setRGB(0,0,0);
            }
        }else{
            for(int j=0; j<round(NUM_LEDS/2); j++){
              leds[j].setRGB(0,0,0);
            }
            for(int j=round(NUM_LEDS/2); j<NUM_LEDS; j++){
              leds[j].setHSV(hue, 255, 255);
            }
        }
    }
    FastLED.show();
}


//Alternating lights *************************************************
void alternatingLights(int hue, int beatCount){
  if(beatCount % 2 == 0){
      for(int j=0; j<NUM_LEDS; j++){
        if(j % 8 == 0){
          leds[j].setHSV(hue, 255, 255);
        }else{
          leds[j].setRGB(0,0,0); 
        }
      }
  }else{
      for(int j=0; j<NUM_LEDS; j++){
        if(j % 8 == 0){
          if (j <= (NUM_LEDS - 8)){
            leds[(j+4)].setHSV(hue, 255, 255);
          }
        }else{
          if (j <= (NUM_LEDS)){
            leds[(j+4)].setRGB(0,0,0);
          }
        }
      }
  }
  FastLED.show();
}


//Ease in effect ******************************************************
void stepIn(int hue, int beatCount){
  int ledCount = (((NUM_LEDS/1.8)/16)*beatCount);
  for (int i=0; i < ledCount; i ++){
    leds[i].setHSV(hue, 255, 255);
  }
  for (int i=NUM_LEDS; i >( NUM_LEDS-ledCount); i --){
    leds[i].setHSV(hue, 255, 255);
  }
  FastLED.show();
}


//Rainbow *************************************************************
void rainbow(int beatCount){
  int startingHue = (beatCount * 34 % 255);
  for(int i=0; i<NUM_LEDS; i++){
    //fadeUsingColor(leds, NUM_LEDS, leds[i].setHSV(((startingHue + (i*3)) % 255), 255, 255));
    leds[i].setHSV(((startingHue + (i*3)) % 255), 255, 255);
  }
  FastLED.show();
}
