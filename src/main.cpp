
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

const int TFT_CS = 18;
const int TFT_DC = 20;
const int TFT_MOSI = 21;
const int TFT_SLK = 22;
const int TFT_RST = 19;
const int TFT_LED = -1;
const int TFT_MISO = 23;

int32_t last_draw = 0;

SPIClass spi(FSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&spi, TFT_DC, TFT_CS, TFT_RST);

// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SLK,
// TFT_RST, TFT_MISO);

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define CLEAR_COLOR ILI9341_WHITE

Adafruit_MPU6050 mpu;

std::string timestr;
std::string data_validity;
std::string lat, lon;
double speed_kph = 0;
double speed_knots = 0;
sensors_event_t a, g, temp;

double max_acc_x_p = 0;
double max_acc_y_p = 0;
double max_acc_z_p = 0;
double max_acc_x_n = 0;
double max_acc_y_n = 0;
double max_acc_z_n = 0;
int max_speed_int = 0;
bool update_max_accels = true;
bool update_timings = true;

float from_0_to_100 = 0;
float from_80_to_100 = 0;
float from_80_to_120 = 0;
bool reset_values = false;

void reset_stored_values() {
  max_acc_x_p = 0;
  max_acc_y_p = 0;
  max_acc_z_p = 0;
  max_acc_x_n = 0;
  max_acc_y_n = 0;
  max_acc_z_n = 0;
  max_speed_int = 0;
  update_max_accels = true;
  update_timings = true;
  from_0_to_100 = 0;
  from_80_to_100 = 0;
  from_80_to_120 = 0;
}

void parse_vtg(const std::string &sentence) {
  std::stringstream test(sentence);
  std::string segment;

  int i = 0;

  double tmp_speed_kph = 0;

  while (std::getline(test, segment, ',')) {
    if ((i == 7) && (!segment.empty())) {
      // Serial.print("Speed ");
      // Serial.println(segment.c_str());
      tmp_speed_kph = std::stod(segment);

    } else if ((i == 9) && (!segment.empty())) {
      data_validity = segment[0];
      if (((data_validity == "A") || (data_validity == "D")) &&
          (tmp_speed_kph > 0) && (tmp_speed_kph < 360)) {

        speed_kph = tmp_speed_kph;
      } else {
        // Serial.print("DATA no valid: ");
        // Serial.println(data_validity.c_str());
        speed_kph = 0;
      }
      return;
    }
    ++i;
  }
}

void parse_gll(const std::string &sentence) {

  std::stringstream test(sentence);
  std::string segment;

  int i = 0;

  while (std::getline(test, segment, ',')) {
    if ((i == 5) && (!segment.empty())) {
      timestr = segment;
    }

    if ((i == 6) && (!segment.empty())) {
      data_validity = segment;

      return;
    }

    if ((i == 1) && (!segment.empty())) {
      lat = segment;
    }

    if ((i == 2) && (!segment.empty())) {
      lat += segment;
    }

    if ((i == 3) && (!segment.empty())) {
      lon = segment;
    }

    if ((i == 4) && (!segment.empty())) {
      lon += segment;
    }
    ++i;
  }
}

void parse_rmc(const std::string &sentence) {

  std::stringstream test(sentence);
  std::string segment;

  int i = 0;

  // Serial.println("PARSING RMC");
  while (std::getline(test, segment, ',')) {
    if ((i == 1) && (!segment.empty())) {
      timestr = segment;
    }

    if ((i == 2) && (!segment.empty())) {
      data_validity = segment;
    }

    if ((i == 3) && (!segment.empty())) {
      lat = segment;
    }

    if ((i == 4) && (!segment.empty())) {
      lat += segment;
    }

    if ((i == 5) && (!segment.empty())) {
      lon = segment;
    }

    if ((i == 6) && (!segment.empty())) {
      lon += segment;
    }

    if ((i == 7) && (!segment.empty())) {
      speed_knots = std::stod(segment);
      // Serial.println("PARSED OK");
      return;
    }
    ++i;
  }
}

void parse_nmea_sentence(const std::string &sentence) {

  if (sentence.length() < 16) {
    Serial.println("SENTENCE NOT VALID");
    return;
  }

  std::string prefix = sentence.substr(3, 3);

  if (prefix == "VTG") {
    // Serial.println("VTG");
    parse_vtg(sentence);
  } else if (prefix == "GLL") {
    // Serial.println("GLL");
    parse_gll(sentence);
  } else if (prefix == "RMC") {
    // Serial.println("RMC");
    parse_rmc(sentence);
  } else {
    // Serial.print("Sentence cannot be parsed: '");
    // Serial.print(sentence.c_str());
    // Serial.println("'");
  }
}

int nmea0183_checksum(char *nmea_data) {
  int crc = 0;
  int i;

  // the first $ sign and the last two bytes of original CRC + the * sign
  for (i = 1; i < strlen(nmea_data) - 3; i++) {
    crc ^= nmea_data[i];
  }

  return crc;
}

static const int RXPin = 6, TXPin = 5;
static const int SDAPin = 12, SCLPin = 13;

static const uint32_t GPSBaud = 9600;

// The serial connection to the GPS module
// SoftwareSerial gpsSerial(RXPin, TXPin);
const char *configure_gll = "$PUBX,40,GLL,1,0,0,0,0,0*5D\r\n";
const char *configure_gps = "$PUBX,41,1,0007,0003,19200,0*25\r\n";
const char *cfg_230400 = "$PUBX,41,1,0007,0003,230400,0*1A\r\n";
const char *cfg_115200 = "$PUBX,41,1,0007,0003,115200,0*18\r\n";
const char *cfg_38400 = "$PUBX,41,1,0007,0003,38400,0*24\r\n";

const char *disable_gsv = "$PUBX,40,GSV,1,0,0,0,0,0*58\r\n";
const char *disable_gga = "$PUBX,40,GGA,1,0,0,0,0,0*5B\r\n";
const char *disable_gsa = "$PUBX,40,GSA,1,0,0,0,0,0*4F\r\n";
const char *disable_gll = "$PUBX,40,GLL,1,0,0,0,0,0*5D\r\n";

const unsigned char ubxRate10Hz[] PROGMEM = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 100, 0x00, 0x01, 0x00, 0x01, 0x00};

const unsigned char ubxRate16Hz[] PROGMEM = {0x06, 0x08, 0x06, 0x00, 50,
                                             0x00, 0x01, 0x00, 0x01, 0x00};

void setup_tft() {
  spi.begin(TFT_SLK, TFT_MISO, TFT_MOSI, TFT_CS);
  Serial.println("SPI init");

  Serial.println("TFT2");

  tft.begin();
  Serial.println("BEGIN");
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);

  tft.setCursor(250, 230);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_BLACK, CLEAR_COLOR);
  tft.print("CARACC 0.1");
}

void setup_mpu() {
  Serial.println("Adafruit MPU6050 test!");
  Wire.begin(SDAPin, SCLPin);
  Wire.beginTransmission(0x68);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  // Try to initialize!
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("Failed to find MPU6050 chip");

    delay(1000);
    Serial.println("Failed to find MPU6050 chip");
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
}

void setup() {
  Serial.begin(115200);
  setup_tft();
  setup_mpu();
  Serial.println("Wait 10S to reset");
  Serial1.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);
  delay(10000);

  // Disable non needed messages
  Serial1.println(disable_gsv);
  delay(199);
  Serial1.println(disable_gsa);
  delay(199);
  Serial1.println(disable_gga);
  delay(199);
  Serial1.println(disable_gll);
  delay(199);

  /*delay(100);
  Serial.println(F("$PUBX,40,RMC,0,0,0,0*47")); //RMC OFF
  delay(100);
  Serial.println(F("$PUBX,40,VTG,0,0,0,0*5E")); //VTG OFF
  delay(100);
  Serial.println(F("$PUBX,40,GGA,0,0,0,0*5A")); //GGA OFF
  delay(100);
  Serial.println(F("$PUBX,40,GSA,0,0,0,0*4E")); //GSA OFF
  delay(100);
  Serial.println(F("$PUBX,40,GSV,0,0,0,0*59")); //GSV OFF
  delay(100);
  Serial.println(F("$PUBX,40,GLL,0,0,0,0*5C")); //GLL OFF
  //Serial1.println(F("$PUBX,40,GLL,1,0,0,0,0,0*5D")); //GLL ON
  delay(100);*/

  /*Serial1.println(configure_gll);
  delay(250);*/

  Serial1.println(cfg_115200);
  delay(1000);
  Serial1.begin(115200, SERIAL_8N1, RXPin, TXPin);
  delay(1000);
  Serial.println("Setting 10Hz");
  Serial1.write(ubxRate10Hz, sizeof(ubxRate10Hz));

  Serial.println("READY!");
  draw_skin();

  /*for(int i = 0; i < sizeof(ubxRate10Hz); i++)
  {
    Serial1.write(ubxRate10Hz[i]);
  }*/
}

std::string sentence;
bool initFound = false;
bool endFound = false;
bool not_in_sync = false;

void get_accel() {

  mpu.getEvent(&a, &g, &temp);

  static int i = 0;

  i++;

  // if(i>1){
  /*Serial.print("Acceleration X: ");
Serial.print(a.acceleration.x);
Serial.print(", Y: ");
Serial.print(a.acceleration.y);
Serial.print(", Z: ");
Serial.print(a.acceleration.z);
Serial.println(" m/s^2");*/

  if (a.acceleration.x > max_acc_x_p) {
    max_acc_x_p = a.acceleration.x;
    update_max_accels = true;
  }

  if (a.acceleration.x < max_acc_x_n) {
    max_acc_x_n = a.acceleration.x;
    update_max_accels = true;
  }

  if (a.acceleration.y > max_acc_y_p) {
    max_acc_y_p = a.acceleration.y;
    update_max_accels = true;
  }

  if (a.acceleration.y < max_acc_y_n) {
    max_acc_y_n = a.acceleration.y;
    update_max_accels = true;
  }

  if (a.acceleration.z > max_acc_z_p) {
    max_acc_z_p = a.acceleration.z;
    update_max_accels = true;
  }

  if (a.acceleration.z < max_acc_z_n) {
    max_acc_z_n = a.acceleration.z;
    update_max_accels = true;
  }

  /*Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");*/

  /*i = 0;
  }*/


}

void draw_skin() {
  tft.setTextSize(2);
  tft.setCursor(120, 10);
  tft.print("Max v");
  tft.drawLine(120, 25, 230, 25, ILI9341_BLUE);

  tft.setCursor(120, 30);
  tft.print("0-100");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 45, 230, 45, ILI9341_BLUE);

  tft.setCursor(120, 50);
  tft.print("80-100");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 65, 230, 65, ILI9341_BLUE);
  tft.setCursor(120, 70);
  tft.print("80-120");
  tft.setTextSize(1);
  tft.print("km/h");
  tft.setTextSize(2);
  tft.drawLine(120, 85, 230, 85, ILI9341_BLUE);
}

void draw_circle_acc() {
  tft.fillRect(160, 160, 30, 30, CLEAR_COLOR);
  tft.drawCircle(190, 190, 30, ILI9341_RED);
  tft.drawCircle(190, 190, 20, ILI9341_RED);
  tft.drawCircle(190, 190, 10, ILI9341_RED);
  tft.fillCircle(190, 190, 3, ILI9341_RED);
}

void draw_tft() {

  // tft.fillScreen(ILI9341_WHITE);
  tft.fillRect(5, 10, 105, 45, CLEAR_COLOR);
  tft.setTextColor(BLACK);
  tft.setCursor(5, 10);
  tft.setTextSize(6);
  tft.setTextColor(ILI9341_BLACK, CLEAR_COLOR);

  int speed_int = lround(speed_kph);
  tft.print(speed_int);
  if (speed_int > max_speed_int) {
    update_timings = true;

    max_speed_int = speed_int;
  }

  if (update_timings) {

    tft.fillRect(230, 10, 100, 80, CLEAR_COLOR);

    tft.setTextSize(2);
    tft.setCursor(230, 10);
    tft.setTextColor(ILI9341_RED);
    tft.print(max_speed_int);
    tft.print("km/h");
    tft.setTextColor(ILI9341_BLACK);

    tft.setTextSize(2);
    tft.setCursor(230, 30);
    tft.setTextColor(ILI9341_RED);
    tft.print(from_0_to_100);
    tft.print("s");

    tft.setCursor(230, 50);
    tft.setTextColor(ILI9341_RED);
    tft.print(from_80_to_100);
    tft.print("s");

    tft.setCursor(230, 70);
    tft.setTextColor(ILI9341_RED);
    tft.print(from_80_to_120);
    tft.print("s");

    tft.setTextColor(ILI9341_BLACK);
    update_timings = false;
  }

  // tft.setTextSize(3);
  // tft.println(" KM/H");

  /*tft.setCursor(5, 60);
  tft.setTextSize(2);
  tft.println(lat.c_str());
  tft.setCursor(5, 80);
  tft.println(lon.c_str());*/
  tft.fillRect(5, 100, 110, 75, CLEAR_COLOR);
  tft.setTextSize(3);
  tft.setCursor(5, 100);
  tft.print(a.acceleration.x);
  tft.setCursor(5, 125);
  tft.print(a.acceleration.y);
  tft.setCursor(5, 150);
  tft.print(a.acceleration.z);

  if (update_max_accels) {
    tft.fillRect(120, 100, 200, 60, CLEAR_COLOR);
    tft.setTextSize(2);
    tft.setCursor(120, 100);
    tft.print(max_acc_x_p);
    tft.setCursor(230, 100);
    tft.print(max_acc_x_n);

    tft.setCursor(120, 120);
    tft.print(max_acc_y_p);
    tft.setCursor(230, 120);
    tft.print(max_acc_y_n);

    tft.setCursor(120, 140);
    tft.print(max_acc_z_p);
    tft.setCursor(230, 140);
    tft.print(max_acc_z_n);
    update_max_accels = false;
  }

  tft.setTextSize(2);
  tft.setCursor(5, 215);

  if ((data_validity == "A") || (data_validity == "D")) {
    tft.setTextColor(GREEN);

  } else {
    tft.setTextColor(RED);
  }
  tft.fillRect(5, 215, 100, 15, CLEAR_COLOR);
  tft.println(timestr.c_str());
  draw_circle_acc();
}

void loop() {

  /*static bool configured = false;
  if (!configured)
  {
        int i = strlen(configure_gll);
        for(i = 0; i < strlen(configure_gll); i++)
        {
          Serial1.write(configure_gll[i]);
          Serial.write(configure_gll[i]);
        }

        Serial.println("TOTAL");
        Serial.println(i);
          Serial.println("DONE");
          configured=true;
  }*/

  /*

    if (Serial.available()) {      // If anything comes in Serial (USB),
      Serial1.write(Serial.read());   // read it and send it out Serial1 (pins 0
    & 1)
    }

    if (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)
      Serial.write(Serial1.read());   // read it and send it out Serial (USB)
    }
  */
  if (reset_values) {
    reset_stored_values();
    reset_values = false;
  }

  if (Serial1.available()) { // If anything comes in Serial1 (pins 0 & 1)
    char gpsChar = Serial1.read();
    // Serial.print(gpsChar);
    if (gpsChar == '$') {
      // Serial.println("Init of sentence");
      initFound = true;
      endFound = false;
      sentence = gpsChar;
    }

    // consume until end
    while (initFound && !endFound) {
      if (Serial1.available()) {
        char gpsChar = Serial1.read();
        // Serial.print(gpsChar);
        if (initFound) {
          sentence += gpsChar;
        }
        if (gpsChar == '\n') {
          // Serial.println("End of sentence");
          endFound = initFound;
          break;
        }
      }
    }

    if (endFound) {
      // Serial.print(">");
      // Serial.println(sentence.c_str());
      parse_nmea_sentence(sentence);
      initFound = false;
      endFound = false;
      sentence = "";
      /*Serial.print(" Time: ");
      Serial.println(timestr.c_str());
      Serial.print(" Status: ");

       Serial.println(data_validity.c_str());
       Serial.print(" Lat,Lon: ");
       Serial.print(lat.c_str());
       Serial.print(",");
       Serial.println(lon.c_str());
       Serial.print(" Speed knots: ");
       Serial.println(speed_knots);*/
    }

    // Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  }

  get_accel();

  uint32_t now = millis();
  if ((now - last_draw) >= 33) {
    //draw TFT
    draw_tft();
    last_draw = now;
  }

  /* while (Serial1.available() > 0){
     // get the byte data from the GPS
     byte gpsData = Serial1.read();
     Serial.write(gpsData);
   }*/
}
