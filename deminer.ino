
//                20 June 2015
//   Basic R/C:  reads/logs time, current, inclination

byte elevator_pin  = 3;  //  speed - yel
byte throttle_pin  = 4;  //  mode - blu
byte rudder_pin    = 5;  //  turn - blk
byte contactor_pin = 7;
byte speed_pin     = 8;  // Mega to Saber S1
byte turn_pin      = 9;  // Mega to Saber S2

int Speed;
int Mode;
int Turn;
int autoSpeed;
int autoTurn;

unsigned long runTime;

void setup()
{
  Serial.begin(9600);      //  serial to monitor
  Serial1.begin(9600);     //  serial to data logger
  
  autoSpeed = 1470;
  autoTurn  = 1490;
  
  pinMode(elevator_pin, INPUT); 
  pinMode(throttle_pin, INPUT); 
  pinMode(rudder_pin, INPUT);
  pinMode(speed_pin, OUTPUT); 
  pinMode(turn_pin, OUTPUT);
 
  digitalWrite(speed_pin, LOW);
  digitalWrite(turn_pin, LOW);
}

void loop() {
//timeout is to avoid default 1 sec when no pulse present   

  Speed = pulseIn (elevator_pin, HIGH, 25000);     
  Turn  = pulseIn (rudder_pin, HIGH, 25000);
  Mode  = pulseIn (throttle_pin, HIGH, 25000);  

//  Serial.print(Mode);
//  Serial.write(9);
//  Serial.print(Speed);
//  Serial.write(9);
//  Serial.println(Turn);
  
//zero when transmitter off 
  if ((Speed == 0) || (Turn == 0)) {           
    digitalWrite(contactor_pin, LOW); 
    }

  else if(Mode >= 1850){                                                     
// throttle down position, STOP 
    digitalWrite(contactor_pin, HIGH);
    pulseOut(speed_pin, 1470);
    pulseOut(turn_pin,  1490); 
//    PrintMon();
//    PrintLogo();
  }
  
// throttle middle position, autonomous  
  else if((Mode > 1200)&&(Mode < 1850)){      
    digitalWrite(contactor_pin, LOW);
//    pulseOut(speed_pin, autoSpeed);
//    pulseOut(turn_pin, autoTurn);
//      PrintLogo();
   }

//  throttle up, drive    
  else if(Mode <= 1200){  
    digitalWrite(contactor_pin, HIGH);  
    Turn = autoTurn + (Turn - autoTurn)/3;                 
    pulseOut(speed_pin, Speed - 50);
    pulseOut(turn_pin,  Turn);
//    PrintMon();
//     Serial1.print(Speed);
//     Serial1.write(9);
//     Serial1.print(Turn);
//     Serial1.write(9);
//      PrintLogo();     
   }    
}

void pulseOut(int pin,int duration) // PulseOut function
{
  digitalWrite(pin,HIGH);
  delayMicroseconds(duration);
  digitalWrite(pin,LOW);
  return;
}   
