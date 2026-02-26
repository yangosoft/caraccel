

#include <esp32-hal-gpio.h>
#include <esp32-hal.h>

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();

void setup()
{

  Serial.begin(115200);
  Wire.begin(7, 8);
  mma.begin(0x1C, &Wire);
  // put your setup code here, to run once:

  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  mma.setRange(MMA8451_RANGE_8_G);
  mma.setDataRate(MMA8451_DATARATE_800_HZ);

  Serial.print("Range = ");
  Serial.print(2 << mma.getRange());
  Serial.println("G");
}

void loop()
{

  // put your main code here, to run repeatedly:
  // digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(5, HIGH);
  // digitalWrite(6, HIGH);
  // digitalWrite(7, HIGH);
  delay(10);
  // digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  // digitalWrite(6, LOW);
  // digitalWrite(7, LOW);

  delay(10);
  // Read the 'raw' data in 14-bit counts
  mma.read();
  Serial.print("X:\t");
  Serial.print(mma.x_g);
  Serial.print("\tY:\t");
  Serial.print(mma.y_g);
  Serial.print("\tZ:\t");
  Serial.print(mma.z_g);
  Serial.println();

  /*
    sensors_event_t event;
    mma.getEvent(&event);

      /*Serial.print("X: \t");
    Serial.print(event.acceleration.x);
    Serial.print("\t");
    Serial.print("Y: \t");
    Serial.print(event.acceleration.y);
    Serial.print("\t");
    Serial.print("Z: \t");
    Serial.print(event.acceleration.z);
    Serial.print("\t");
    Serial.println("m/s^2 ");


    uint8_t o = mma.getOrientation();

    switch (o)
    {
    case MMA8451_PL_PUF:
      Serial.println("Portrait Up Front");
      break;
    case MMA8451_PL_PUB:
      Serial.println("Portrait Up Back");
      break;
    case MMA8451_PL_PDF:
      Serial.println("Portrait Down Front");
      break;
    case MMA8451_PL_PDB:
      Serial.println("Portrait Down Back");
      break;
    case MMA8451_PL_LRF:
      Serial.println("Landscape Right Front");
      break;
    case MMA8451_PL_LRB:
      Serial.println("Landscape Right Back");
      break;
    case MMA8451_PL_LLF:
      Serial.println("Landscape Left Front");
      break;
    case MMA8451_PL_LLB:
      Serial.println("Landscape Left Back");
      break;
    }
    Serial.println();*/
}

// put function definitions here:
