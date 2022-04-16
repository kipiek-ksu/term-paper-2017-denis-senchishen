#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>

#include "EmonLib.h"            
EnergyMonitor emon1;


ESP8266WebServer server(80);

const char* ssid = "powerSocket-001";
const char* passphrase = "12345689";
const char* powerSocketId = "5b01fec7a1dd79665c63e942";

const char* apiUrl = "http://625c5c11.ngrok.io/api";

String st;
String content;
int statusCode;

bool isAuth = true;

void sendData(String data) {
  if(WiFi.status()== WL_CONNECTED){   
   
     HTTPClient http;    
   
     http.begin(String(apiUrl) + "/data");     
     http.addHeader("Content-Type", "application/json");  
   
     int httpCode = http.POST(data);
        
     String payload = http.getString();                  
   
     Serial.println(httpCode);   
     Serial.println(payload);   
     if (payload == "true") {
       digitalWrite(5, HIGH);
       Serial.println("ON");  
     } 
     
     if (payload == "false") {
      digitalWrite(5, LOW);
      Serial.println("OFF");
     }
   
     http.end();  
   
   }else{
   
      Serial.println("Error in WiFi connection");   
   
   }
  
}

void changeState() {
  if(WiFi.status()== WL_CONNECTED){   
   
     HTTPClient http;    
   
     http.begin(String(apiUrl) + "/updateState/" + String(powerSocketId));     
     http.addHeader("Content-Type", "application/json");  
   
     int httpCode = http.GET();
        
     String payload = http.getString();                  
   
     Serial.println(httpCode);   
     Serial.println(payload);   
   
     http.end();  
   
   }else{
   
      Serial.println("Error in WiFi connection");   
   }
}


void setup() {
  Serial.begin(115200);
  Serial.begin(9600);

  emon1.voltage(A0, 541, 1.7);
  
  emon1.current(A0, 10.1);

  pinMode(4, OUTPUT);

  pinMode(5, OUTPUT);

  //pinMode(6, INPUT);
  
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } 
  }
  setupAP();
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(WiFi.status());    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}

void setupAP(void) {
  isAuth = false;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ol>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ol>";
  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        content += ipStr;
        content += "<p>";
        content += st;
        content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"id\":\"" + String(powerSocketId) + "\"}";
          statusCode = 200;
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
        isAuth = true;
        delay(3000);
        ESP.restart();
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"ip\":" + ipStr + ",\"id\":" + powerSocketId + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      WiFi.disconnect();
      delay(1000);
      ESP.restart();
    });
  }
}

void loop() {
  server.handleClient();

  /*if (digitalRead(6) == HIGH){
    changeState();
  }*/
  
  if (isAuth) {
        digitalWrite(4, HIGH);
        delay(4);
        double Irms = emon1.calcIrms(1480); 
        
        digitalWrite(4, LOW);
        delay(4);
        emon1.calcVI(20, 2000); // Voltage
        
        float realPower       = emon1.realPower;       
        float apparentPower   = emon1.apparentPower;    
        float powerFactor     = emon1.powerFactor;     
        float supplyVoltage   = emon1.Vrms;            
        
        double Watt = Irms*supplyVoltage;
        Serial.print(Watt);           
        Serial.print(" ");
        Serial.print(Irms);           
        Serial.print(" ");
        Serial.print(supplyVoltage);
        Serial.println(" ");  

        sendData(String("{\"id\":\"" + String(powerSocketId) +"\",\"data\":[") + Watt + ',' + Irms + ',' + supplyVoltage +String("]}"));
        delay(3000);
  }
}
