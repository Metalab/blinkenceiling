#include <Adafruit_NeoPixel.h>
#include <SimpleTimer.h>

#define PIN A3
#define PIRPIN 2
#define BUTTONPIN 3
#define LEDPIN 13

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(162, PIN, NEO_GRB + NEO_KHZ800);

bool oldMovement;
volatile enum {
  idle = 0,
  standard,
  obmode
} displayState;
volatile bool stateChanged;
uint8_t rainbowState;

SimpleTimer timer;
int timeout_id;
int obmode_id;

void setup() {
  pinMode(LEDPIN, OUTPUT);
  pinMode(PIRPIN, INPUT);
  pinMode(BUTTONPIN, INPUT_PULLUP);
  stateChanged = true;
  if(digitalRead(PIRPIN)) {
    displayState = standard;
  } else {
    displayState = idle;
  }
  
  timeout_id = -1;
  obmode_id = -1;
    
  attachInterrupt(0, pirChange, CHANGE);
  attachInterrupt(1, buttonPressed, RISING);
  
  rainbowState = 0;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void pirChange() {
  int val = digitalRead(PIRPIN);
  digitalWrite(LEDPIN, val);
  
  if(val == 0) {
    timeout_id = timer.setTimeout(2*60*1000, timeoutCallback);
  } else if(timeout_id != -1) {
    if(displayState == idle) {
      stateChanged = true;
      displayState = standard;
    }
    timer.disable(timeout_id);
    timer.deleteTimer(timeout_id);
    timeout_id = -1;
  }
}

void buttonPressed() {
  if(obmode_id != -1) {
    // prolong timer
    timer.restartTimer(obmode_id);
  } else {
    stateChanged = true;
    displayState = obmode;
    obmode_id = timer.setTimeout(3*60*1000, obTimeoutCallback);
  }
}

void timeoutCallback() {
  timeout_id = -1;
  if(displayState == standard) {
    displayState = idle;
    stateChanged = true;
  }
}

void obTimeoutCallback() {
  obmode_id = -1;
  if(timeout_id != -1 || digitalRead(PIRPIN)) {
    displayState = standard;
  } else {
    displayState = idle;
  }
  stateChanged = true;
}

void loop() {
  // do this atomically to avoid interrupt issues
  uint8_t saveSREG = SREG;
  cli();
  volatile bool localStateChanged = stateChanged;
  stateChanged = false;
  SREG = saveSREG;
  
  if(localStateChanged) {
    if(displayState == idle) {
      colorWipe(strip.Color(10, 10, 10));
    } else if(displayState == obmode) {
      colorWipe(strip.Color(200, 200, 200));
    }
  }
  if(displayState == standard) {
      rainbowCycle();
//      delay(20);
  }
  timer.run();
}

void colorWipe(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
  strip.show();
}

void rainbowCycle() {
  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbowState) & 255));
  }
  strip.show();
  
  rainbowState++;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

