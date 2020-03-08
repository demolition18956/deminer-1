/* Deminer 2020

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
    autoTurn  = 1480;
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

typedef struct {
  float x;
  float y;
} pos;

#define N 10
pos cpos,c1,c2,c3,c4;
int ci = 0;
pos path[N*N];
bool  mines[N*N];
float W,H;

void setup() {
  Serial.begin(9600);  // stdout
  Serial1.begin(9600); // data logger

  // Inputs
  pinMode(elevator_pin,   INPUT); // Speed
  pinMode(throttle_pin,   INPUT); // Mode
  pinMode(rudder_pin,     INPUT); // Turn
  pinMode(detector_pin,   INPUT); // Detector

  // Outputs
  pinMode(speed_pin,      OUTPUT); // Speed
  pinMode(turn_pin,       OUTPUT); // Turn
  pinMode(contactor_pin,  OUTPUT); // Contactor
  pinMode(horn_pin,       OUTPUT); // Horn

  digitalWrite(speed_pin,     LOW);
  digitalWrite(turn_pin,      LOW);
  digitalWrite(contactor_pin, LOW);
  digitalWrite(horn_pin,      LOW);

  // t1 = millis();
  // t2 = millis() + printInterval;

  //setup_rover()


  c1 = (pos){ .x=0.0, .y=0.0 };
  c2 = (pos){ .x=1.0, .y=0.0 };
  c3 = (pos){ .x=0.0, .y=1.0 };
  c4 = (pos){ .x=1.0, .y=1.0 };

  cpos = c1;

  delay(1000);
}


void moveTo(pos p){
  // face rover towards p
  // move  
}

bool closeEnough(pos p1,pos p2){
  // if p1 and and p2 are within a certain distance
  // return abs(p1.x-p2.x)<1 && abs(p1.y-p2.y)<1
  return true;
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
  Detector  = digitalRead (detector_pin);

  if      (Speed == 0 || Turn == 0)     mode = OFF;
  else if (Mode >= 1800)                mode = STOP;
  else if (Mode < 1800 && Mode > 1200)  mode = AUTO;
  else if (Mode <= 1200)                mode = DRIVE;

  if (debug) {
    // SendGPSRequest(GPSDIRECTION);
    p("%d\t%d\t%d\t%d\t%d\t%d\n", Mode, Speed, Turn, Detector, mode, auto_mode);
  }

  // if (Serial2.available() > 0) Serial.println(Serial2.read(), DEC);

  if(Detector>1){
    //digitialWrite(horn_pin, HIGH);
    p("Found mine at %d,%d",cpos.x,cpos.y);
    mines[ci] = true;
  }else{
    //digitialWrite(horn_pin, LOW);
  }

  switch (mode) {
    case OFF:
      break;

    case STOP:
      auto_mode = STARTING;
      digitalWrite(contactor_pin, LOW);
      break;
      
    case AUTO:
      digitalWrite(contactor_pin, LOW);
      // digitialWrite(horn_pin, HIGH); // YOU WILL DIE
      // pulseOut(horn_pin, 10000); // HOW DO I MAKE IT QUIETER???
      switch (auto_mode) {
        case DONE:
          break;
        case STARTING:
          W = c2.x - c1.x;
          H = c4.y - c1.y;
          for(float x=0;x<N;x++){
            for(float y=0;y<N;y++){
              path[int(x*N+y)] = pos { .x=(c1.x + x*W/float(N)), .y=(c1.y + (int(x)%2==0?y:N-y)*H/float(N)) };
            }
          }
          for(int i=0;i<N*N;i++){
            p("%d",path[i]);
          }
          
          auto_mode = DEMINING;
          break;
        case DEMINING:
          moveTo(path[ci+1]);
          if(ci==N*N-1){
            auto_mode = RETURNING; 
          }
          break;
        case RETURNING:
          moveTo(path[0]);
          if(closeEnough(cpos,path[0])){
            auto_mode = PRINTING;
          }
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
      pulseOut(speed_pin, autoSpeed);
      pulseOut(turn_pin,  autoTurn);
      break;
  }
}
