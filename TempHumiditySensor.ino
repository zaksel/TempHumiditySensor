// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include "DHT.h"                
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include <CircularBuffer.h>

#define DHTPIN 3
#define DHTTYPE DHT22
#define INTERVAL 10000    // time  between measurements in ms
#define STORAGE 1500      // 17280 storage for 48hours @1/10Hz

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Replace with your network credentials
const char* ssid = "****************";
const char* password = "*****************";

int timestamp;
float temperature;
float humidity;
CircularBuffer<int,STORAGE> timestamp_log;
CircularBuffer<float,STORAGE> temperature_log;
CircularBuffer<float,STORAGE> humidity_log;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DHT dht(DHTPIN, DHTTYPE);
    
void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  SPIFFS.begin();
  server.begin();
  dht.begin();
  timeClient.begin();
  
  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Load Page");
    request->send(SPIFFS, "/index.html");
  });
  server.on("/now", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get now");
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument jsonObj(1024);
    jsonObj["timestamp"] = timestamp;
    jsonObj["temperature"] = temperature;
    jsonObj["humidity"] = humidity;
    serializeJson(jsonObj, *response);
    request->send(response);
  });
  server.on("/timestamp_log", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get Log Timestamps");
    AsyncJsonResponse *response = new AsyncJsonResponse(false, 20000);
    JsonVariant& jsonObj = response->getRoot();
    for (int i = 0; i<STORAGE; i++) {
      jsonObj["timestamp_log"][i] = timestamp_log[i];
    }
    response->setLength();
    request->send(response);
  });
  server.on("/temperature_log", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get Log Temperature");
    //Serial.println(ESP.getFreeHeap());
    AsyncJsonResponse *response = new AsyncJsonResponse(false, 20000);
    JsonVariant& jsonObj = response->getRoot();
    for (int i = 0; i<STORAGE; i++) {
      jsonObj["temperature_log"][i] = temperature_log[i];
    }
    response->setLength();
    request->send(response);
  });
  server.on("/humidity_log", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Get Log Humidity");
    //Serial.println(ESP.getFreeHeap());
    AsyncJsonResponse *response = new AsyncJsonResponse(false, 20000);
    JsonVariant& jsonObj = response->getRoot();
    for (int i = 0; i<STORAGE; i++) {
      jsonObj["humidity_log"][i] = humidity_log[i];
    }
    response->setLength();
    request->send(response);
  });

  //pinMode(0, OUTPUT);
}
 
void loop(){  
  timeClient.update();
  timestamp = timeClient.getEpochTime();
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  Serial.println("Temp: " + String(temperature) +"°C | Humidity: "+ String(humidity) + "%");
    
  timestamp_log.unshift(timestamp);
  temperature_log.unshift(temperature);
  humidity_log.unshift(humidity);

  delay(INTERVAL);
}
