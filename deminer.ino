/* Deminer 2020

   NOTE: YOU LITERALLY CANNOT PRINT STRINGS
    TO SERIAL IN loop() WITHOUT RUINING THE WHOLE
    THING FOR SOME REASON (Speed will be 0 intermittently)
*/

#include "gps.h"

byte elevator_pin  = 3,   // yellow - Speed
     throttle_pin  = 4,   // blue - Mode
     rudder_pin    = 5,   // black - Turn
     contactor_pin = 7,   // white
     speed_pin     = 8,   // red - Mega to Saber S1
     turn_pin      = 9,   // red - Mega to Saber S2
     horn_pin      = 10,  // blue
     detector_pin  = 11;  // what pin is the metal detector input?

bool debug = true;
int Speed,
    Mode,
    Turn,
    Detector,
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

  // Inputs
  pinMode(elevator_pin,   INPUT); // Speed
  pinMode(throttle_pin,   INPUT); // Mode
  pinMode(rudder_pin,     INPUT); // Turn
  //pinMode(detector_pin,   INPUT); // Detector

  // Outputs
  pinMode(speed_pin,      OUTPUT); // Speed
  pinMode(turn_pin,       OUTPUT); // Turn
  pinMode(contactor_pin,  OUTPUT); // Contactor
  pinMode(horn_pin,       OUTPUT); // Horn

  digitalWrite(speed_pin,     LOW);
  digitalWrite(turn_pin,      LOW);
  digitalWrite(contactor_pin, LOW);
  digitalWrite(horn_pin,      LOW); // How do you turn the horn on?

  // t1 = millis();
  // t2 = millis() + printInterval;

  // navigation_setup();

  // SendGPSRequest(GPSDIRECTION);
  // Receive_GPS_Data(ptr_gps);
  // ProcessData(pts_gps, byte *aValue, int count);

  delay(1000);
}

void loop() {
  //  t1 = millis();
  //  if ((t2 - t1) > 0) {
  //    return;
  //  }
  //  t2 = millis() + printInterval;

  Speed     = pulseIn (elevator_pin, HIGH, 25000);
  Turn      = pulseIn (rudder_pin,   HIGH, 25000);
  Mode      = pulseIn (throttle_pin, HIGH, 25000);
  //Detector  = pulseIn (detector_pin, HIGH, 25000);

  if      (Speed == 0 || Turn == 0)     mode = OFF;
  else if (Mode >= 1800)                mode = STOP;
  else if (Mode < 1800 && Mode > 1200)  mode = AUTO;
  else if (Mode <= 1200)                mode = DRIVE;

  if (debug) {
    // SendGPSRequest(GPSDIRECTION);
    p("%d\t%d\t%d\t%d\t%d\t%d\n", Mode, Speed, Turn, mode, auto_mode, GPS.gps_Course);
    //p("%d\t%d\t%d\t%d \t\t %d\t%d\t%d\n", Mode, Speed, Turn, Detector, mode, auto_mode, GPS.gps_Course);
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
          // get 4 corners (hardcoded by us with 4 #defines)
          // create grid (create 100 waypoints interpolated from those corners)
          // calculate path through grid (simple zig zag)
          // goto closest corner and center yourself in the square
          // record current position as home

          // if everything's ready, then
          auto_mode = DEMINING;
          break;
        case DEMINING:
          // move through path
          // if mine, record its position and send an alert

          // if the whole field has been demined, then
          auto_mode = RETURNING;
          break;
        case RETURNING:
          // go home

          // if back at start position, then
          auto_mode = PRINTING;
          break;
        case PRINTING:
          // print results (contains GPS location of all mines in a nice grid format)

          // if done printing, then
          auto_mode = DONE;
          break;
      }
      break;
    case DRIVE:
      auto_mode = STARTING;
      digitalWrite(contactor_pin, HIGH);
      pulseOut(speed_pin, autoSpeed + (Speed - autoSpeed) / 5 - 50);
      pulseOut(turn_pin,  autoTurn  + (Turn  - autoTurn)  / 5);
      break;
  }
}
