/*
  This is the modified code that goes onto teh original mega
  to control the rover all of the new code to make this mega
  talk to the gps mega is in the loop where the auto mode starts.
  It talks over the Serial3 port like before. If you need more
  explaination on how it works, let me know - Jacob
*/

bool gpstest = 0;
unsigned long lastMillis;

byte elevator_pin  = 3,   // yellow - Speed
     throttle_pin  = 4,   // blue - Mode
     rudder_pin    = 5,   // black - Turn
     contactor_pin = 7,   // white
     speed_pin     = 8,   // red - Mega to Saber S1
     turn_pin      = 9,   // red - Mega to Saber S2
     horn_pin      = 10,  // blue
     detector_pin  = 11;  // white

int Speed,
    Mode,
    Turn,
    Detector,
    autoSpeed,
    autoTurn;

char data[100];

enum Auto_Mode {
  DONE,
  STARTING,
  DEMINING,
  RETURNING,
  PRINTING
} auto_mode;

void setup() {
  Serial.begin(9600);  // stdout
  //Serial1.begin(9600); // data logger
  start();

	autoSpeed = 1440;
	autoTurn  = 1500;

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

  delay(1000);
}

void loop() {
  Speed     = pulseIn (elevator_pin, HIGH, 25000);
  Turn      = pulseIn (rudder_pin,   HIGH, 25000);
  Mode      = pulseIn (throttle_pin, HIGH, 25000);
  Detector  = digitalRead (detector_pin);

/*
	Serial.print(Mode);
  Serial.write(9);
  Serial.print(Speed);
  Serial.write(9);
  Serial.println(Turn);
*/

	//zero when transmitter off
	  if ((Speed == 0) || (Turn == 0)) {
			auto_mode = STARTING;
	    digitalWrite(contactor_pin, LOW);
	    }

	  else if(Mode >= 1800){
	// throttle down position, STOP
			auto_mode = STARTING;
	    digitalWrite(contactor_pin, HIGH);
	    pulseOut(speed_pin, 1440);
	    pulseOut(turn_pin,  1500);
	  }

	// throttle middle position, autonomous
	  else if((Mode > 1200)&&(Mode < 1800)){

      if (Serial3.available() > 0)
      {
        // read the incoming byte:
        float inFloat = Serial3.parseFloat();
        //Serial.print("Received: ");
        Serial.println(inFloat);
      }

      if (millis() - lastMillis >= 2*1000UL)
      {
        lastMillis = millis();  //get ready for the next iteration
        Serial3.write(GPSDIRECTION);
      }

      /*
      digitalWrite(contactor_pin, HIGH);
      pulseOut(speed_pin, autoSpeed-20);
      pulseOut(turn_pin, autoTurn);
      */
      digitalWrite(contactor_pin, HIGH);
      pulseOut(speed_pin, 1440);
      pulseOut(turn_pin,  1500);

			switch (auto_mode) {
				case DONE:
					break;
				case STARTING:
					// get 4 corners {lat,long} (hardcoded by us with 4 #defines)
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
	   }

	//  throttle up, drive
	  else if(Mode <= 1200){
			auto_mode = STARTING;
	    digitalWrite(contactor_pin, HIGH);
	    Turn = autoTurn + (Turn - autoTurn) / 3;
	    pulseOut(speed_pin, Speed - 25);
	    pulseOut(turn_pin,  Turn);
	}
}
