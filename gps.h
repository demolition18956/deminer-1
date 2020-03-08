#ifndef ROVERUPDATE
#define ROVERUPDATE
#include <math.h>

#define NUMBEROFCORNERS   4
#define NUMBEROFWAYPOINTS   81
#define MINEFIELDWIDTH    9
#define GPSHEALTH       0x55
#define GPSPOSNORTH     0x7B
#define GPSPOSEAST      0x7C
#define GPSVELNORTH     0x7F
#define GPSVELEAST      0x80
#define GPSLAT        0x84
#define GPSLONG       0x85
#define GPSDIRECTION    0x87
#define GPSVELOCITY     0x88
#define GPSFLASHCOMMIT    0xAB
#define GPSSETHOME      0xAE
#define deg2rad       0.017453292 //this is the most accurate a 32bit float can represent pi/180
#define rad2deg       57.29577951 //same for this value
#define lat2meter     110935.42
#define lon2meter     91598.80


/* This structure is updated when register 0x55 is requested from the GPS Module. It should not be needed much
   during the running of the program, and I only currently plan to use it during initial power on.*/
typedef struct {
  int satsUsed;
  float hDOP;
  int sats_in_view;
  bool ovf;
  byte stat;
  bool pressure_fail;
  bool accel_fail;
  bool gyro_fail;
  bool mag_fail;
  bool gps_fail;
} gpsHealth;

/* This structure holds all the values we think we need from the GPS module. Each value is matched
   to the register you need to request.*/
typedef struct {    
  unsigned long health;
  gpsHealth Health;        //Data Register 0x55 (85)
  float pos_North;         //Data Register 0x7B (123)
  float pos_East;          //Data Register 0x7C (124)
  float vel_North;         //Data Register 0x7F (127)
  float vel_East;          //Data Register 0x80 (128)
  float gps_Lat;           //Data Register 0x84 (132)
  float gps_Long;          //Data Register 0x85 (133)
  float gps_Course;        //Data Register 0x87 (135)
  float gps_Speed;         //Data Register 0x88 (136)
} gpsData;

/* Basic structure to include the two values needed for each boundary corner.
   The values should be set as actual values before program starts (in boundary initialization)*/
typedef struct {
  float gps_Lat;
  float gps_Long;
} boundaryCorner;

/* This structure defines what I think we need for each waypoint to be able to fully navigate and check for mines
   If we need to change anything, it would be this and/or the roverData structure.*/
typedef struct {
  char id[2];         //used to allows us to easily identify which waypoint it is on our grid
  float distanceFromHomeNorth;  //should be calculated in waypoint initialization, needs trig to be accurate
  float distanceFromHomeEast; //should be calculated in waypoint initialization, needs trig to be accurate
  bool hasVisited;        //simple true false to see if the rover has checked this point or not.
  bool isMined;         //should start out false, and only set to true if the rover finds a mine.
} waypoint;

/* This structure can also be modified for the main program. It's what I currently believe the rover needs for data
   to accurately navigate.*/
typedef struct {
  float distanceFromHomeNorth;
  float distanceFromHomeEast;
  float degreeDirection;
  float velocity;
  bool GPSGood;
} roverData;

void BeginningSetup(gpsData*, boundaryCorner*, waypoint*, roverData*);
void GPS_Setup(gpsData*);
void BoundarySetup(boundaryCorner*);
void WaypointSetup(waypoint*, boundaryCorner*);
void RoverSetup(roverData*, gpsData*);
void GetGenericData(byte, gpsData*);
void SendGPSRequest(byte);
void ReceiveGPSData(gpsData*);
void ProcessData(gpsData*, byte*, int);


/*  This is a function to allow only one line of code inside the main .ino for setting up the GPS and the
  Rover for operations. Ideally, this should be called once the rover has been placed on the starting point
  which should also be boundary point 0 in the array.
  */
void BeginningSetup(gpsData* gps, boundaryCorner* bp, waypoint* wp, roverData* rd)
{
  GPS_Setup(gps);
  BoundarySetup(bp);
  WaypointSetup(wp, bp);
  RoverSetup(rd, gps);
}

/*  This function will ensure the GPS module is working correctly. It will wait until it has enough
  satellites, then set the home position to boundary point 0 
  */
void GPS_Setup(gpsData* ptr_gps)
{
  int count = 0;
  while((ptr_gps->Health.stat != 3) && (count < 5))   //this waits for a GPS lock on enough satellites
  {
    GetGenericData(GPSHEALTH, ptr_gps);
  count++;
  delay(500);
  }
  //this sets the home position to where the rover currently is as reported by the GPS.
  //Do we need to find some way to wait until it's at position 0 to run this command?
  //GetGenericData(GPSSETHOME, ptr_gps);
    
}

/*  The boundary setup function will set up the corners of minefield based on the constant values
  given to the team. Current format is in decimal degrees. 
  */
void BoundarySetup(boundaryCorner* b_PTR)
{
  //this part is test code for the corners:
  //all values currently assume direct north and east alignment of the grid
  //this is where the constant values for Lat/Lon should be entered for final code
  //these current points have been calculated online to be as close to 20m edges as possible
  //I have NOT scouted these points out except through Google Maps
  //corners[0] should be the start point
  b_PTR[0].gps_Lat =   34.7192830;
  b_PTR[0].gps_Long = -86.6425000;
  //corners[1] should be 20m directly in front of the start point
  b_PTR[1].gps_Lat =   34.7194629 ;  //20m directly north from corners[0] equals a Latitude change of 0.00017979
  b_PTR[1].gps_Long = -86.6425000;  //For directly north, this is the same as corners[0].gps_Long
  //corners[2] should be the 20m directly to the right of start point
  b_PTR[2].gps_Lat =   34.7192830;
  b_PTR[2].gps_Long = -86.6422812;
  //corners[3] should be 20m right and 20m front of start point, approx. 28.28m diagonally
  b_PTR[3].gps_Lat =   34.7194629;  //should be corners[0].gps_Lat + 0.02
  b_PTR[3].gps_Long = -86.6422812;  //should be corners[0].gps_Long + 0.00
}

/*  Waypoint setup should build the "map" of the points we need to check within the minefield. 
  The points will be based on the degree of direction between point 0 and point 1 of the boundary points
  Each point will be in a square grid, at the center of a 2x2 m square.
  */
void WaypointSetup(waypoint* wpoint, boundaryCorner* bptr)
{

  float difflong, difflat, longMdiff, latMdiff;
  difflat = bptr[1].gps_Lat - bptr[0].gps_Lat;
  difflong = bptr[1].gps_Long - bptr[0].gps_Long;
  longMdiff = (difflong/10.0) * lon2meter;
  latMdiff = (difflat/10.0) * lat2meter;
  int i;
  int row, column;
  
  //first waypoint is special as it needs the trig. Everything else is a pure 2m*(row|column)(north|east) from the initial
  wpoint[0].distanceFromHomeNorth = 2*latMdiff;   
  wpoint[0].distanceFromHomeEast = 2*longMdiff;     
  wpoint[0].hasVisited = false;
  wpoint[0].isMined = false;
  wpoint[0].id[0] = 'A';
  wpoint[0].id[1] = '1';
  
  for(i = 1; i < NUMBEROFWAYPOINTS; i++)
  {
    row = (i-1) / MINEFIELDWIDTH;
    column = (i-1) % MINEFIELDWIDTH;
  
  wpoint[i].distanceFromHomeNorth = wpoint[0].distanceFromHomeNorth + ((float)(row*2))*latMdiff;    
  wpoint[i].distanceFromHomeEast = wpoint[0].distanceFromHomeEast + ((float)(column*2))*longMdiff;  
  wpoint[i].hasVisited = false;
  wpoint[i].isMined = false;
  wpoint[i].id[0] = (char)(row + 0x41);
  wpoint[i].id[1] = (char)(column + 0x31);
  
  }
}


/*  Once the GPS is set up, this will set the Rover's home position (prefereably at boundary position 0)
  as well as the initial values for the rover data
   */
void RoverSetup(roverData* rover, gpsData* gps)
{
  rover->GPSGood = false;
  int count = 0;
  while((!rover->GPSGood) && (count < 10))
  {
    GetGenericData(GPSHEALTH, gps);
    if(gps->Health.gps_fail == 0)
      rover->GPSGood = true;
    else
      rover->GPSGood = false;
    count++;
    delay(500);
  }
  GetGenericData(GPSPOSNORTH, gps);
  GetGenericData(GPSPOSEAST, gps);
  GetGenericData(GPSDIRECTION, gps);
  GetGenericData(GPSVELOCITY, gps);
  rover->distanceFromHomeNorth = gps->pos_North;  //ideally, these values should be the home position at the start
  rover->distanceFromHomeEast = gps->pos_East;
  rover->degreeDirection = gps->gps_Course;
  rover->velocity = gps->gps_Speed;
}

/*  This function is the one line needed to call once this is a header file, where we can ask for any address
  inside the GPS data structure. It makes the interaction between the Arduino and the GPS mostly transparent*/
void GetGenericData(byte addr, gpsData* GPS)
{
  SendGPSRequest(addr);
  ReceiveGPSData(GPS);
}


/*  This function takes in one byte that is the value of the register of data you are requesting from the
  GPS. This function does not return any value. */
void SendGPSRequest(byte addr)
{
  int i;
  unsigned short checksum;
  static byte transmit[7];
  transmit[0] = 0x73;
  transmit[1] = 0x6E;
  transmit[2] = 0x70;
  transmit[3] = 0x00;
  transmit[4] = addr;
  for(i = 0; i < 5; i++)
  {
    checksum = checksum + transmit[i];
  }
  transmit[6] = (checksum & 0xFF);
  transmit[5] = ((checksum & 0xFF00) >> 8);
  Serial3.write(transmit,7);
}

/*  This function takes in a pointer to the gpsData stucture. It will check the Receive buffer of the 
  UART attached to the GPS module, and pull in the oldest message waiting in the buffer.
  It conducts a calculation on the checksum to ensure that the message transmitted to the Arduino with
  no errors. If everything is working, it will call ProcessGPSData to update the corresponding value 
  within the GPS data structure.
  */
void ReceiveGPSData(gpsData* GPS)
{
  int i, j, k, a;
  int state = 0;
  int hasData;
  int batch;
  int batchLength;
  unsigned int checkSum = 0;
  unsigned int checkValue = 0;
  byte rawDataValue[4];
  byte receive[71];

  a = 0;
  while(!Serial3.available());
  while(Serial3.available())
  {
    receive[a] = Serial3.read();
    a = a + 1;
  }
 
  for(i = 0; i < a; i++)                //this will run through the entire received buffer
  {                                     //first checks for header "snp", then packet type and length
    switch(state)
    {
      case 0: if(receive[i] == 0x73)    //header byte 1('s')
              {
                j = i;                  //sets a starting point for the packet
                state = 1;              //if match, go to the next state
              }
              break;
      
      case 1: if(receive[i] == 0x6E)    //header byte 2 ('n')
              {
                state = 2;
              }
              else
              {
                state = 0;
              }
              break;

      case 2: if(receive[i] == 0x70)    //header byte 3 ('p')
              {
                state = 3;
              }
              else
              {
                state = 0;
              }
              break;

      case 3: hasData = ((receive[i]&0x80)>>7);
              batch = ((receive[i]&0x40)>>6);
              if(hasData == 0)    //if no data, look for the next header
              {
                state = 0;
                break;
              }
              else if(batch == 0)
              {
                if((a-i) < 7)
                {
                  state = 0;
                  break;
                }
                else
                {
                  checkSum = 0;
                  for(k = j; k < 9; k++)
                  {
                    checkSum = checkSum + receive[k];
                  }
                  checkValue = ((receive[j+9]<<8) | (receive[j+10]));
                  if(checkSum != checkValue)
                  {
                    Serial.println("Error with checksum");
                    state = 0;
                    break;
                  }
                  ProcessData(GPS, &receive[i+1], 1);
                  state = 0;
                  break;                  
                }
              }
              else
              {
                batchLength = ((receive[i]&0x3C)>>2);
                if((a-i) < ((batchLength*4)+3))
                {
                  state = 0;
                  break;
                }
              }
      default: state = 0;
               break;
    }
  }
}

/*  ProcessData is even more transparent to the user than it was in earlier iterations.
  It has the capability to process the data from 1 to 16 registers (the max the GPS sends at one time).
  It includes the byte reversing needed to correct the endianess conflict between the GPS and the Arduino
  Most of the code for the unneeded registers are empty and commented out, with me focusing only on the 
  needed registers for my senior project.
  */
void ProcessData(gpsData* gps, byte* aValue, int count)
{
  static int doptemp;
  byte tempF[4];
  byte sRegister = *aValue;
  for(int i = 0; i < count; i++)
  {
    switch(sRegister)
    {
      case 0x55:
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->health = *(unsigned long*)&tempF[0];
            gps->Health.satsUsed = (gps->health & 0xFC000000) >> 0x1A;
            doptemp = (gps->health & 0x3FF0000) >> 0x10;
            gps->Health.hDOP = float(doptemp) / 10.0;
            gps->Health.sats_in_view = (gps->health & 0xFC00) >> 0xA;
            gps->Health.ovf = ((gps->health & 0x200)>>9);
            gps->Health.stat = (gps->health & 0x60) >> 5;
            gps->Health.pressure_fail = ((gps->health & 0x10) >> 4);
            gps->Health.accel_fail = ((gps->health & 0x8) >> 3);
            gps->Health.gyro_fail = ((gps->health & 0x4) >> 2);
            gps->Health.mag_fail = ((gps->health & 0x2) >> 1);
            gps->Health.gps_fail = (gps->health & 0x1);
            break;
      
      /*case 0x56:
      case 0x57:
      case 0x58:
      case 0x59:
      case 0x5A:
      case 0x5B:
      case 0x5C:
      case 0x5D:
      case 0x5E:
      case 0x5F:
      
      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63:
      case 0x64:
      case 0x65:
      case 0x66:
      case 0x67:
      case 0x68:
      case 0x69:
      case 0x6A:
      case 0x6B:
      case 0x6C:
      case 0x6D:
      case 0x6E:
      case 0x6F:
      
      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73:
      case 0x74:
      case 0x75:
      case 0x76:
      case 0x77:
      case 0x78:
      case 0x79:None of the arrays commented out are needed for this program
      case 0x7A:*/
      
      case 0x7B:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->pos_North = *(float*)&tempF[0];
            break;
            
      case 0x7C:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->pos_East = *(float*)&tempF[0];
            break;
            
      case 0x7D:
      case 0x7E:  break;
      
      case 0x7F:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->vel_North = *(float*)&tempF[0];
            break;
      
      case 0x80:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];     
            gps->vel_East = *(float*)&tempF[0];
            break;
      case 0x81:
      //case 0x82:  This register is reserved so we don't need to use
      case 0x83:  break;
      
      case 0x84:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->gps_Lat = *(float*)&tempF[0];
            break;
            
      case 0x85:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->gps_Long = *(float*)&tempF[0];
            break;
      case 0x86:  break;
      
      case 0x87:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->gps_Course = *(float*)&tempF[0];
            break;
            
      case 0x88:  
            tempF[0] = aValue[4 + i*4];
            tempF[1] = aValue[3 + i*4];
            tempF[2] = aValue[2 + i*4];
            tempF[3] = aValue[1 + i*4];
            gps->gps_Speed = *(float*)&tempF[0];
            break;
            
      /*case 0x89:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8D:
      case 0x8E:
      case 0x8F:
      
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9A:*/
      default:  break;
    }
    sRegister++;
  }
}

#endif
