#include <WiFi.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"

//Define DHT Data
#define DHTTYPE DHT22 

const char* ssid     = "SSID";
const char* password = "PASS";

const char* request = "https://maker.ifttt.com/trigger/<Event>/with/key/<key>";
const char* server = "maker.ifttt.com";

const uint64_t TIME_TO_SLEEP = 3600;//1800;

const uint8_t DHTPin = 22;
const uint8_t DS18b20Pin = 4;

OneWire oneWire(DS18b20Pin);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPin, DHTTYPE);  

void setup() {
  Serial.begin(115200); 
  delay(3000);

  Serial.println("Program start"); 

  pinMode(DHTPin, INPUT);
  dht.begin();

  Serial.println("Init WIFI:"); 

  if(initWifi() == true){
    Serial.println("SendRequest:"); 
    SendIFTTTRequest();
  }

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000);    
  esp_deep_sleep_start();
}

void loop() { }

bool initWifi() {
  Serial.print("Connecting to: "); 
  Serial.print(ssid);
  WiFi.begin(ssid, password);  

  int timeout = 100; // 10 seconds
  while(WiFi.status() != WL_CONNECTED  && (timeout > 0)) {
    delay(100);
    Serial.print(".");
    timeout--;
  }

  if(WiFi.status() != WL_CONNECTED) {
     Serial.println("/nWifi connection Error");
     return false;
  }

  Serial.print("/nWiFi connected, IP address: "); 
  Serial.println(WiFi.localIP());
  return true;
}

float GetDHTTemperature(void)
{
  return dht.readTemperature();
} 

float GetDHTHumid(void)
{
  return dht.readHumidity(); 
}

float GetDS18B20Temperature(void)
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);   
}

void SendIFTTTRequest() {
  WiFiClient client;
  
  float Humid_DHT22 = 0;
  float Temperature_DHT22 = 0;
  float Temperature_DS18B20 = 0;

  int retransmission = 5;

  while(!client.connect(server, 80) && (retransmission > 0)) {
    Serial.print(".");
    retransmission--;
  }
  Serial.println();

  if(!client.connected()) {
    Serial.println("Failed to connect...");
  }

  Temperature_DHT22 = GetDHTTemperature();
  Humid_DHT22 = GetDHTHumid();
  Temperature_DS18B20 = GetDS18B20Temperature();

  // Temperature in Celsius
  String jsonSensorData = String("{\"value1\":\"") +
                      (Temperature_DHT22) + 
                      "\",\"value2\":\"" + 
                      (Temperature_DS18B20) +
                      "\",\"value3\":\"" + 
                      (Humid_DHT22) + "\"}";
                      
  client.println(String("POST ") + request + " HTTP/1.1");
  client.println(String("Host: ") + server); 
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonSensorData.length());
  client.println();
  client.println(jsonSensorData);

  int timeout = 50; // 5 seconds             
  while(!client.available() && (timeout > 0)){
    delay(100);
    timeout--;

    if(timeout == 0){
      Serial.println("No response from server..");
    }
  }

  while(client.available()){
    //Print server response
    Serial.write(client.read());
  }
  
  Serial.println("\nClient close\n");
  client.stop(); 
}