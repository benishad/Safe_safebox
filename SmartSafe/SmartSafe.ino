#include <SoftwareSerial.h>
#include <MFRC522.h>            // "툴 - 라이브러리 관리" 에서 MFRC522 검색 후 설치
#include <Servo.h>              // 첨부한 파일로 라이브러리 추가
#include <SPI.h>
#include "I2Cdev.h"
#include "MPU6050.h"

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define OUTPUT_READABLE_ACCELGYRO

SoftwareSerial SerialBT(2, 3);

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

byte nuidPICC[4];

Servo servo;

byte SoundPin = A0;
byte RedPin = 4;
byte GreenPin = 5;
byte ServoPin = 6;
byte PirPin = 7;
byte BuzzerPin = 8;

byte LockingState = 0;

unsigned long prevMillis = 0;

void setup() {
  // join I2C bus (I2Cdev library doesn't do this automatically)
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
  Wire.begin();
#elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
  Fastwire::setup(400, true);
#endif

  Serial.begin(9600);
  SerialBT.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(RedPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(PirPin, INPUT);
  pinMode(BuzzerPin, OUTPUT);
  setLocked(true);
}

void loop() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // these methods (and a few others) are also available
  //accelgyro.getAcceleration(&ax, &ay, &az);
  //accelgyro.getRotation(&gx, &gy, &gz);

#ifdef OUTPUT_READABLE_ACCELGYRO
  // display tab-separated accel/gyro x/y/z values
  Serial.print("a/g:\t");
  Serial.print(ax); Serial.print("\t");
  Serial.print(ay); Serial.print("\t");
  Serial.print(az); Serial.print("\t");
  Serial.print(gx); Serial.print("\t");
  Serial.print(gy); Serial.print("\t");
  Serial.println(gz);
  
#endif

  if (SerialBT.available()) {
    switch (SerialBT.read()) {
      case 'A':
        if (LockingState == 0)
        {
          setLocked(false);
          LockingState++;
        } else {
          setLocked(true);
          LockingState = 0;
        }
        break;
      default:
        break;
    }
  }
  
  if ((analogRead(SoundPin) > 200 || digitalRead(PirPin) == HIGH) || ((ax > 5000 || ax < -5000) || (ay > 5000 || ay < -5000))) {
    unsigned long currentMillis = millis();
    if (currentMillis - prevMillis > 1000) {
      SerialBT.print("B");
      prevMillis = currentMillis;

      digitalWrite(BuzzerPin, HIGH);
      delay(100);
      digitalWrite(BuzzerPin, LOW);
      delay(100);
      digitalWrite(BuzzerPin, HIGH);
      delay(100);
      digitalWrite(BuzzerPin, LOW);
      delay(100);
    }
  }
  nfc();
}

void nfc() {
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  if ( ! rfid.PICC_ReadCardSerial())
    return;

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
    if (LockingState == 0)
    {
      setLocked(false);
      LockingState++;
    } else {
      setLocked(true);
      LockingState = 0;
    }
  }
  else {
    if (LockingState == 0)
    {
      setLocked(false);
      LockingState++;
    } else {
      setLocked(true);
      LockingState = 0;
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void setLocked(int locked) {
  if (locked) {
    SerialBT.print("D");
    digitalWrite(RedPin, HIGH);
    digitalWrite(GreenPin, LOW);
    digitalWrite(BuzzerPin, HIGH);
    delay(100);
    digitalWrite(BuzzerPin, LOW);
    delay(100);
    servo.attach(ServoPin);
    servo.write(0);
    delay(1000);
    servo.detach();
  } else {
    SerialBT.print("C");
    digitalWrite(RedPin, LOW);
    digitalWrite(GreenPin, HIGH);
    digitalWrite(BuzzerPin, HIGH);
    delay(100);
    digitalWrite(BuzzerPin, LOW);
    delay(100);
    servo.attach(ServoPin);
    servo.write(90);
    delay(1000);
    servo.detach();
  }
}
