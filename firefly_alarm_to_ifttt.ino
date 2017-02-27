#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include "FireFly.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0xED };

char server[] = "maker.ifttt.com"; 
IPAddress ip(192, 168, 1, 100);
EthernetClient client;

String IFTTTKey = "57pj4rNesbEbS6QkANs_Y";
String TriggerWord = "ff_alarm";
/*
String IFTTTKey = "<YOUR IFTTT KEY>";
String TriggerWord = "<YOUR TRIGGER WORD>";
*/

int SerialMessageLength = 200;
char data[200] = {}; //serial buffer
int index = 0;
bool messageRead = false;
char moduleId[] = "909";

int alarm = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Serial ready!");

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  }

  FFSetSensors(moduleId, 1, 1, 1, 1, 1, 1);
  FFContinuousResponse(moduleId, "4", "10");
}

void loop() {
  if(client.available()){
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
    client.stop();
  }
  
  //Read sensor response
  while(Serial.available() > 0){
    //reading serial from FF1502 BLE module
    if(index < SerialMessageLength){
      data[index] = Serial.read();
      //Serial.print(data[index]);
      if(index == 0){
        if(data[index] == '!'){
          data[index] = ' ';
          index++;
        }
      }else{
        if(data[index] == '?'){ //Response message end
          data[index] = ' \0';
          index = 0;
          messageRead = true;
        }else if(data[index] == '!'){ //Response message start
          data[0] == ' ';
          index=1;
        }else{
          index++;
        }
      }
    }else{
      index = 0;
    }
    if(messageRead == true){
      Serial.println("message read");
      messageRead = false;
      
      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(data);

      if (!root.success()){
        Serial.println("parseObject() failed");
        return;
      }
      int accthr = root["d"] ["AccThr"];

      if(accthr == 1 && alarm == 0){
        alarm = 1;
        POSTrequest();
      }
      if(accthr == 0){
        alarm = 0;
      }

      Serial.println(accthr);

      data[0] = '\0'; //clear data array
      break;
    } 
  }
}

String getJSON() {
  String json = "{\"value1\":\"";
  json += 1;
  json += "\",\"value2\":\"";
  json += 2;
  json += "\",\"value3\":\"";
  json += 3;
  json += "\"}";

  return json;
}

int POSTrequest(){
  Serial.println("connecting...");
  if(client.connect(server, 80)) {
    Serial.println("connected");
    String json = getJSON();
  
    client.println("POST http://maker.ifttt.com/trigger/"+TriggerWord+"/with/key/"+IFTTTKey+" HTTP/1.1");
    client.println("Host: maker.ifttt.com");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println("Connection: close");
    client.println();
    client.println(json);
    client.println();
  } else {
    Serial.println("connection failed");
  }
}
