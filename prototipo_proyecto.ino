#include "ESP32Servo.h"
#include "NewPing.h"

const int ledPin_R = 27;
const int ledPin_G = 26;
const int ledPin_B = 25;

const int Servo_PWM = 13;

const int trigger_1 = 14;
//const int trigger_2 = 26;

const int echo_1 = 12;
//const int echo_2 = 27;

Servo MG995_Servo;
NewPing sonarTank(trigger_1, echo_1, 200);
//NewPing sonarPetDetection(trigger_2, echo_2, 50);

unsigned long tank_distance = 0;
unsigned long pet_distance = 0;

void setup() 
{ 
  Serial.begin(115200); // Initialize UART with 115200 Baud rate  

  // configure LED pins functionalitites
  pinMode(ledPin_R, OUTPUT);
  pinMode(ledPin_G, OUTPUT);
  pinMode(ledPin_B, OUTPUT);

  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 0, 0); // OFF

  MG995_Servo.attach(Servo_PWM);
}

void loop() 
{
  MG995_Servo.write(0); //Turn clockwise at high speed
  delay(2000);
  
  MG995_Servo.detach();//Stop. You can use deatch function or use write(x), as x is the middle of 0-180 which is 90, but some lack of precision may change this value
  delay(500);
  MG995_Servo.attach(Servo_PWM);//Always use attach function after detach to re-connect your servo with the board
  
  MG995_Servo.write(180);
  delay(2000);
  MG995_Servo.detach();//Stop
  delay(500);
  MG995_Servo.attach(Servo_PWM);
  
  tank_distance = sonarTank.ping_cm(); 
  //pet_distance = sonarPetDetection.ping_cm();
  
  Serial.print("Tank distance ");
  Serial.print(tank_distance);
  Serial.println("cm");

/*
  Serial.print("Detected pet at ");
  Serial.print(pet_distance);
  Serial.println("cm");
*/
  if(tank_distance >= 66)
  {
    RGB_color(255, 0, 0);
  }
  else if(tank_distance >= 33)
  {
    RGB_color(255, 255, 0);    
  }
  else if (tank_distance > 0)
  {
    RGB_color(0, 255, 0);  
  }
  else
  {
    RGB_color(0, 0, 255);
  }
}

void RGB_color(int R, int G, int B)
 {
  analogWrite(ledPin_R, R);
  analogWrite(ledPin_G, G);
  analogWrite(ledPin_B, B);
}
