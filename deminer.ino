byte elevator_pin  = 3,  //  speed - yel
     throttle_pin  = 4,  //  mode - blu
     rudder_pin    = 5,  //  turn - blk
     contactor_pin = 7,
     //metal_detector_pin = 10
     //LED_pin = 11
     speed_pin     = 8,  // Mega to Saber S1
     turn_pin      = 9;  // Mega to Saber S2
int Speed,
    Mode,
    Turn,
    autoSpeed = 1470,
    autoTurn  = 1490;
unsigned long runTime,
         t1,
         t2,
         printInterval = 1000;
char data[100];

void setup() {
  delay(1000);

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

  t1 = millis();
  t2 = millis() + printInterval;
}

void loop() {
  t1 = millis();
  if ((t2 - t1) > 0) {
    return;
  }
  t2 = millis() + printInterval;

  Speed = pulseIn(elevator_pin, HIGH, 25000);
  Mode  = pulseIn(throttle_pin, HIGH, 25000);
  Turn  = pulseIn(rudder_pin,   HIGH, 25000);

  if ((Speed == 0) || (Turn == 0)) {
    p("Transmitter off");
    digitalWrite(contactor_pin, LOW);
  }
  else if (Mode >= 1850) {
    p("Throttle down (STOP) off");
    digitalWrite(contactor_pin, HIGH);
    pulseOut(speed_pin, autoSpeed);
    pulseOut(turn_pin,  autoTurn);
  }
  else if ((Mode > 1200) && (Mode < 1850)) {
    p("Throttle middle (AUTO)");
    digitalWrite(contactor_pin, LOW);
    //    pulseOut(speed_pin, autoSpeed);
    //    pulseOut(turn_pin, autoTurn);
  }
  else if (Mode <= 1200) {
    p("Throttle up (DRIVE)");
    digitalWrite(contactor_pin, HIGH);
    Turn = autoTurn + (Turn - autoTurn) / 3;
    pulseOut(speed_pin, Speed - 50);
    pulseOut(turn_pin,  Turn);
  }
  p(": %d\t%d\t%d\n", Mode, Speed, Turn);
}
