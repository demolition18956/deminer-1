byte transmit[7];
//this structure should contain all the data we need for processing during runtime, variables based on GP9 variable types
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
  gpsHealth Health;        //Data Register 0x55 (85)
  float pos_North;         //Data Register 0x7B (123)
  float pos_East;          //Data Register 0x7C (124)
  float vel_North;         //Data Register 0x7F (127)
  float vel_East;          //Data Register 0x80 (128)
  float gps_Lat;           //Data Register 0x84 (132)
  float gps_Long;          //Data Register 0x85 (133)
  float gps_Course;        //Data Register 0x87 (135)
  float gps_Speed;         //Data Register 0x88 (136)
} gpsdata;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);                 //serial0 connection to the computer
  while (!Serial);                    //delays until serial connection to host computer is valid
  Serial3.begin(115200, SERIAL_8N1);  //serial3 connection to the GPS Module
  while (!Serial3);                   //delays until serial connection to GPS is valid
  delay(3000);
}

void loop() {
  // put your main code here, to run repeatedly:
  static gpsdata GPS;
  GPS_Test(GPS);
  Serial.println();
}

void Set_Home_Pos()
{
  unsigned int checksum;
  transmit[0] = 0x73;
  transmit[1] = 0x6E;
  transmit[2] = 0x70;
  transmit[3] = 0x00;
  transmit[4] = 0xAE;
  checksum = (0x73 + 0x6E + 0x70 + 0xAE);
  transmit[6] = (checksum & 0xFF);
  transmit[5] = ((checksum & 0xFF00) >> 8);
}

void GPS_Test(gpsdata GPS)
{
  static int flag = 0;
  gpsdata* GPSp = &GPS;
  delay(200);
  Test(0x55, GPSp);
  Test(0x7B, GPSp);
  Test(0x7C, GPSp);
  Test(0x7F, GPSp);
  Test(0x80, GPSp);
  Test(0x84, GPSp);
  Test(0x85, GPSp);
  Test(0x87, GPSp);
  Test(0x88, GPSp);
  if (flag == 0) {
    Serial.println("Current GPS Data");
    Serial.print("GPS Health: ");
    Serial.println(GPSp->health, HEX);
    Serial.print("Current # Satellites Used: ");
    Serial.println(GPSp->Health.satsUsed, DEC);
    Serial.print("Horizontal Dilution of Precision: ");
    Serial.println(GPSp->Health.hDOP, 8);
    Serial.print("Satellites in View: ");
    Serial.println(GPSp->Health.sats_in_view, DEC);
    if (GPSp->Health.ovf)
    {
      Serial.println("GPS Broadcast Rate too high. Reduce Broadcast rate");
    }
    else
    {
      Serial.println("GPS Broadcast Rate in parameters");
    }
    Serial.print("GPS Status: ");
    if (GPSp->Health.stat == 3)
    {
      Serial.println("Good");
    }
    else if (GPSp->Health.stat == 2)
    {
      Serial.println("Low Quality Lock, GPS Unused");
    }
    else
    {
      Serial.println("No GPS Lock");
    }
    if (GPSp->Health.pressure_fail)
    {
      Serial.println("Pressure Sensor Fail");
    }
    else
    {
      Serial.println("Pressure Sensor OK");
    }
    if (GPSp->Health.accel_fail)
    {
      Serial.println("Accelerometor Sensor Fail");
    }
    else
    {
      Serial.println("Accelerometor Sensor OK");
    }
    if (GPSp->Health.gyro_fail)
    {
      Serial.println("Gyrometer Sensor Fail");
    }
    else
    {
      Serial.println("Gyrometer Sensor OK");
    }
    if (GPSp->Health.mag_fail)
    {
      Serial.println("Magnometer Sensor Fail");
    }
    else
    {
      Serial.println("Magnometer Sensor OK");
    }
    if (GPSp->Health.gps_fail)
    {
      Serial.println("GPS Packet Failed for > 2 Seconds");
    }
    else
    {
      Serial.println("GPS Packet OK");
    }
  }
  Serial.print("North Pos: ");
  Serial.println(GPSp->pos_North, 8);
  Serial.print("East Pos: ");
  Serial.println(GPSp->pos_East, 8);
  Serial.print("North Velocity: ");
  Serial.println(GPSp->vel_North, 8);
  Serial.print("East Velocity: ");
  Serial.println(GPSp->vel_East, 8);
  Serial.print("GPS Latitude: ");
  Serial.println(GPSp->gps_Lat, 8);
  Serial.print("GPS Longitude: ");
  Serial.println(GPSp->gps_Long, 8);
  Serial.print("GPS Course: ");
  Serial.println(GPSp->gps_Course, 8);
  Serial.print("GPS Speed: ");
  Serial.println(GPSp->gps_Speed, 8);
  flag = ((flag + 1) % 20);
}

void Test(byte cmd, gpsdata* GPS)
{
  delay(50);
  //Serial.print("Testing Register 0x");
  //Serial.println(cmd, HEX);
  Send_GPS_Request(cmd);
  delay(50);
  Receive_GPS_Data(GPS);
}

void Send_GPS_Request(byte cmd)
{
  int i;
  unsigned short checksum;
  checksum = 0;
  transmit[0] = 0x73;
  transmit[1] = 0x6E;
  transmit[2] = 0x70;
  transmit[3] = 0x00;
  transmit[4] = cmd;
  for (i = 0; i < 5; i++)
  {
    checksum = checksum + transmit[i];
  }
  transmit[6] = (checksum & 0xFF);
  transmit[5] = ((checksum & 0xFF00) >> 8);
  Serial3.write(transmit, 7);
}

void Receive_GPS_Data(gpsdata* GPS)
{
  int i, j, k, a;
  int state = 0;
  int hasData;
  int batch;
  int batchLength;
  unsigned int checkSum = 0;
  unsigned int checkValue = 0;
  byte rawDataValue[4];
  //Serial.println(rawDataValue, HEX);
  byte receive[71];

  a = 0;
  while (!Serial3.available());
  while (Serial3.available())
  {
    receive[a] = Serial3.read();
    a = a + 1;
  }
  //Serial.print("Data Received: ");
  //Serial.println(a, DEC);
  //Serial.println("Raw Data Array:");
  //for(i = 0; i < a; i++)
  //{
  //if(receive[i] < 0x10)
  //{
  //Serial.print("0");
  //}
  //Serial.print(receive[i], HEX);
  //Serial.print(" ");
  //}
  //Serial.println();
  for (i = 0; i < a; i++)               //this will run through the entire received buffer
  { //first checks for header "snp", then packet type and length
    switch (state)
    {
      case 0: if (receive[i] == 0x73)   //header byte 1('s')
        {
          //Serial.println("First Header Byte Found");
          j = i;                  //sets a starting point for the packet
          state = 1;              //if match, go to the next state
        }
        break;

      case 1: if (receive[i] == 0x6E)   //header byte 2 ('n')
        {
          //Serial.println("Second Header Byte Found");
          state = 2;
        }
        else
        {
          state = 0;
        }
        break;

      case 2: if (receive[i] == 0x70)   //header byte 3 ('p')
        {
          //Serial.println("Third Header Byte Found");
          state = 3;
        }
        else
        {
          state = 0;
        }
        break;

      case 3: hasData = ((receive[i] & 0x80) >> 7);
        batch = ((receive[i] & 0x40) >> 6);
        if (hasData == 0)   //if no data, look for the next header
        {
          state = 0;
          break;
        }
        else if (batch == 0)
        {
          if ((a - i) < 7)
          {
            state = 0;
            break;
          }
          else
          {
            checkSum = 0;
            for (k = j; k < 9; k++)
            {
              checkSum = checkSum + receive[k];
            }
            checkValue = ((receive[j + 9] << 8) | (receive[j + 10]));
            //Serial.println(checkSum, HEX);
            //Serial.println(checkValue, HEX);
            if (checkSum != checkValue)
            {
              Serial.println("Error with checksum");
              state = 0;
              break;
            }
            //Serial.println("Checksum good");
            ProcessData(GPS, &receive[i + 1], 1);
            state = 0;
            break;
          }
        }
        else
        {
          batchLength = ((receive[i] & 0x3C) >> 2);
          if ((a - i) < ((batchLength * 4) + 3))
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

void ProcessData(gpsdata* gps, byte* aValue, int count)
{
  static int doptemp;
  byte tempF[4];
  byte sRegister = *aValue;
  for (int i = 0; i < count; i++)
  {
    switch (sRegister)
    {
      case 0x55:  //Reverse BYTE ORDER
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->health = *(unsigned long*)&tempF[0];
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
        gps->pos_North = *(float*)&tempF[0];
        break;

      case 0x7C:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->pos_East = *(float*)&tempF[0];
        break;

      case 0x7D:
      case 0x7E:  break;

      case 0x7F:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->vel_North = *(float*)&tempF[0];
        break;

      case 0x80:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->vel_East = *(float*)&tempF[0];
        break;
      case 0x81:
      //case 0x82:  This register is reserved so we don't need to use
      case 0x83:  break;

      case 0x84:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->gps_Lat = *(float*)&tempF[0];
        break;

      case 0x85:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->gps_Long = *(float*)&tempF[0];
        break;
      case 0x86:  break;

      case 0x87:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
        gps->gps_Course = *(float*)&tempF[0];
        break;

      case 0x88:
        tempF[0] = aValue[4 + i * 4];
        tempF[1] = aValue[3 + i * 4];
        tempF[2] = aValue[2 + i * 4];
        tempF[3] = aValue[1 + i * 4];
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
