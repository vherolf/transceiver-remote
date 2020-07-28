/*

This is a arduino sketch for a esp32 to build a remote control that can 
send and receive mqtt messages.
The ESp32 connects to your wireless network and connects to your mqtt server. 

Functions:
 - connect to your wifi
 - connect to yout mqtt server
 - can publish mqtt messages 
 - can subscribe to mqtt topics
 - on a triggered GPIO sends a mqtt message (optional - set GPIOREMOTE)
 - sets a GPIO if a subscribed mqtt message arrived (optional - set GPIOREMOTE)
 - use a Adafruit MPR121 for touch input to a send mqtt message (optional - set MPR121)
 - OTA (Over the air) updates (optional - set OTA)

Tested with nodeMCU, Adafruit Feather ESP32 and Heltec Wireless Stick

*/
;


// Here you define your options, CLIENTID and which board you have
// NEVER flash 2 boards with same CLIENTID or you run into mqtt reconnect hell
namespace {

/*
// name: xylophon
// board: Heltec wireless stick
// mac: 
const char* CLIENTID = "xylophon";
#define MPR121 1
#define GPIOREMOTE 0
#define OTA 1
*/

// name: greenbox
// board: Heltec wireless stick
// mac: 6bbf713c
const char* CLIENTID = "greenbox";
#define MPR121 1
#define GPIOREMOTE 0
#define OTA 1


/*
// name: rack
// board: Adafruit feather 32
// mac: 
const char* CLIENTID = "rack";
#define MPR121 0
#define GPIOREMOTE 1
#define OTA 1
*/
} 

#if MPR121 == 1
  #include "Adafruit_MPR121.h"
#endif

#if GPIOREMOTE == 1
  // Define if you use a switch to trigger a mqtt message
  // change also in setup and loop
  
  // for ESP32 Feather
  /*
  const int PushButtonRECORD = 13;
  const int PushButtonSTOP = 14;
  const int PushButtonREWIND = 15;
  const int PushButtonPLAY = 27;
  const int PushButtonFORWARD = 32;
  const int PushButtonEJECT = 33;
  */
  // for Heltec Wireless stick
  /*
  const int PushButtonRECORD = 13;
  const int PushButtonSTOP = 36;
  const int PushButtonREWIND = 37;
  const int PushButtonPLAY = 38;
  const int PushButtonFORWARD = 39;
  const int PushButtonEJECT = 23;
  */
#endif


// Woodquater
//const int PushButtonHolz = 38;
//const int PushButtonHof = 38;

// int RELAIS_ONE = 33; // define Pin where the relais is attached
   
// comment this line if you dont use a config.h
#include "config.h"
// Variables set in config.h - just uncomment if you dont use a config.h
//const char* ssid = "";
//const char* wifiPassword = "";
//const char* mqttServer = "";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
unsigned long check_wifi = 30000;

//comment this in is you use platformio instead of Arduino IDE
//#inlcude "Arudino.h"
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

#if OTA == 1
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
#endif

WiFiClient espClient;
PubSubClient client(espClient);
int count = 0;

#ifndef _BV
   #define _BV(bit) (1 << (bit)) 
#endif

#if MPR121 == 1
  Adafruit_MPR121 cap = Adafruit_MPR121();
#endif

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// This function is to handle incoming messages
void callback(char* intopic, byte* payload, unsigned int length) {
  // decode message 
  String message = "";
  for (unsigned int i=0; i< length; i++) {
    message = message + (char)payload[i];
  }
  // decode topic
  String topic = String(intopic);
  
  Serial.println("MQTT Message: " + message + " Topic: " + topic);

  /*
  // listen for specific messages and topics
  if(topic == "hof" && message == "lighton") {
    // Turn the Light on
    digitalWrite(RELAIS_ONE, HIGH);
    Serial.println("schalte im hof das licht ein");
  }
  if(topic == "hof" && message == "lightoff") {
    // Turn the light off
    digitalWrite(RELAIS_ONE, LOW);
    Serial.println("schalte im hof das licht aus");
  }
  */
  if(topic == "remote/scene" && message == "konzentration") {
    //digitalWrite(BUILTIN_LED, HIGH);
    Serial.println("got message "+ message + " to switch led ");
  }

}

long lastReconnectAttempt = 0;

boolean reconnect() {
  Serial.println("Reconnecting MQTT Client: "+ String(CLIENTID) );
  if (client.connect(CLIENTID)) {
    // Once connected, publish an announcement...
    String mqttStatus = "client " + String(CLIENTID) + " reconnected";
    client.publish("/status",(char*) mqttStatus.c_str() );
    // subscribe to all channels
    client.subscribe("#");
    // or specific channels
    //client.subscribe("wohnung/scene");
    //client.subscribe("status");
  }
  return client.connected();
}

void setup()
{
  Serial.begin(115200);
  String CHIPID = String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.println(CHIPID);


#if GPIOREMOTE == 1
  //pinMode(PushButtonHOF, INPUT);
  //pinMode(PushButtonHOLZ, INPUT);
  pinMode(PushButtonRECORD, INPUT);
  pinMode(PushButtonSTOP, INPUT);
  pinMode(PushButtonREWIND, INPUT);
  pinMode(PushButtonPLAY, INPUT);
  pinMode(PushButtonFORWARD, INPUT);
  pinMode(PushButtonEJECT, INPUT);
  pinMode(RELAIS_ONE, OUTPUT);
#endif
  
#if MPR121 == 1
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
#endif
 
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
/*
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, wifiPassword);
  WiFi.setSleep(false);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    if (count == 5){
        WiFi.begin(ssid, wifiPassword);
        count = 0;
        Serial.println();
        Serial.println("Retrying Wifi connection of client "+ String(CLIENTID) );
        delay(5000);
    }
   count ++;
  }
  Serial.println("Client: " + String(CLIENTID) + " connected to the WiFi network");
  lastReconnectAttempt = 0;
*/
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

#if OTA == 1
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(CLIENTID);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("YEAH OTA is on");
#endif

}


void loop()
{
  static unsigned long last_pushbutton_time = 0;
  
  // check MQTT connection
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        Serial.println("Client " + String(CLIENTID) + " reconnected" );
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }


  // check WiFi connection
  if ((WiFi.status() != WL_CONNECTED) && (millis() > check_wifi)) {
    Serial.println("Reconnecting Wifi client " + String(CLIENTID) +  " to WiFi");
    WiFi.disconnect();
    WiFi.begin(ssid, wifiPassword);
    check_wifi = millis() + 30000;
  }


#if GPIOREMOTE == 1
  if(millis() - last_pushbutton_time > 125) {
    // read switch to send a mqtt message
    int Push_Button_RECORD_State = digitalRead(PushButtonRECORD);
    if (Push_Button_RECORD_State) {
          //client.publish("hof", "lighton");
          Serial.println("Switch " + String(PushButtonRECORD) + " on");
          client.publish("remote/scene", "konzentration");
    }
    int Push_Button_STOP_State = digitalRead(PushButtonSTOP);
    if (Push_Button_STOP_State) {
          Serial.println("Switch " + String(PushButtonSTOP) + " on");
          client.publish("remote/scene", "space");
    }
    int Push_Button_REWIND_State = digitalRead(PushButtonREWIND);
    if (Push_Button_REWIND_State) {
          Serial.println("Switch " + String(PushButtonREWIND) + " on");
          client.publish("remote/scene", "chill");
    }
    int Push_Button_PLAY_State = digitalRead(PushButtonPLAY);
    if (Push_Button_PLAY_State) {
          Serial.println("Switch " + String(PushButtonPLAY) + " on");
          client.publish("remote/scene", "konzentration");
    }
    int Push_Button_FORWARD_State = digitalRead(PushButtonFORWARD);
    if (Push_Button_FORWARD_State) {
          Serial.println("Switch " + String(PushButtonFORWARD) + " on");
          client.publish("remote/scene", "space");
    }
    int Push_Button_EJECT_State = digitalRead(PushButtonEJECT);
    if (Push_Button_EJECT_State) {
          Serial.println("Switch " + String(PushButtonEJECT) + " on");
          client.publish("remote/scene", "schlafenszeit");
    }
  last_pushbutton_time = millis();
  }
  delay(1);
#endif

#if MPR121 == 1
  // Get the currently touched pads
  currtouched = cap.touched();
  
  for (uint8_t i=0; i<12; i++) {
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" touched");
      if (i == 0) {
        client.publish("remote/scene", "tor");
      }
      if (i == 1) {
        //client.publish("remote/scene", "hof");
      }
      if (i == 2) {
        //client.publish("remote/scene", "holz");
      }
      if (i == 3) {
        client.publish("remote/scene", "stube");
      }
      if (i == 4) {
        client.publish("remote/scene", "schlafenszeit");
      }
      if (i == 5) {
        client.publish("remote/scene", "schlafenszeit");
      }
      if (i == 6) {
        client.publish("remote/scene", "space");
      }
      if (i == 7) {
        client.publish("remote/scene", "space");
       }
      if (i == 8) {
        client.publish("remote/scene", "chill");
      }
      if (i == 9) {
        client.publish("remote/scene", "chill");
      }
      if (i == 10) {
        client.publish("remote/scene", "konzentration");
      }
      if (i == 11) {
        client.publish("remote/scene", "konzentration");
      }

    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
      Serial.print(i); Serial.println(" released");
    }
  }

  // reset our state
  lasttouched = currtouched;
#endif

#if OTA == 1
  ArduinoOTA.handle();
#endif
}
