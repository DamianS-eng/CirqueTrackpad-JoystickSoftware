// 7MM_Joystick_JoyCon_QTPy_SAMD21_v3A
// 2022-11-03-A
const long buildDate = 2022110301;

//=================================================================================

// IMPORTANT: For the V3 of the JoyCon Style device I am using the QT PY (SAMD21) board.
//            You MUST use the TinyUSB package instead of the usual "joystick.h".
//                TOOLS >> USB STACK >> TinyUSB must be selected (not "arduino").

// Board: Adafruit QT PY (SAMD21)
// Optimize: Faster (-O3)
// USB Stack: TinyUSB  <-- Important: Using TinyUSB, not Arduino USB!
// Debug: Off
// Port: COMxx (Adafruit QT PY (SAMD21))

// This sketch is only valid on boards which have native USB support
// and compatibility with Adafruit TinyUSB library. 
// For example SAMD21, SAMD51, nRF52840.

// 2022-11-03 : Changed button assignment to match Slider (or other press-down sticks)

#include <Adafruit_TinyUSB.h>
#include <Adafruit_NeoPixel.h>

//=================================================================================

const boolean debug = true; // if true, output info to serial monitor.
const boolean graph = false; // if true, output data for plotter/graph viewing.

//=================================================================================

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN_NEOPIXEL);

uint32_t redLow = pixels.Color(64,0,0);
uint32_t redHigh = pixels.Color(255,0,0);
uint32_t greenLow = pixels.Color(0,64,0);
uint32_t greenHigh = pixels.Color(0,255,0);
uint32_t blueLow = pixels.Color(0,0,64);
uint32_t blueHigh = pixels.Color(0,0,255);
uint32_t pinkLow = pixels.Color(64,13,30);
uint32_t pinkHigh = pixels.Color(255,51,119);
uint32_t yellowLow = pixels.Color(64,43,0);
uint32_t yellowHigh = pixels.Color(255,170,0);
uint32_t whiteLow = pixels.Color(64,64,64);
uint32_t whiteHigh = pixels.Color(255,255,255);
uint32_t black = pixels.Color(0,0,0);

//=================================================================================

//Joystick via TinyUSB

uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_GAMEPAD() };

Adafruit_USBD_HID usb_hid(desc_hid_report, sizeof(desc_hid_report), HID_ITF_PROTOCOL_NONE, 2, false);

hid_gamepad_report_t    joystick;

//=================================================================================

// Joystick pins
#define BUTTON_PIN  A1 // Pushbutton of the thumbstick
#define VERT_PIN    A2 // "Y" -- dependent on how thumbstick is oriented
#define HORZ_PIN    A3 // "X" -- dependent on how thumbstick is oriented

// Mapping to XBOX Adaptive Controller
// X and Y will auto map to Axis 0/1 or 2/3 depending on which side you plug
// the joystick into (left=0/1 or right=2/3).

// Joystick variables - Button (with debounce)
bool btnStick = false;
int SEL;
int prevSEL = HIGH;
long lastDebounceTime = 0;
const int debounceDelay = 50;  //millis

//=================================================================================

// values from analog stick
long rawHorz, rawVert;
long mapHorz, mapVert;

// invert if needed
const bool invHorz = false;
const bool invVert = false;

//=================================================================================

// The XBOX Adapative Controller (XAC) likes joystick values from -127 to + 127
// adjust these min/max based on your upstream device. Same values for both X and Y.
const int joyMin = -127;
const int joyMax = +127;

//=================================================================================

// set the default min/max values.
// Determined during testing. These are for JoyCon Switch style joysticks.
// Adjust values based on what you actually get from the stick being used.

int minHorz=90;
int centeredHorz=474;
int maxHorz=866;

int minVert=114;
int centeredVert=484;
int maxVert=820;

//hard limits, to catch out-of-range readings
const int limitMinHorz=50;
const int limitMaxHorz=950;
const int limitMinVert=50;
const int limitMaxVert=950;

//=================================================================================


void setup() {

  if (debug) { 
    Serial.begin(115200);
    Serial.println("7MM_Joystick_JoyCon_QTPy_SAMD21");
  }


  // Set HORZ and VERT and BUTT pins (from joystick) to input
  pinMode(VERT_PIN, INPUT);
  pinMode(HORZ_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Startup the NeoPixel
  pixels.begin(); 

  // Startup TinyUSB for Joystick
  pixels.clear(); pixels.setPixelColor(0, redHigh); pixels.show();
  #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
    // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
    TinyUSB_Device_Init(0);
  #endif
  // IMPORTANT: This will loop until device is mounted.
  //            So, if you see a RED light, it's powered, but not mounted.
  //            When the LED goes yellow, then green, it's mounted.
  usb_hid.begin();
  while( !TinyUSBDevice.mounted() ) delay(10);

  //Sampling centerpoint
  pixels.clear(); pixels.setPixelColor(0,yellowHigh); pixels.show();
  
  // On startup, take some readings from the (hopefully) 
  // centered joystick to determine actual center for X and Y. 
  int iNumberOfSamples = 50;
  long lSampleSumHorz = 0;
  long lSampleSumVert = 0;
  for (int iSample = 1; iSample<=iNumberOfSamples; iSample++) {
    lSampleSumHorz += analogRead(HORZ_PIN); delay(10);
    lSampleSumVert += analogRead(VERT_PIN); delay(10);
  }
  centeredHorz=int(lSampleSumHorz/iNumberOfSamples);
  centeredVert=int(lSampleSumVert/iNumberOfSamples);

  // All ready.
  pixels.clear(); pixels.setPixelColor(0,greenLow); pixels.show();

}//setup


//=================================================================================


void loop() {

  // ----------------------------------------------------

  // Check for button push - and debounce
  int reading = digitalRead(BUTTON_PIN);
  if (reading != prevSEL) {
     lastDebounceTime = millis();// reset timer
  }
  if (millis() - lastDebounceTime > debounceDelay) {
    if (reading != SEL) {
      SEL = reading;
      if (SEL == LOW) {
        btnStick = true;
      }
      else {
        btnStick = false;
      }
    }
  }
  prevSEL = reading;

  // ----------------------------------------------------

  // Read analog values from input pins.
  rawHorz = analogRead(HORZ_PIN); delay(10);
  rawVert = analogRead(VERT_PIN); delay(10);

  // Toss out invalid ranges (out of limit). If out of range, set to centered value.
  //if (rawHorz<limitMinHorz || rawHorz>limitMaxHorz) {rawHorz=centeredHorz;}
  //if (rawVert<limitMinVert || rawVert>limitMaxVert) {rawVert=centeredVert;}
  
  // update the min/max during run, in case we see something outside of the normal
  if (rawHorz<minHorz) {minHorz=rawHorz;}
  if (rawHorz>maxHorz) {maxHorz=rawHorz;}
  if (rawVert<minVert) {minVert=rawVert;}
  if (rawVert>maxVert) {maxVert=rawVert;}

  // Map values to a range the upstread device likes
  mapHorz = map(rawHorz, minHorz, maxHorz, joyMin, joyMax);
  mapVert = map(rawVert, minVert, maxVert, joyMin, joyMax);

  // Clean up center position, the 12 and 12 are based on value seen during testing.
  if (abs(mapHorz)>0 && abs(mapHorz)<12) {mapHorz=0;}
  if (abs(mapVert)>0 && abs(mapVert)<12) {mapVert=0;}
  
  // Invert value if requested (if "up" should go "down" or "left" to "right")
  if (invHorz) {mapHorz = -mapHorz;}
  if (invVert) {mapVert = -mapVert;}

  if ( usb_hid.ready() ) {
      joystick.x = mapHorz;
      joystick.y = mapVert;
      if (btnStick) { joystick.buttons = GAMEPAD_BUTTON_2; } else joystick.buttons = 0;
      usb_hid.sendReport(0, &joystick, sizeof(joystick));
    }

  // ----------------------------------------------------

  if (debug) {serialDebug();}

}//loop

//=================================================================================

void serialDebug() {
  
    if (!graph) {
      Serial.print("H: ");
      Serial.print(minHorz);
      Serial.print(" ");
      Serial.print(centeredHorz);
      Serial.print(" ");
      Serial.print(maxHorz);
      Serial.print(" ");
      Serial.print("V: ");
      Serial.print(minVert);
      Serial.print(" ");
      Serial.print(centeredVert);
      Serial.print(" ");
      Serial.print(maxVert);
      Serial.print(" ");
      Serial.print("Raw H/V: ");
      Serial.print(rawHorz);
      Serial.print(" ");
      Serial.print(rawVert);
      Serial.print(" ");
    }
    if (!graph) {Serial.print("Button: ");}
    //always graph/print button
    Serial.print(digitalRead(BUTTON_PIN));
    Serial.print(" ");
    if (!graph) {Serial.print("Map H/V: ");}
    //always graph/print mapped horz/vert
    Serial.print(mapHorz);
    if (invHorz) { Serial.print("i"); } // i indicates value was inverted 
    Serial.print(" ");
    Serial.print(mapVert);
    if (invVert) { Serial.print("i"); } // i indicates value was inverted
    Serial.print(" ");
    Serial.println();

}//serialDebug
