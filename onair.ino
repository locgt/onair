/***************************************************
  On Air Ligh Controll, Based on Adafruit MQTT Library ESP8266 Example
  Modified by Ben Miller @vmfoo

 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"  //uses MQTT to get updates to state
#include "Adafruit_IO_Client.h"   //uses IOclient to get existing state


/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "YOURSSID"
#define WLAN_PASS       "YOURPASS"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOURUSERNAME"
#define AIO_KEY         "YOURKEY"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'onoff' for subscribing to changes.
const char ONOFF_FEED[] PROGMEM = AIO_USERNAME "/feeds/onair";  //Change "onair" to be your feed
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);

//Setup the REST client stuff - Allows us to fetch previous state
WiFiClient RESTclient;
Adafruit_IO_Client aio = Adafruit_IO_Client(RESTclient, AIO_KEY);
//Get the feed for IO use
Adafruit_IO_Feed iofeed = aio.getFeed("onair"); //Change "onair" to be your feed

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  pinMode(0, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);

  digitalWrite(0, HIGH);
  digitalWrite(5, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);

  
  delay(10);

  Serial.println(F("On Air Light Switch: v1.1"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);
}

uint32_t x=0;


void lightOff(){
  Serial.println("Turning Light Off");
  digitalWrite(0, HIGH);
  digitalWrite(5, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
}

void lightOn(){
  Serial.println("Turning Light On");
  digitalWrite(0, LOW);
  digitalWrite(5, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
}


void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      char *message=(char *)onoffbutton.lastread;
      Serial.println(message);
      if(!strcmp(message, "On")) {
        lightOn();
      }
      if(!strcmp(message, "Off")) {
        lightOff();
      }
    }
  }


  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  delay(1000);

}



// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  Serial.println("MQTT Connected!");
  
  //If we've just conencted, then lets get the state we should have before we sit and listen.
  Serial.println("Fetching previous state");

  //Now go get the current state:
  FeedData latest = iofeed.receive();
  if (latest.isValid()) {
    char *message=(char *)latest;
    Serial.print(F("Received value from feed:...")); Serial.print(message); Serial.println("...");
    if (!strcmp(message,"On")) {
      lightOn();   
    }
    if (!strcmp(message,"Off")) {
      lightOff();
    }
  }
  else {
    Serial.print(F("Failed to receive the latest feed value!"));
  }
  
}
