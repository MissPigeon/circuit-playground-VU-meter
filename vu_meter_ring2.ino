/*
LED VU meter for Circuit Playground
This is a port of the Adafruit Amplitie project to Circuit Playground.
Based on code for the adjustable sensitivity version of amplitie from:
  https://learn.adafruit.com/led-ampli-tie/the-code
 
 Hardware requirements:
 - Circuit Playground
 
 Software requirements:
 - Adafruit Circuit Playground library
 
 Written by Adafruit Industries.  Distributed under the BSD license.
 This paragraph must be included in any redistribution.
 
 fscale function:
 Floating Point Autoscale Function V0.1
 Written by Paul Badger 2007
 Modified from code by Greg Shakar
 
 */
#include <Adafruit_CircuitPlayground.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 9
#define NUMRING 12

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, PIN, NEO_GRB + NEO_KHZ800);

#define MIC_PIN         A4  // Microphone is attached to this analog pin (A4 for circuit playground)
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG       0  // Time of pause before peak dot falls
#define PEAK_FALL        0  // Rate of falling peak dot
#define INPUT_FLOOR     10  // Lower range of analogRead input
#define INPUT_CEILING  500  // Max range of analogRead input, the lower the value the more sensitive (1023 = max)


byte peak = 16; // Peak level of column; used for falling dots
byte peakring = 16; 
unsigned int sample;
byte dotCount = 0;     //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve);

void setup() 
{
  CircuitPlayground.begin();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() 
{
  int numPixels = CircuitPlayground.strip.numPixels();
  int numPixelsring = NUMRING;
  unsigned long startMillis= millis();  // Start of sample window
  float peakToPeak = 0;   // peak-to-peak level

  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned int c, y;
  unsigned int d, x;

  // collect data for length of sample window (in mS)
  while (millis() - startMillis < SAMPLE_WINDOW)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 1024)  // toss out spurious readings
    {
      if (sample > signalMax)
      {
        signalMax = sample;  // save just the max levels
      }
      else if (sample < signalMin)
      {
        signalMin = sample;  // save just the min levels
      }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
 
  // Serial.println(peakToPeak);

  //Fill the strip with rainbow gradient
  for (int i=0;i<=numPixels-1;i++){
    CircuitPlayground.strip.setPixelColor(i,Wheel(map(i,0,numPixels-1,30,150)));
  }
  for (int j=0;j<=numPixelsring-1;j++){
    strip.setPixelColor(j,Wheel(map(j,0,numPixelsring-1,30,150)));
  }

  //Scale the input logarithmically instead of linearly
  c = fscale(INPUT_FLOOR, INPUT_CEILING, numPixels, 0, peakToPeak, 2);
  d = fscale(INPUT_FLOOR, INPUT_CEILING, numPixelsring, 0, peakToPeak, 2);

  // Turn off pixels that are below volume threshold.
  if(c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if(d < peakring) {
    peakring = d;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
 }
  if (c <= numPixels) { // Fill partial column with off pixels
    drawLine(numPixels, numPixels-c, CircuitPlayground.strip.Color(0, 0, 0));
  }
  if (d <= numPixelsring) { // Fill partial column with off pixels
    drawLine(numPixelsring, numPixelsring-d, strip.Color(0, 0, 0));
  }

  // Set the peak dot to match the rainbow gradient
  y = numPixels - peak;
  CircuitPlayground.strip.setPixelColor(y-1,Wheel(map(y,0,numPixels-1,30,150)));
  CircuitPlayground.strip.show();
  x = numPixelsring - peakring;
  strip.setPixelColor(x-1,Wheel(map(x,0,numPixelsring-1,30,150)));
  strip.show();

  // Frame based peak dot animation
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate 
      peak++;
      dotCount = 0;
    }
  } 
  else {
    dotHangCount++; 
  }
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate 
      peakring++;
      dotCount = 0;
    }
  } 
  else {
    dotHangCount++; 
  }
}

//Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    CircuitPlayground.strip.setPixelColor(i, c);
  }
}
void drawLinering(uint8_t fromring, uint8_t toring, uint32_t d) {
  uint8_t fromTempring;
  if (fromring > toring) {
    fromTempring = fromring;
    fromring = toring;
    toring = fromTempring;
  }
  for(int j=fromring; j<=toring; j++){
     strip.setPixelColor(j, d);
  }
}


float fscale( float originalMin, float originalMax, float newBegin, float
newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output 
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  /*
   Serial.println(curve * 100, DEC);   // multply by 100 to preserve resolution  
   Serial.println(); 
   */

  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){ 
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd; 
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine 
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {   
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange); 
  }

  return rangedValue;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return CircuitPlayground.strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return CircuitPlayground.strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return CircuitPlayground.strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}
