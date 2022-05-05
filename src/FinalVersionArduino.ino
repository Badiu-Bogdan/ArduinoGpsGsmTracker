#define TINY_GSM_MODEM_SIM800

#include <Arduino.h>
#include <TinyGPS++.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <SoftwareSerial.h>

const char FIREBASE_HOST[] = "lucraredelicenta-a5e0f-default-rtdb.firebaseio.com";
const String FIREBASE_AUTH = "Te4mbS0km7YuGD2hixTBHqSbRu0hQIKuIYw343cQ";
const String FIREBASE_PATH = "/";
const int SSL_PORT = 443;

char apn[] = "live.vodafone.com";
char user[] = "live";
char pass[] = "";

SoftwareSerial sim800(10, 11);
SoftwareSerial SerialGps(8,9);
TinyGsm modem(sim800);
TinyGPSPlus gps;

TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);

void initGsmModule()
{
    Serial.println(F("Sim module initializing."));
    sim800.begin(9600);
    modem.init();
    String modemInfo = modem.getModemInfo();
    Serial.println(modemInfo);
    Serial.println(F("Sim module initialized."));
}
void setup() {

  Serial.begin(9600);
  while(!Serial)
  {

  }
  initGsmModule();
  SerialGps.begin(9600);
  sim800.listen();

  http_client.setHttpResponseTimeout(9*1000);
}

void postToFirebase(const char* method, const String &path, const String &data, HttpClient* http)
{
  sim800.listen();
  String response;
  int statusCode = 0;
  http->connectionKeepAlive(); // Currently, this is needed for HTTPS
  
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  String url;
  if (path[0] != '/') {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=" + FIREBASE_AUTH;
  Serial.print("POST:");
  Serial.println(url);
  Serial.print("Data:");
  Serial.println(data);
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  
  String contentType = "application/json";
  http->put(url, contentType, data);
  
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  // read the status code and body of the response
  //statusCode-200 (OK) | statusCode -3 (TimeOut)
  statusCode = http->responseStatusCode();
  Serial.print(F("Status code: "));
  Serial.println(statusCode);
  response = http->responseBody();
  Serial.print(F("Response: "));
  Serial.println(response);
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN

  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  if (!http->connected()) {
    Serial.println();
    http->stop();// Shutdown
    Serial.println(F("HTTP POST disconnected"));
  }
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  
}

void gpsloop()
{
  //NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
  //Can take up to 60 seconds
  SerialGps.listen();
  boolean newData = true;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (SerialGps.available()){
      if (gps.encode(SerialGps.read())){
        newData = true;
        break;
      }
    }
  }
  
  if(true){
  newData = false;
  
  String latitude, longitude;
  //float altitude;
  //unsigned long date, time, speed, satellites;
  
  latitude = String(gps.location.lat(), 6); // Latitude in degrees (double)
  longitude = String(gps.location.lng(), 6); // Longitude in degrees (double)
  
  //altitude = gps.altitude.meters(); // Altitude in meters (double)
  //date = gps.date.value(); // Raw date in DDMMYY format (u32)
  //time = gps.time.value(); // Raw time in HHMMSSCC format (u32)
  //speed = gps.speed.kmph();
  
  Serial.print(F("Latitude= ")); 
  Serial.print(latitude);
  Serial.print(F(" Longitude= ")); 
  Serial.println(longitude);
      
  String gpsData = "{";
  gpsData += "\"lat\":" + latitude + ",";
  gpsData += "\"lng\":" + longitude + "";
  gpsData += "}";

  //PUT   Write or replace data to a defined path, like messages/users/user1/<data>
  //PATCH   Update some of the keys for a defined path without replacing all of the data.
  //POST  Add to a list of data in our Firebase database. Every time we send a POST request, the Firebase client generates a unique key, like messages/users/<unique-id>/<data>
  //https://firebase.google.com/docs/database/rest/save-data
  
  postToFirebase("PATCH", FIREBASE_PATH, gpsData, &http_client);
  

  }
}
void loop() {
  //Waiting so that sim800 has internet disponibility
  sim800.listen();
  Serial.println(F("Waiting for network..."));
  if(!modem.waitForNetwork())
  {
    Serial.println(F("fail"));
    delay(100); //inainte 1000
    return;
  }
  Serial.println(F("OK"));

  //Connecting to internet
  Serial.println(F("Connecting to "));
  Serial.print(apn);
  if(!modem.gprsConnect(apn, user, pass))
  {
    Serial.println(F("fail"));
    delay(100); //inainte 1000
    return;
  }
  Serial.println(F("OK"));

  http_client.connect(FIREBASE_HOST, SSL_PORT);
  
  while(true)
  {
    if(!http_client.connected())
    {
      Serial.println();
      http_client.stop();
      Serial.println(F("HTTP not connected"));
      break;
    }
    else{
      gpsloop();
    }
  }

}
