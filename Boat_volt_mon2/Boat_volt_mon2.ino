/*
 ESP8266 Blink by Simon Peter
 Blink the blue LED on the ESP-01 module
 This example code is in the public domain
 
 The blue LED on the ESP-01 module is connected to GPIO1 
 (which is also the TXD pin; so we cannot use Serial.print() at the same time)
 
 Note that this sketch uses LED_BUILTIN to find the pin with the internal LED

 new board, so new rev 2.0

*/

const char *gRev = "bat-0.2.0";  // Software Revision Code

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
// #define SLEEPTIME 10 * 1000000      // seconds 10 000 000  600=10 minutes
#define SLEEPTIME 600 * 1000      // seconds 10 000 millisec for delay, not dfeepsleep  600=10 minutes

#include <Wire.h>
#include <Adafruit_ADS1015.h>

unsigned long startTime = millis();

// Static IP details...
IPAddress ip(192, 168, 11, 63);
IPAddress gateway(192, 168, 11, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 11, 1);
const char* ssid     = "tcls";
const char* password = "svkatielee";

IPAddress mqtt_server(192, 168, 11, 30);
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
String clientId = "Batt2";   // Create a client ID
Adafruit_ADS1115 ads; 
int16_t adc1;

// OTA config --------------------------------
void checkForOTA() {
  t_httpUpdate_return ret = ESPhttpUpdate.update("192.168.11.30", 80, "/serv.php", gRev ); // only md5sum, not use version
        
  // It does NOT return from successfull update!!
  snprintf (msg, 50, "Update successful");
  switch(ret) {
       case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
            Serial.println();
            snprintf (msg, 50, "HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); 
            break;

       case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            snprintf (msg, 50, "No newer update available");
            break;

       case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            snprintf (msg, 50, "Update successful");
            break;
        }
}





void setup() {
  
  byte ret_code;
   
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println("--> Connecting Wifi");
 // if (strcmp (WiFi.SSID().c_str(), ssid) != 0) {
    byte timeout = 0;  
  
    WiFi.config(ip, gateway, subnet, dns);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (timeout++ >= 15) {
        Serial.println("--> ERROR: Wifi commection timeout");
        break;
      }
    }
  //}
  Serial.println();
  Serial.println(WiFi.macAddress());
  
  client.setServer(mqtt_server, 1883);
  ads.setGain(GAIN_TWOTHIRDS);     // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  ads.begin();
  
}

// the loop function runs over and over again forever
void loop() {
    startTime = millis();
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(clientId.c_str())) {
          Serial.println("connected");
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
      
          if(client.state() == -2 ) {
            Serial.println("MQTT:  WiFi not connected.");
          }
        } 
    }
  
  // check for OTA update
  checkForOTA();
  yield();
  delay(100);
    int cycle = millis() - startTime;
    snprintf (msg, 75, "{\"Revision\": %s, \"Duration\": %d}", gRev, cycle); 
    if (!client.publish("tcls/Battery/rev", msg, true) ) {
        Serial.println("Failed to publish !!!");
    } else {
        Serial.print("Publish message: "); Serial.println(msg); 
    }
    delay(100);
    yield();

  char v_str[15];  
  
  // read ADS1115
  adc1 = ads.readADC_SingleEnded(1);
  long v4 = map(adc1,  21176, 26818, 11010, 14010);
  Serial.print("v4: "); Serial.println((float)v4/1000.0f);
  float_t v3 = ((float)v4/1000.0f);
  dtostrf(v3, 2, 3, v_str);
  
  Serial.print("v_str: "); Serial.println(v_str);
  snprintf (msg, 75, "{\"ADS1115\": %s}", v_str); 
    if (!client.publish("tcls/Battery/adc", msg, true) ) {
        Serial.println("Failed to publish !!!");
    } else {
      Serial.print("Publish message: "); Serial.println(msg); 
    }
    yield();  
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                    // but actually the LED is on; this is because 
                                    // it is active low on the ESP-01)
  delay(100);                      // Wait for a second

  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(SLEEPTIME);                      // Wait for SLEEPTIME (to demonstrate the active low LED)
}
