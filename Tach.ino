/* Display */
#include <Arduino.h>
#include <TM1637Display.h>

#define CLK 5
#define DIO 6

TM1637Display display(CLK, DIO);

/*
const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
	};
*/


  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };


/*
 * SIMPLE ARDUINO CAR TACHOMETER
 */

const int DATA_PIN            = 4;
const int LED_GROUND_PIN      = 8;

// Vtec solenoid control pin (relay control)
const int VTEC_PIN            = 7;

const int PINS_COUNT          = 4;
const int LED_PINS[PINS_COUNT]        = {9,    10,   11,   12};
const int LED_SWITCH_RPM[PINS_COUNT]  = {1000, 1500, 2000, 2500};
const int REV_LIMITER_RPM             = 5800;
const int VTEC_ON_RPM         = 3500;

const int NUMBER_OF_CYLINDERS = 4;
const int LED_UPDATE_INTERVAL = 200;

/*
 * Last led state update time in ms, used to calculate the time from last update
 */
unsigned long lastUpdateTime  = 0;

/*
 * Amount of spark fires in a single interval
 */
volatile int sparkFireCount            = 0;

/*
 * Rpm value from last update
 * Used to average the last 2 rpms for smoother output
 */
int lastRpmValue              = 0;

/*
 * Blinking rev limiter state
 */
bool revLimiterOn             = ;

/*
 *
 */
void incrementRpmCount () {
  sparkFireCount++;
}

/*
 * Turns all leds on or off
*/
void setGlobalState(bool state) {
  for (int i = 0; i < PINS_COUNT; i++) {
    digitalWrite(LED_PINS[i], state);
  }
}

/*
 * Turn on leds, based on input rpm
*/
void setLedState(int rpm) {
  setGlobalState(LOW);

  // If rpm is over REV_LIMITER_RPM, all leds should be blinking at 200ms interval
  if (rpm > REV_LIMITER_RPM) {
    if (revLimiterOn) {
      setGlobalState(LOW);
    } else {
      setGlobalState(HIGH);
    }

    revLimiterOn = !revLimiterOn;
    return;
  }

  for (int i = 0; i < PINS_COUNT; i++) {
      if (rpm > LED_SWITCH_RPM[i]) {
          digitalWrite(LED_PINS[i], HIGH);
      }
  }
} 

/*
 * Turns on VTEC solenoid based on VTEC_ON_RPM set
 */
void vtec(int rpm) {
  if (rpm > VTEC_ON_RPM) {
    digitalWrite(VTEC_PIN, HIGH);
  } else {
    digitalWrite(VTEC_PIN, LOW);
  }
}

/*
 * Defines led pins as output,
 * turns all leds on for 500ms when started
 * attach to serial if available
 */
void setup() {
  // Define all led pins as outputs
  for (int i = 0; i < PINS_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }

  pinMode(LED_GROUND_PIN, OUTPUT);
  digitalWrite(LED_GROUND_PIN, LOW); // Use pin 8 as ground for the leds

  setGlobalState(HIGH);
  delay(500);
  setGlobalState(LOW);

  pinMode(DATA_PIN, INPUT_PULLUP);
  attachInterrupt(1, incrementRpmCount, FALLING);
  Serial.begin(9600);
}

// 4 stroke engine fires every spark in 2 revolutions
// so calculate at what degree interval sparks fires and divide 360 by it,
// to find the number of fires per rotation
const int FIRES_PER_REV = (360 / (720 / NUMBER_OF_CYLINDERS));

void loop() {
  if ((millis() - lastUpdateTime) > LED_UPDATE_INTERVAL) {

    // multiply the amount the spark fires in one interval by the number of intervals per
    // second, to find the amount in one second
    // then multiply the amount in one second by 60, to find the spark fires in one minute and
    // divide the result by the number of fires per revolution to find the rpm
    int currentRpm = (sparkFireCount * (1000 / LED_UPDATE_INTERVAL) * 60) / FIRES_PER_REV;

    // average the current and last rpm for smoother results
    int averagedRpm = (currentRpm + lastRpmValue) / 2;

    setLedState(averagedRpm);
    Serial.println(averagedRpm);

    sparkFireCount = 0;
    lastUpdateTime = millis();
    lastRpmValue = currentRpm;
    
    // Output rpm value on screen 
    display.setBrightness(0x0f);
    display.showNumberDec(averagedRpm);
    //delay(500);
  
  }
}