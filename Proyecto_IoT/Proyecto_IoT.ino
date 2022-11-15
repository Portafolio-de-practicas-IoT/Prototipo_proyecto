#include <Arduino.h>

/****** 
 * Include Libraries 
 ******/ 
#include "ESP32Servo.h"
#include "NewPing.h"
#include "time.h"
#include <WiFi.h> 
#include <Firebase_ESP_Client.h>
/* #include <WiFiClient.h>
#include <HTTPClient.h> */

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include "ArduinoJson.h"

/****** 
 * Define Constants 
 ******/

/* WIFI loggin*/
#define WIFI_SSID "TestWiFi" //WIFI SSID aqui 
#define WIFI_PASSWORD "1234567890" // WIFI pwd 

/* Firebase API KEY */
#define FIREBASE_API_KEY "AIzaSyBJpluQ6Prg1NqHdzxBpjJ-n3LGu-lF5Z8"
#define FIREBASE_PROJECT_ID "iot-app-1f19f"

/* Firebase user loggin */
#define USER_EMAIL "iot.feeder.3@gmail.com"
#define USER_PASSWORD "1234567890"

/* Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://espdemo-4765e-default-rtdb.firebaseio.com/"

#define JSONDOCSIZE 384 /* Calculated from https://arduinojson.org/v6/assistant/#/step3 */

/****** 
 * Global variables 
 ******/

/* Firebase objects */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* User ID */
String uid;

/* Database main path (to be updated in setup with the user UID) */
String databasePath;

/* Database child nodes */
String tankPath = "/tankLevel";
String detectPetPath = "/petDetectedAt";
/* String presPath = "/pressure"; */
String timePath = "/timestamp";

String dbPath_1 = "/UsersData/";
String dbPath_2 = "/readings";

/* Parent Node (to be updated in every loop) */
String parentPath;

int timestamp;
FirebaseJson fbJson;

const char* ntpServer = "pool.ntp.org";

const int Servo_PWM = 13;

const int trigger_1 = 12;
const int trigger_2 = 32;

const int echo_1 = 14;
const int echo_2 = 33;

Servo MG995_Servo;
NewPing sonarTank(trigger_1, echo_1, 200);
NewPing sonarPetDetection(trigger_2, echo_2, 40);

unsigned long tank_distance = 0;
unsigned long pet_distance = 0;

/* RGB LED pins */
const int ledPin_R = 27;
const int ledPin_G = 26;
const int ledPin_B = 25;

/* Timer variables (send new readings every three minutes) */
unsigned long sendDataPrevMillis = 0;
unsigned long getDataPrevMillis = 0;
unsigned long timerDelay = 30000;

bool taskcomplete = false;
int count = 0;
int values_count = 0;
int prev_distance = 0;
int current_distance = 0;
bool already_feed = false;

/****** 
 * Auxiliar Functions
 ******/ 

 /* setTankLevelPayload */
/* void setTankLevelPayload(char* payload) 
{
  tank_distance = sonarTank.ping_cm();
  Serial.print("Tank level in cm: ");
  Serial.println(tank_distance); // Imprime temperatura en el serial monitor  
  /* numero maximo 4 precision 2 y convierte el valor a string*/ 
/*   dtostrf(tank_distance, 4, 0, str_tank);
  sprintf(payload, "{\"value\": %s}", str_tank); */

  /* Turn on/off led */
/*   if(tank_distance >= 66)
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
} */

/* setPetDetectPayload */

/*
void setPetDetectPayload(char* payload) 
{
  pet_distance = sonarPetDetection.ping_cm();
  Serial.print("Detected pet at: ");
  Serial.println(pet_distance); // Imprime humedad en el serial monitor  
  dtostrf(pet_distance, 4, 2, str_pet); // numero maximo 4 precision 2 y convierte el valor a string 
  sprintf(payload, "{\"value\": %s}", str_pet);

  // Move motor
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
} */

/* void sendPostRequest(char* url, char* payload) 
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
} */

/* void http_post()
{
  setTankLevelPayload(payload);
  sendPostRequest(TankLevelUrl, payload);
  delay(2000);
  setPetDetectPayload(payload);
  sendPostRequest(DetectPetUrl, payload);
  delay(11000);
} */

void RGB_color(int R, int G, int B)
 {
  analogWrite(ledPin_R, R);
  analogWrite(ledPin_G, G);
  analogWrite(ledPin_B, B);
}

/**
 * It connects to the WiFi network using the SSID and password defined in the `WIFI_SSID` and
 * `WIFI_PASSWORD` constants
 */
void initWiFi() 
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println(""); 
  Serial.println("WiFi Connected"); 
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
  Serial.println();
}

/**
 * This function sets the pin modes for the three pins that are connected to the RGB LED
 */
void initRGBLED()
{
  // configure LED pins functionalitites
  pinMode(ledPin_R, OUTPUT);
  pinMode(ledPin_G, OUTPUT);
  pinMode(ledPin_B, OUTPUT);

  /* RGB LED */
  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 0, 0); // OFF
  delay(500);
  RGB_color(0, 255, 0); // Green
  delay(500);
  RGB_color(0, 0, 0); // OFF
}

void initServo()
{
  /* MG995 servo */
  ESP32PWM::allocateTimer(0); // allocate timer 0
  ESP32PWM::allocateTimer(1); // allocate timer 1
  ESP32PWM::allocateTimer(2); // allocate timer 2
  ESP32PWM::allocateTimer(3); // allocate timer 3

  MG995_Servo.setPeriodHertz(50); // standard 50 hz servo
  MG995_Servo.attach(Servo_PWM, 500, 2500); // attach the servo on pin 13 to the servo object
}

/**
 * It gets the current time from the RTC and converts it to a Unix timestamp
 * 
 * @return The number of seconds since January 1, 1970.
 */
unsigned long getTime() 
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

void firebaseInit()
{
  /* Assign the api key (required) */
  config.api_key = FIREBASE_API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  //config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  // Print user UID
  uid = "lPifXSOAFfM5n2TxD8ir1q21jwi1";
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  /* databasePath = "/UsersData/" + uid + "/readings"; */
  // databasePath = dbPath_1 + uid + dbPath_2;
}

/* Read Sensors */
unsigned long readTankLevel()
{
  tank_distance = sonarTank.ping_cm();
  return tank_distance;
}

unsigned long readPetDetectSensor()
{
  pet_distance = sonarPetDetection.ping_cm();
  return pet_distance;
}

void sendDataToFirestore()
{
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    count++;

    if(count == 60 * 60 * 8)
    {
      already_feed = false;
      count = 0;
    }
    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    //parentPath = databasePath + "/" + String(timestamp);

    /* FireStore */
    String documentPath = "users/" + uid;
    
    // if (!taskcomplete)
    // {
    //     taskcomplete = true;

    //     fbJson.clear();
    //     fbJson.set("fields/food_level/integerValue", String(readTankLevel()));
    //     fbJson.set("fields/water_level/doubleValue", String(readTankLevel()));
    //     fbJson.set("fields/detected_pet/doubleValue", String(readPetDetectSensor()));

    //     Serial.print("Create a document... ");

    //     if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), fbJson.raw()))
    //         Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    //     else
    //         Serial.println(fbdo.errorReason());
    // }

    fbJson.clear();
    fbJson.set("fields/statistics/mapValue/fields/food_level/integerValue", String(readTankLevel()));
    fbJson.set("fields/statistics/mapValue/fields/water_level/integerValue", String(readTankLevel()));
    fbJson.set("fields/statistics/mapValue/fields/detected_pet/integerValue", String(readPetDetectSensor()));

    prev_distance = current_distance;
    current_distance = readPetDetectSensor();

    Serial.println(readTankLevel());
    Serial.println(current_distance);
    Serial.print("Update a document... ");

    /** if updateMask contains the field name that exists in the remote document and
      * this field name does not exist in the document (content), that field will be deleted from remote document
      */

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), fbJson.raw(), "statistics.food_level" /* updateMask */))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), fbJson.raw(), "statistics.water_level" /* updateMask */))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), fbJson.raw(), "statistics.detected_pet" /* updateMask */))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());

    /*
    fbJson.set(tankPath.c_str(), String(readTankLevel()));
    fbJson.set(detectPetPath.c_str(), String(readPetDetectSensor()));
    fbJson.set(timePath, String(timestamp));
    //Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &fbJson) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set json... %s\n", Firebase.FS.setJSON(&fbdo, parentPath.c_str(), &fbJson) ? "ok" : fbdo.errorReason().c_str());
    */
  }
}

bool deserializeFirestoreSoundJson(String input)
{
  // String input;

  StaticJsonDocument<384> doc;

  DeserializationError error = deserializeJson(doc, input);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // const char* name = doc["name"];

  bool fields_sound_booleanValue = doc["fields"]["sound"]["booleanValue"]; // false

  return fields_sound_booleanValue;

  // const char* createTime = doc["createTime"]; // "2022-11-23T00:42:28.782328Z"
  // const char* updateTime = doc["updateTime"]; // "2022-11-23T00:44:35.361031Z"
}

bool deserializeFirestoreFoodJson(String input)
{
  // String input;

  StaticJsonDocument<384> doc;

  DeserializationError error = deserializeJson(doc, input);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return false;
  }

  // const char* name = doc["name"];

  bool fields_food_booleanValue = doc["fields"]["food"]["booleanValue"]; // false

  return fields_food_booleanValue;

  // const char* createTime = doc["createTime"]; // "2022-11-23T00:42:28.782328Z"
  // const char* updateTime = doc["updateTime"]; // "2022-11-23T00:44:35.361031Z"
}

void getDataFromFirestore()
{
  /* Checks each 10 seconds if a new instruction is available */
  if (Firebase.ready() && (millis() - getDataPrevMillis > 10000 || getDataPrevMillis == 0))
  {
      getDataPrevMillis = millis();

      String documentPath = "actions/dXokcLs29nzwDuXljiOG";
      String mask = "food";

      // If the document path contains space e.g. "a b c/d e f"
      // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"

      Serial.print("Get a document... ");

      if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str()))
      {
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        if(deserializeFirestoreFoodJson(fbdo.payload()))
        {
          already_feed = false;
          activateFeed();
        }
      }
      else
          Serial.println(fbdo.errorReason());

      String mask2 = "sound";

      Serial.print("Get a document... ");

      if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask2.c_str()))
      {
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        if(deserializeFirestoreSoundJson(fbdo.payload()))
        {
          /* RGB LED */
          RGB_color(255, 0, 0); // Green
          delay(500);
          RGB_color(0, 0, 0); // OFF
          delay(500);
          RGB_color(255, 0, 0); // Green
          delay(500);
          RGB_color(0, 0, 0); // OFF
        }
      }
      else
          Serial.println(fbdo.errorReason());
      
      if(deserializeFirestoreFoodJson(fbdo.payload()) || deserializeFirestoreSoundJson(fbdo.payload()))
      {
        Serial.print(" *********************   CLEAR ACTIONS ******************** ");
        clearFeederActions();
      }
  }
}

void clearFeederActions()
{
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    //parentPath = databasePath + "/" + String(timestamp);

    /* FireStore */
    String documentPath = "actions/dXokcLs29nzwDuXljiOG";

    fbJson.clear();
    fbJson.set("fields/food/booleanValue", "false");
    fbJson.set("fields/sound/booleanValue", "false");

    Serial.print("Update actions document... ");

    /** if updateMask contains the field name that exists in the remote document and
      * this field name does not exist in the document (content), that field will be deleted from remote document
      */

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), fbJson.raw(), "food,sound" /* updateMask */))
        Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    else
        Serial.println(fbdo.errorReason());
  }
}

void activateFeed()
{
  if(!already_feed)
  {
    Serial.println("Feeding... ");
    MG995_Servo.attach(Servo_PWM);//Always use attach function after detach to re-connect your servo with the board
    MG995_Servo.write(90); //Turn clockwise at high speed
    delay(1000);

    MG995_Servo.write(180); //Turn clockwise at high speed
    delay(1000);

    MG995_Servo.write(90); //Turn clockwise at high speed
    delay(1000);

    MG995_Servo.write(0); //Turn clockwise at high speed
    delay(1000);

    MG995_Servo.detach();//Stop. You can use deatch function or use write(x), as x is the middle of 0-180 which is 90, but some lack of precision may change this value

    already_feed = true;
  }
}

void petIsWaiting()
{
  if((prev_distance > 0 && prev_distance <= 40) && (prev_distance > 0 && prev_distance <= 40))
  {
    activateFeed();
  }
}
  

/* Setup */
void setup() 
{ 
  Serial.begin(115200); // Initialize UART with 115200 Baud rate  

  initWiFi();
  initServo();
  configTime(0, 0, ntpServer);
  firebaseInit();
  initRGBLED();
}

/* Main loop */
void loop() 
{
  sendDataToFirestore();
  getDataFromFirestore();
  petIsWaiting();
}