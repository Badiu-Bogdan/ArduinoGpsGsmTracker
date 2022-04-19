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

int latitudine, longitudine;

int rxPin = 2;
int txPin = 3;

SoftwareSerial sim800(txPin, rxPin);
TinyGsm modem(sim800);

SoftwareSerial neogps(10, 11);
TinyGPSPlus gps;

TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);

void setup() {

  Serial.begin(115200);
  Serial.println("Sim module initialized.");
  sim800.begin(9600);

  neogps.begin(9600);
  Serial.println("NeoGps serial initialized.");

  //Starting the gsm module.
  modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.println(modemInfo);

  http_client.setHttpResponseTimeout(1000);
}

void postToFirebase(const char* method, const String &path, const String &data, HttpClient* http)
{
  String response;
  int statusCode = 0;
  http->connectionKeepAlive();
  

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


  String contentType = "application/json";
  http->put(url, contentType, data);
  

  // read the status code and body of the response
  //statusCode-200 (OK) | statusCode -3 (TimeOut)
  statusCode = http->responseStatusCode();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  response = http->responseBody();
  Serial.print("Response: ");
  Serial.println(response);


  if (!http->connected()) {
    Serial.println();
    http->stop();// Shutdown
    Serial.println("HTTP POST disconnected");
  }
  
}

void gpsloop()
{
  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neogps.available()){
      if (gps.encode(neogps.read())){
        newData = true;
        break;
      }
    }
  }


  //If newData is true
  if(true){
  newData = false;
  
  String latitude, longitude;

  
  latitude = String(gps.location.lat(), 6); // Latitude in degrees (double)
  longitude = String(gps.location.lng(), 6); // Longitude in degrees (double)
  
  //altitude = gps.altitude.meters(); // Altitude in meters (double)
  //date = gps.date.value(); // Raw date in DDMMYY format (u32)
  //time = gps.time.value(); // Raw time in HHMMSSCC format (u32)
  //speed = gps.speed.kmph();
  
  Serial.print("Latitude= "); 
  Serial.print(latitude);
  Serial.print(" Longitude= "); 
  Serial.println(longitude);
      
  String gpsData = "{";
  gpsData += "\"lat\":" + latitude + ",";
  gpsData += "\"lng\":" + longitude + "";
  gpsData += "}";

  postToFirebase("PATCH", FIREBASE_PATH, gpsData, &http_client);
}


void loop() {
  Serial.println(F("Waiting for network..."));
  if(!modem.waitForNetwork())
  {
    Serial.println("fail");
    delay(1000);
    return;
  }
  Serial.println("OK");
  Serial.println(F("Connecting to "));
  Serial.print(apn);
  if(!modem.gprsConnect(apn, user, pass))
  {
    Serial.println("fail");
    delay(1000);
    return;
  }
  Serial.println("OK");
  http_client.connect(FIREBASE_HOST, SSL_PORT);
  
  while(true)
  {
    if(!http_client.connected())
    {
      Serial.println();
      http_client.stop();
      Serial.println("HTTP not connected");
      break;
    }
    else{
      gpsloop();
    }
  }

}