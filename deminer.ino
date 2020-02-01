/* Deminer 2020

   NOTE: YOU LITERALLY CANNOT PRINT STRINGS
    TO SERIAL IN loop() WITHOUT RUINING THE WHOLE
    THING FOR SOME REASON (Speed will be 0 intermittently)
*/

#include "gps.h"

byte elevator_pin  = 3,  //  speed - yel
     throttle_pin  = 4,  //  mode - blu
     rudder_pin    = 5,  //  turn - blk
     contactor_pin = 7,
     speed_pin     = 8,  // Mega to Saber S1
     turn_pin      = 9,  // Mega to Saber S2
     horn_pin      = 10;
//metal_detector_pin = 11
//LED_pin = 12

bool debug = true;
int Speed,
    Mode,
    Turn,
    autoSpeed = 1470,
    autoTurn  = 1500;
unsigned long runTime,
         t1,
         t2,
         printInterval = 1000;
char data[100];

enum Mode {
  OFF,
  STOP,
  AUTO,
  DRIVE
} mode;

enum Auto_Mode {
  DONE,
  STARTING,
  DEMINING,
  RETURNING,
  PRINTING
} auto_mode;

void setup() {
  Serial.begin(9600);  //  serial to monitor
  Serial1.begin(9600); //  serial to data logger

  pinMode(elevator_pin,  INPUT);
  pinMode(throttle_pin,  INPUT);
  pinMode(rudder_pin,    INPUT);
  pinMode(speed_pin,     OUTPUT);
  pinMode(turn_pin,      OUTPUT);
  pinMode(contactor_pin, OUTPUT);
  // pinMode(metal_detector_pin, INPUT);
  // pinMode(LED_pin,            OUTPUT);

  digitalWrite(speed_pin,     LOW);
  digitalWrite(turn_pin,      LOW);
  digitalWrite(contactor_pin, LOW);
  // digitalWrite(LED_pin

  //  t1 = millis();
  //  t2 = millis() + printInterval;

  //  navigation_setup();

  //  SendGPSRequest(GPSDIRECTION);
  //  Receive_GPS_Data(ptr_gps);
  //ProcessData(pts_gps, byte *aValue, int count);

  delay(1000);
}

void loop() {
  //  t1 = millis();
  //  if ((t2 - t1) > 0) {
  //    return;
  //  }
  //  t2 = millis() + printInterval;

  Speed = pulseIn (elevator_pin, HIGH, 25000);
  Turn  = pulseIn (rudder_pin, HIGH, 25000);
  Mode  = pulseIn (throttle_pin, HIGH, 25000);

  if      (Speed == 0 || Turn == 0)     mode = OFF;
  else if (Mode >= 1800)                mode = STOP;
  else if (Mode < 1800 && Mode > 1200)  mode = AUTO;
  else if (Mode <= 1200)                mode = DRIVE;

  if (debug) {
    SendGPSRequest(GPSDIRECTION);
    p("%d\t%d\t%d\t%d\t%d\t%d\n", Mode, Speed, Turn, mode, auto_mode, GPS.gps_Course);
  }

  switch (mode) {
    case OFF:
    case STOP:
      auto_mode = STARTING;
      digitalWrite(contactor_pin, LOW);
      break;
    case AUTO:
      digitalWrite(contactor_pin, HIGH);
      switch (auto_mode) {
        case DONE:
          break;
        case STARTING:
          // get 4 coords
          // create grid
          // calculate path through grid using 4 coords

          // if ready, then
          auto_mode = DEMINING;
          break;
        case DEMINING:
          // move through path
          // if mine, record its position and send an alert

          // if done
          auto_mode = RETURNING;
          break;
        case RETURNING:
          // go to start position

          // if back at start position, then
          auto_mode = PRINTING;
          break;
        case PRINTING:
          // print results

          // if done printing, then
          auto_mode = DONE;
          break;
      }
      break;
    case DRIVE:
      auto_mode = STARTING;
      digitalWrite(contactor_pin, HIGH);
      if (Speed<1470 || Speed>1500 || Turn>1520 || Turn<1490) {
        pulseOut(speed_pin, autoSpeed + (Speed - autoSpeed) / 5 - 50);
        pulseOut(turn_pin,  autoTurn  + (Turn  - autoTurn)  / 5);
      }
      break;
  }
}
