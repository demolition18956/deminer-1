// Tim Land
#define NUMBEROFCORNERS 4
#define NUMBEROFWAYPOINTS 81
#define GPSHEALTH 0x55
#define GPSPOSNORTH 0x7B
#define GPSPOSEAST 0x7C
#define GPSVELNORTH 0x7F
#define GPSVELEAST 0x80
#define GPSLAT 0x84
#define GPSLONG 0x85
#define GPSDIRECTION 0x87
#define GPSVELOCITY 0x88

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

typedef struct {
  unsigned long health;
  gpsHealth Health; // Data Register 0x55 (85)
  float pos_North;  // Data Register 0x7B (123)
  float pos_East;   // Data Register 0x7C (124)
  float vel_North;  // Data Register 0x7F (127)
  float vel_East;   // Data Register 0x80 (128)
  float gps_Lat;    // Data Register 0x84 (132)
  float gps_Long;   // Data Register 0x85 (133)
  float gps_Course; // Data Register 0x87 (135)
  float gps_Speed;  // Data Register 0x88 (136)
} gpsData;

typedef struct {
  float gps_Lat;
  float gps_Long;
} boundaryCorner;

typedef struct {
  String id;
  float distanceFromHomeNorth;
  float distanceFromHomeEast;
  bool hasVisited;
  bool isMined;
} waypoint;

typedef struct {
  float distanceFromHomeNorth;
  float distanceFromHomeEast;
  float degreeDirection;
  float velocity;
  bool GPSGood;
} roverData;

void navigation_setup();
void navigation_loop();
void GPS_Setup(gpsData *ptr_gps);
void BoundarySetup(boundaryCorner *b_PTR);
void WaypointSetup(waypoint *wpoint);
void RoverSetup(roverData *rover);
void SendGPSRequest(byte addr);
void Receive_GPS_Data(gpsData *GPS);
void ProcessData(gpsData *gps, byte *aValue, int count);

gpsData GPS;

void navigation_setup() {
  // put your setup code here, to run once:
  gpsData *ptr_gps;
  ptr_gps = &GPS;
  roverData rover, *ptr_rover;
  boundaryCorner corners[NUMBEROFCORNERS];
  waypoint wpoints[NUMBEROFWAYPOINTS];
  Serial3.begin(115200, SERIAL_8N1);
  while (!Serial3) {
  }
  GPS_Setup(ptr_gps);
  BoundarySetup(corners);
  WaypointSetup(wpoints);
  ptr_rover = &rover;
  RoverSetup(ptr_rover);
}

void navigation_loop() {
  // put your main code here, to run repeatedly:
}

void GPS_Setup(gpsData *ptr_gps) {
  while (ptr_gps->Health.stat !=
         3) // this waits for a GPS lock on enough satellites
  {
    SendGPSRequest(GPSHEALTH);
    Receive_GPS_Data(ptr_gps);
  }
}

void BoundarySetup(boundaryCorner *b_PTR) {}

void WaypointSetup(waypoint *wpoint) {
  int i;
  int row, column;
  for (i = 0; i < NUMBEROFWAYPOINTS; i++) {
    row = i / NUMBEROFWAYPOINTS;
    column = i % NUMBEROFWAYPOINTS;
  }
}

void RoverSetup(roverData *rover) {}

void SendGPSRequest(byte addr) {
  int i;
  unsigned short checksum;
  static byte transmit[7];
  transmit[0] = 0x73;
  transmit[1] = 0x6E;
  transmit[2] = 0x70;
  transmit[3] = 0x00;
  transmit[4] = addr;
  for (i = 0; i < 5; i++) {
    checksum = checksum + transmit[i];
  }
  transmit[6] = (checksum & 0xFF);
  transmit[5] = ((checksum & 0xFF00) >> 8);
  Serial3.write(transmit, 7);
}

void Receive_GPS_Data(gpsData *GPS) {
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
  while (!Serial3.available())
    ;
  while (Serial3.available()) {
    receive[a] = Serial3.read();
    a = a + 1;
  }

  for (i = 0; i < a; i++) // this will run through the entire received buffer
  { // first checks for header "snp", then packet type and length
    switch (state) {
    case 0:
      if (receive[i] == 0x73) // header byte 1('s')
      {
        j = i;     // sets a starting point for the packet
        state = 1; // if match, go to the next state
      }
      break;

    case 1:
      if (receive[i] == 0x6E) // header byte 2 ('n')
      {
        state = 2;
      } else {
        state = 0;
      }
      break;

    case 2:
      if (receive[i] == 0x70) // header byte 3 ('p')
      {
        state = 3;
      } else {
        state = 0;
      }
      break;

    case 3:
      hasData = ((receive[i] & 0x80) >> 7);
      batch = ((receive[i] & 0x40) >> 6);
      if (hasData == 0) // if no data, look for the next header
      {
        state = 0;
        break;
      } else if (batch == 0) {
        if ((a - i) < 7) {
          state = 0;
          break;
        } else {
          checkSum = 0;
          for (k = j; k < 9; k++) {
            checkSum = checkSum + receive[k];
          }
          checkValue = ((receive[j + 9] << 8) | (receive[j + 10]));
          if (checkSum != checkValue) {
            Serial.println("Error with checksum");
            state = 0;
            break;
          }
          ProcessData(GPS, &receive[i + 1], 1);
          state = 0;
          break;
        }
      } else {
        batchLength = ((receive[i] & 0x3C) >> 2);
        if ((a - i) < ((batchLength * 4) + 3)) {
          state = 0;
          break;
        }
      }
    default:
      state = 0;
      break;
    }
  }
}

void ProcessData(gpsData *gps, byte *aValue, int count) {
  static int doptemp;
  byte tempF[4];
  byte sRegister = *aValue;
  for (int i = 0; i < count; i++) {
    switch (sRegister) {
    case 0x55: // Reverse BYTE ORDER
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->health = *(unsigned long *)&tempF[0];
      gps->Health.satsUsed = (gps->health & 0xFC000000) >> 0x1A;
      doptemp = (gps->health & 0x3FF0000) >> 0x10;
      gps->Health.hDOP = float(doptemp) / 10.0;
      gps->Health.sats_in_view = (gps->health & 0xFC00) >> 0xA;
      gps->Health.ovf = ((gps->health & 0x200) >> 9);
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
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->pos_North = *(float *)&tempF[0];
      break;

    case 0x7C:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->pos_East = *(float *)&tempF[0];
      break;

    case 0x7D:
    case 0x7E:
      break;

    case 0x7F:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->vel_North = *(float *)&tempF[0];
      break;

    case 0x80:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->vel_East = *(float *)&tempF[0];
      break;
    case 0x81:
    // case 0x82:  This register is reserved so we don't need to use
    case 0x83:
      break;

    case 0x84:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->gps_Lat = *(float *)&tempF[0];
      break;

    case 0x85:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->gps_Long = *(float *)&tempF[0];
      break;
    case 0x86:
      break;

    case 0x87:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->gps_Course = *(float *)&tempF[0];
      break;

    case 0x88:
      tempF[0] = aValue[4 + i * 4];
      tempF[1] = aValue[3 + i * 4];
      tempF[2] = aValue[2 + i * 4];
      tempF[3] = aValue[1 + i * 4];
      gps->gps_Speed = *(float *)&tempF[0];
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
    default:
      break;
    }
    sRegister++;
  }
}
