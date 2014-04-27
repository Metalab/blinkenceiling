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
  standardOut,
  obmode,
  obmodeOut
} displayState;
volatile bool stateChanged;
uint8_t rainbowState;

SimpleTimer timer;
int timeout_id;
int obmode_id;
bool movementActive;

bool standardOut_flashState;
int standardOut_flashTimer;

bool obOut_flashState;
int obOut_flashTimer;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Startup!");
#endif
  pinMode(LEDPIN, OUTPUT);
  pinMode(PIRPIN, INPUT_PULLUP);
  pinMode(BUTTONPIN, INPUT_PULLUP);
  stateChanged = true;
  if(digitalRead(PIRPIN)) {
    displayState = standard;
    movementActive = true;
  } else {
    displayState = idle;
    movementActive = false;
  }
  
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
    timeout_id = timer.setTimeout(2*60*100, timeoutCallback);
  } else if(!movementActive) {
    if(displayState == idle || displayState == standardOut) {
      stateChanged = true;
      displayState = standard;
      if(displayState == standardOut) {
        timer.deleteTimer(standardOut_flashTimer);
      }
    }
    timer.deleteTimer(timeout_id);
    movementActive = true;
  }
}

void buttonPressed() {
#ifdef DEBUG
  Serial.println("button pressed");
#endif
  if(displayState == obmode) {
#ifdef DEBUG
    Serial.println("prolong ob timeout");
#endif
    // prolong timer
    timer.restartTimer(obmode_id);
  } else {
    if(displayState == obmodeOut) {
      Serial.println("display state was ob mode out");
      timer.deleteTimer(obOut_flashTimer);
      timer.deleteTimer(obmode_id);
    }
    stateChanged = true;
    displayState = obmode;
    obmode_id = timer.setTimeout(3*60*100, obTimeoutCallback);
#ifdef DEBUG
    Serial.println("initialized ob timeout");
#endif
  }
}

void timeoutCallback() {
  if(displayState == standard) {
    displayState = standardOut;
    stateChanged = true;
    
    standardOut_flashState = false;
    standardOut_flashTimer = timer.setInterval(200, standardOut_flash);
    timeout_id = timer.setTimeout(3000, out_timeoutCallback);
  } else {
    movementActive = false;
  }
}

void standardOut_flash() {
  if(displayState == standardOut) {
    if(!standardOut_flashState) {
      colorWipe(strip.Color(255, 0, 0));
    } else {
      colorWipe(strip.Color(10, 10, 10));
    }
    standardOut_flashState = !standardOut_flashState;
  }
}

void out_timeoutCallback() {
  timer.deleteTimer(standardOut_flashTimer);
  if(displayState == standardOut) {
    displayState = idle;
    stateChanged = true;
  }
  movementActive = false;
}

void obTimeoutCallback() {
#ifdef DEBUG
  Serial.println("ob timeout");
#endif
  displayState = obmodeOut;
  stateChanged = true;
  obmode_id = timer.setTimeout(3000, obOutTimeoutCallback);
  
  obOut_flashState = false;
  obOut_flashTimer = timer.setInterval(200, obOut_flash);
}

void obOutTimeoutCallback() {
#ifdef DEBUG
  Serial.println("ob out timeout");
#endif
  timer.disable(obOut_flashTimer);
  timer.deleteTimer(obOut_flashTimer);
  if(movementActive) {
    displayState = standard;
  } else {
    displayState = idle;
  }
  stateChanged = true;
}

void obOut_flash() {
#ifdef DEBUG
  Serial.println("ob out flash");
#endif
  if(displayState == obmodeOut) {
    if(!obOut_flashState) {
      colorWipe(strip.Color(255, 0, 0));
    } else {
      colorWipe(strip.Color(/*255, 255, 255*/100,100,100));
    }
    obOut_flashState = !obOut_flashState;
  }
}

void loop() {
  timer.run();
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
      colorWipe(strip.Color(/*255, 255, 255*/100,100,100));
    }
  }
  if(displayState == standard) {
      rainbowCycle();
//      delay(20);
  }
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

