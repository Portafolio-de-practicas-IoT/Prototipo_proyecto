/************** 
 * Include Libraries 
 **************/ 
#include "ESP32Servo.h"
#include "NewPing.h"
#include <WiFi.h> 
#include <WiFiClient.h>
#include <HTTPClient.h>

/************** 
 * Define Constants 
 **************/

#define WIFISSID "TestWiFiNet" //WIFI SSID aqui 
#define PASSWORD "1234567890" // WIFI pwd 
#define TOKEN "BBFF-0O3RvdSvbpw5m6FH4Ggz8bzWJNb9tT" // Ubidots TOKEN name el mismo que usamos en clase 

#define VARIABLE_LABEL_temp "temperatura" // Variable Temperatura 
#define VARIABLE_LABEL_hum "humedad" // Variable Humedad 
#define DEVICE_LABEL "esp32" // Nombre del dispositivo a crear 

/************** 
 * Global variables 
 **************/

const int ledPin_R = 27;
const int ledPin_G = 26;
const int ledPin_B = 25;

const int Servo_PWM = 13;

const int trigger_1 = 12;
const int trigger_2 = 32;

const int echo_1 = 14;
const int echo_2 = 33;

// Space to store values to send 
char str_tank[10]; 
char str_pet[10];

char payload[200]; // Leer y entender el payload aqui una de tantas referencias "https://techterms.com/definition/payload"

char TankLevelUrl[] = "http://industrial.api.ubidots.com/api/v1.6/variables/634054128472cb6ff6cb3578/values";
char DetectPetUrl[] = "http://industrial.api.ubidots.com/api/v1.6/variables/63404d8dfe13941d8496d0cc/values";

WiFiClient client;
HTTPClient httpClient;

Servo MG995_Servo;
NewPing sonarTank(trigger_1, echo_1, 200);
NewPing sonarPetDetection(trigger_2, echo_2, 40);

unsigned long tank_distance = 0;
unsigned long pet_distance = 0;

/************** 
 * Funciones auxiliares 
 **************/ 

 /* setTankLevelPayload */
void setTankLevelPayload(char* payload) 
{
  tank_distance = sonarTank.ping_cm();
  Serial.print("Tank level in cm: ");
  Serial.println(tank_distance); // Imprime temperatura en el serial monitor  
  /* numero maximo 4 precision 2 y convierte el valor a string*/ 
  dtostrf(tank_distance, 4, 0, str_tank);
  sprintf(payload, "{\"value\": %s}", str_tank);

  /* Turn on/off led */
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

/* setPetDetectPayload */
void setPetDetectPayload(char* payload) 
{
  pet_distance = sonarPetDetection.ping_cm();
  Serial.print("Detected pet at: ");
  Serial.println(pet_distance); // Imprime humedad en el serial monitor  
  /* numero maximo 4 precision 2 y convierte el valor a string*/ 
  dtostrf(pet_distance, 4, 2, str_pet);
  sprintf(payload, "{\"value\": %s}", str_pet);

  /* Move motor */
  if(pet_distance >= 10 && pet_distance <= 40)
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
  }  
}

void sendPostRequest(char* url, char* payload) 
{
  int httpStatusCode;

  httpClient.begin(client, url);
 
  httpClient.addHeader("Content-Type", "application/json");
  httpClient.addHeader("X-Auth-Token", TOKEN);

  httpStatusCode = httpClient.POST(payload);
  Serial.print("HTPP Status code: ");
  Serial.println(httpStatusCode);
  Serial.print("Send ");
  Serial.println(payload);

  Serial.println(httpClient.getString());

  // Free resources
  httpClient.end();
}

void http_post()
{
  setTankLevelPayload(payload);
  sendPostRequest(TankLevelUrl, payload);
  delay(2000);
  setPetDetectPayload(payload);
  sendPostRequest(DetectPetUrl, payload);
  delay(11000);
}

void RGB_color(int R, int G, int B)
 {
  analogWrite(ledPin_R, R);
  analogWrite(ledPin_G, G);
  analogWrite(ledPin_B, B);
}

/* Setup */

void setup() 
{ 
  Serial.begin(115200); // Initialize UART with 115200 Baud rate  

  // configure LED pins functionalitites
  pinMode(ledPin_R, OUTPUT);
  pinMode(ledPin_G, OUTPUT);
  pinMode(ledPin_B, OUTPUT);

  /* WiFi */
  WiFi.begin(WIFISSID, PASSWORD); 
 
  Serial.println(); 
  Serial.print("Wait for WiFi..."); 
   
  while (WiFi.status() != WL_CONNECTED) 
  { 
    Serial.print("."); 
    delay(500); 
  } 
   
  Serial.println(""); 
  Serial.println("WiFi Connected"); 
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());  

  /* RGB LED */
  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 0, 0); // OFF
  delay(500);
  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 0, 0); // OFF

  /* MG995 servo */
  ESP32PWM::allocateTimer(0); // allocate timer 0
  ESP32PWM::allocateTimer(1); // allocate timer 1
  ESP32PWM::allocateTimer(2); // allocate timer 2
  ESP32PWM::allocateTimer(3); // allocate timer 3

  MG995_Servo.setPeriodHertz(50); // standard 50 hz servo
  MG995_Servo.attach(Servo_PWM, 500, 2500); // attach the servo on pin 13 to the servo object
}

/* Main loop */
void loop() 
{
  http_post();
}
