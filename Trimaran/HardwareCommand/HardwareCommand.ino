/*******************************************
 * Motor Shield 1-Channel DC Motor + Servo
 * by DeRoBat
 ******************************************/

#include <Servo.h>

Servo servo1;
Servo servo2;


int Freq;
char msg[10];
int cnt=0;
int val;
int Angle;
int Speed;


void setup() {
  
  Serial.begin(115200);
  Serial.println(" En attente ");

  servo1.attach(0);
  servo2.attach(5);
}


void loop() {
    
   while (Serial.available() > 0 ){
      msg[cnt] = Serial.read();
      cnt ++;
      
      if (cnt == 4) {
        val = atoi(msg);
        cnt = 0;
      }
                      
      if (val >= 1000 && val < 1255){
        Angle = val - 1000;
        servo1.write(Angle);
      }
      
      if (val >= 2000 && val < 2255){
	Angle = val - 2000;
        servo1.write(Angle);
      }
   }
}   
