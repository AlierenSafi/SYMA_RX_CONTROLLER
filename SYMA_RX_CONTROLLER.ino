#include <Servo.h>
Servo servom;

#include <eRCaGuy_Timer2_Counter.h>

#include <SPI.h>
#include <symax_protocol.h>

int ppmPin = 7;
long ustLimit = 255;
long altLimit = 0;
uint8_t value;
int i = 0;
int h = 0;
nrf24l01p wireless;
symaxProtocol protocol;

unsigned long time = 0;
long dizi[11];

void setup() {
  timer2.setup();
  Serial.begin(115200);
  servom.attach(7);

  // SS pin must be set as output to set SPI to master !
  pinMode(5, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  pinMode(SS, OUTPUT);
  pinMode(ppmPin, OUTPUT);
  digitalWrite(ppmPin, LOW);

  // Set CE pin to 10 and CS pin to 9
  wireless.setPins(10, 9);

  // Set power (PWRLOW,PWRMEDIUM,PWRHIGH,PWRMAX)
  wireless.setPwr(PWRLOW);

  protocol.init(&wireless);

  time = micros();
  Serial.println("Basladi");

}

rx_values_t rxValues;

unsigned long newTime;

void loop()
{
  time = micros();
  value = protocol.run(&rxValues);
  newTime = micros();

  switch ( value )
  {
    case  NOT_BOUND:
      Serial.println("Eslesme Yok");
      Serial.write('X');
      break;

    case  BIND_IN_PROGRESS:
      Serial.println("Baglanti islemede");
      Serial.write('B');
      break;

    case BOUND_NEW_VALUES:
      //Serial.print(newTime - time);


      //      dizi[0] = (rxValues.throttle);
      //      dizi[1] = (rxValues.yaw);
      //      dizi[2] = (rxValues.pitch);
      //      dizi[3] = (rxValues.roll);
      //      dizi[4] = (rxValues.trim_yaw);
      //      dizi[5] = (rxValues.trim_pitch);
      //      dizi[6] = (rxValues.trim_roll);
      //      dizi[7] = (rxValues.video);
      //      dizi[8] = (rxValues.picture);
      //      dizi[9] = (rxValues.highspeed);
      //      dizi[10] = (rxValues.flip);



      dizi[0] = map(rxValues.throttle, 0, 255, altLimit, ustLimit);
      dizi[1] = map(rxValues.yaw, 127, -127, altLimit, ustLimit);
      dizi[2] = map(rxValues.pitch, -127, 127, altLimit, ustLimit);
      dizi[3] = map(rxValues.roll, 127, -127, altLimit, ustLimit);
      dizi[4] = map(rxValues.trim_yaw, 31, -31, altLimit, ustLimit);
      dizi[5] = map(rxValues.trim_pitch, -31, 31, altLimit, ustLimit);
      dizi[6] = map(rxValues.trim_roll, 31, -31, altLimit, ustLimit);
      dizi[7] = map(rxValues.video, 0, 1, altLimit, ustLimit);
      dizi[8] = map(rxValues.picture, 0, 1, altLimit, ustLimit);
      dizi[9] = map(rxValues.highspeed, 0, 1, altLimit, ustLimit);
      dizi[10] = map(rxValues.flip, 0, 1, altLimit, ustLimit);


      h = map(rxValues.throttle, 0, 255, 1000, 2000);
      servom.writeMicroseconds(h);


      //digitalWrite(5, ((dizi[0] > 250 && dizi[3] > 250) ? 1 : 0));
      
      if (rxValues.picture == 1)i += 1;
      if (i > 3)i = 0;
      servom.write(map(dizi[i], altLimit, ustLimit, 0, 180));

      //      if (dizi[0] > 250) {
      //        digitalWrite(5, HIGH);
      //      }
      //      else {
      //        digitalWrite(5, LOW);
      //      }


      Serial.write('R');
      for (int i = 0; i < 11; i++) {
        Serial.write(dizi[i]);
      }



      //      ppmGonder(ppmPin, 6000);
      //      for (int i = 0; i <= 10; i++) {
      //        ppmGonder(ppmPin, dizi[i]);
      //      }


      //      Serial.print(" :\t"); Serial.print(dizi[0]);
      //      for (int i = 1; i <= 9; i++) {
      //        Serial.print("\t"); Serial.print(dizi[i]);
      //      }
      //      Serial.print("\t"); Serial.println(dizi[10]);

      //      Serial.print("\t"); Serial.print(rxValues.throttle);
      //      Serial.print("\t"); Serial.print(rxValues.yaw);
      //      Serial.print("\t"); Serial.print(rxValues.pitch);
      //      Serial.print("\t"); Serial.print(rxValues.roll);
      //      Serial.print("\t"); Serial.print(rxValues.trim_yaw);
      //      Serial.print("\t"); Serial.print(rxValues.trim_pitch);
      //      Serial.print("\t"); Serial.print(rxValues.trim_roll);
      //      Serial.print("\t"); Serial.print(rxValues.video);
      //      Serial.print("\t"); Serial.print(rxValues.picture);
      //      Serial.print("\t"); Serial.print(rxValues.highspeed);
      //      Serial.print("\t"); Serial.println(rxValues.flip);
      //      time = newTime;

      break;

    case BOUND_NO_VALUES:

      break;

    default:
      break;

  }

}

void ppmGonder(int a, int b) {
  digitalWrite(a, HIGH);
  delayMicroseconds(b);
  digitalWrite(a, LOW);
}


