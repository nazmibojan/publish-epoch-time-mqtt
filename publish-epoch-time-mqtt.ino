/*
 * @brief: request relay status to arduino 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ESP8266WiFi.h>
#include "time.h"
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//#define SERIAL_EVENT

#define RXD2    16
#define TXD2    17
#define PERIOD  5000

const char* ssid      = "CONNEXT-AXIATA";
const char* password  = "4xiatadigitallabs18";
const char* ntp_server = "pool.ntp.org";
const char* mqtt_server = "mqtt.flexiot.xl.co.id";
const int mqtt_port = 1883;
const char* mqtt_user = "generic_brand_2003-esp32_test-v2_3792";
const char* mqtt_pwd = "1579159694_3792";
String device_serial = "1595649038140789";
const char* event_topic = "generic_brand_2003/esp32_test/v2/common";
String sub_topic_string = "+/" + device_serial + "/generic_brand_2003/esp32_test/v2/sub";

const long utcOffsetInSeconds = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

char msg[300];
double relay_status = 0;
unsigned long last_request = 0;
unsigned long epoch_time = 0;

WiFiClient ESPClient;
PubSubClient client(ESPClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup()
{
  Serial.begin(115200);

  setup_wifi();

  // Connect to MQTT Server
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.println("ESP > Connecting to MQTT...");

    if (client.connect("ESP32Client", mqtt_user, mqtt_pwd)) {
      Serial.println("Connected to FlexIoT");
    } else {
      Serial.print("ERROR > failed with state");
      Serial.print(client.state());
      Serial.print("\r\n");
      delay(2000);
    }
  }

  timeClient.begin();
  client.setCallback(callback);
}

void loop()
{
  if(WiFi.status() != WL_CONNECTED){
    setup_wifi();  
  }

  if (WiFi.status()== WL_CONNECTED && !client.connected()) {
    reconnect();
  }
  
  if (millis() - last_request > PERIOD) {
    last_request = millis();

    Serial.println("get_status");

    printLocalTime();
    send_event();
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void printLocalTime()
{
  timeClient.update();

  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());

  epoch_time = timeClient.getEpochTime();
}

//receiving a message
void callback(char* topic, byte* payload,long length) {
  Serial.print("Message arrived [");
  Serial.print(sub_topic_string);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  do_actions(msg);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    char id[32] = "";
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    clientId.toCharArray(id, sizeof(id));
    // Attempt to connect
    if (client.connect(id, mqtt_user, mqtt_pwd)) {
      Serial.println("connected");
      //subscribe to the topic
      const char* sub_topic = sub_topic_string.c_str();
      client.subscribe(sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publish_message(const char* message){
//  Serial.println(message);
  client.publish(event_topic, message);
  Serial.println("Event published...");  
}

//====================================================================================================================================================================  
void send_event(){

  char msgtosend[1024] = {0};
  char day[3], month[3], year[5], hour[3], minute[3], second[3];  
  char epoch_sec[16];
  char relay[2];
  double relay_status = 1;

//  String sday = String(timeinfo.tm_mday);
//  String smonth = String(timeinfo.tm_mon + 1);    /**< months since January - [ 0 to 11 ] */
//  String syear = String(timeinfo.tm_year + 1900); /**< years since 1900 */
//  String shour = String(timeinfo.tm_hour);
//  String sminute = String(timeinfo.tm_min);
//  String ssecond = String(timeinfo.tm_sec);
  String sepoch = String(epoch_time);
//  sday.toCharArray(day, sizeof(day));
//  smonth.toCharArray(month, sizeof(month));
//  syear.toCharArray(year, sizeof(year));
//  shour.toCharArray(hour, sizeof(hour));
//  sminute.toCharArray(minute, sizeof(minute));
//  ssecond.toCharArray(second, sizeof(second));
  sepoch.toCharArray(epoch_sec, sizeof(epoch_sec));
  dtostrf(relay_status, 1, 0, relay);
  

  strcat(msgtosend, "{\"eventName\":\"relayStatus\",\"status\":\"none\"");
//  strcat(msgtosend, ",\"day\":");
//  strcat(msgtosend, day);
//  strcat(msgtosend, ",\"month\":");
//  strcat(msgtosend, month);
//  strcat(msgtosend, ",\"year\":");
//  strcat(msgtosend, year);
//  strcat(msgtosend, ",\"hour\":");
//  strcat(msgtosend, hour);
//  strcat(msgtosend, ",\"minute\":");
//  strcat(msgtosend, minute);
  strcat(msgtosend, ",\"relay\":");
  strcat(msgtosend, relay);
  strcat(msgtosend, ",\"time\":");
  strcat(msgtosend, epoch_sec);
  strcat(msgtosend, ",\"mac\":\"1595649038140789\"}");
  publish_message(msgtosend);  //send the event to backend
  memset(msgtosend, 0, sizeof msgtosend);
}
//====================================================================================================================================================================
//==================================================================================================================================================================== 
void do_actions(const char* message){
  //Create this function according to your actions. you will receive a message something like this
  /* Eg : {
        "action":"actionOne",
        "param":{
          "ac1Value1":"1110" ,
          "parentMac":"6655591876092787",
          "ac1Value4":"port:03",
          "ac1Value5":"on",
          "mac":"6655591876092787",
          "ac1Value2":"2220",
          "ac1Value3":"567776"
        }
      } */
  Serial.println(message);
}
//=====================================================================================================================================================================
