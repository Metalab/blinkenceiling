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

//#define DEBUG

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
  displayState = idle;
  movementActive = false;
  
  rainbowState = 0;

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void pirChange(int val) {
#ifdef DEBUG
  if(val) {
    Serial.println("pir on");
  } else {
    Serial.println("pir off");
  }
#endif
  digitalWrite(LEDPIN, val);
  
  if(val == 0) {
    timeout_id = timer.setTimeout(30*1000, timeoutCallback);
  } else {
    if(displayState == idle || displayState == standardOut) {
      if(displayState == standardOut) {
        timer.deleteTimer(standardOut_flashTimer);
      }
      stateChanged = true;
      displayState = standard;
    }
    if(timeout_id != -1) {
      timer.deleteTimer(timeout_id);
      timeout_id = -1;
    }
    movementActive = true;
#ifdef DEBUG
    Serial.println("movementActive = true");
#endif
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
#ifdef DEBUG
      Serial.println("display state was ob mode out");
#endif
      timer.deleteTimer(obOut_flashTimer);
      timer.deleteTimer(obmode_id);
    } else if(displayState == standardOut) {
#ifdef DEBUG
      Serial.println("display state was standard out");
#endif
      timer.deleteTimer(standardOut_flashTimer);
      timer.deleteTimer(timeout_id);
      timeout_id = -1;
      movementActive = false;
    }
    stateChanged = true;
    displayState = obmode;
    obmode_id = timer.setTimeout(30*1000, obTimeoutCallback);
#ifdef DEBUG
    Serial.println("initialized ob timeout");
#endif
  }
}

void timeoutCallback() {
#ifdef DEBUG
    Serial.println("timeoutCallback");
#endif
  if(displayState == standard) {
    displayState = standardOut;
    stateChanged = true;
    
    standardOut_flashState = false;
    standardOut_flashTimer = timer.setInterval(200, standardOut_flash);
    timeout_id = timer.setTimeout(3013, out_timeoutCallback); // do not use multiples of the interval above, this seems to trigger a bug in the timer library!
  } else {
    movementActive = false;
#ifdef DEBUG
    Serial.println("movementActive = false");
#endif
  }
}

void standardOut_flash() {
#ifdef DEBUG
    Serial.println("standardOut_flash");
#endif
  if(!standardOut_flashState) {
    colorWipe(strip.Color(255, 0, 0));
  } else {
    colorWipe(strip.Color(10, 10, 10));
  }
  standardOut_flashState = !standardOut_flashState;
}

void out_timeoutCallback() {
#ifdef DEBUG
    Serial.println("out_timeoutCallback");
#endif
  timer.deleteTimer(standardOut_flashTimer);
  if(displayState == standardOut) {
    displayState = idle;
    stateChanged = true;
  }
  movementActive = false;
  timeout_id = -1;
#ifdef DEBUG
    Serial.println("movementActive = false");
#endif
}

void obTimeoutCallback() {
#ifdef DEBUG
  Serial.println("ob timeout");
#endif
  displayState = obmodeOut;
  stateChanged = true;
  obmode_id = timer.setTimeout(3013, obOutTimeoutCallback); // do not use multiples of the interval set below, this seems to trigger a bug in the timer library!
  
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
  if(!obOut_flashState) {
    colorWipe(strip.Color(255, 0, 0));
  } else {
    colorWipe(strip.Color(255, 255, 255));
  }
  obOut_flashState = !obOut_flashState;
}

int pirState = -1;

void loop() {
  timer.run();
  if(digitalRead(BUTTONPIN)) {
    buttonPressed();
  }
  int pirPin = digitalRead(PIRPIN);
  if(pirState != pirPin) {
    pirChange(pirPin);
    pirState = pirPin;
  }
  
  if(stateChanged) {
    if(displayState == idle) {
      colorWipe(strip.Color(10, 10, 10));
    } else if(displayState == obmode) {
      colorWipe(strip.Color(255, 255, 255));
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

