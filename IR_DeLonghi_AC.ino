/* MQTT IR Remote for DeLonghi A/C Model: Pinguino PAC EM93 Silent
*
* Goal: Home Assistant integration (or other home automation) of AC
* Hardware: Lolin/Wemos D1 mini (ESP8266) & IR Controller Shield
*
* Based on the work of David Conran https://github.com/crankyoldgit/IRremoteESP8266
*  and Adafruit                     https://github.com/adafruit/Adafruit_MQTT_Library
*/

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Electra.h>  // works with Pinguino PAC EM93 Silent and ir_Delonghi.h doesn´t.
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

const uint16_t kIrLed = 0;  // Lolin/Wemos D1 mini (ESP8266) + IR Controller Shield GPIO pin uses 0 (D3).
IRElectraAc ac(kIrLed);

/************************* WIFI Setup *********************************/
#define WLAN_SSID "your_SSID"
#define WLAN_PASS "your_PASSWORD"
String hostname = "D1_mini_IR-Controller";
/************************* MQTT Setup *********************************/
#define AIO_SERVER "your_MQTT_Broker_IP"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "your_MQTT_USERNAME"
#define AIO_KEY "your_MQTT_PASSWORD"
#define AIO_TOPIC "IR-Remote"
/**********************************************************************/

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish topic_state = Adafruit_MQTT_Publish(&mqtt, AIO_TOPIC "/info/state");
Adafruit_MQTT_Publish topic_emptying = Adafruit_MQTT_Publish(&mqtt, AIO_TOPIC "/command");
Adafruit_MQTT_Subscribe topic_command = Adafruit_MQTT_Subscribe(&mqtt, AIO_TOPIC "/command");

// initial settings
void ac_initially_settings() {
  ac.begin();
  ac.off();
  ac.setMode(kElectraAcCool);
  ac.setTemp(22);
  ac.setFan(kElectraAcFanLow);
  ac.setSwingV(false);                          // no hardware
  ac.setSwingH(false);                          // no hardware
  ac.setLightToggle(kElectraAcLightToggleOff);  // no hardware
}

void MQTT_connect();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // Connect to WiFi access point.
  WiFi.hostname(hostname.c_str());
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    led_notify_long();
  }
  // Setup MQTT subscription for topic_command feed.
  mqtt.subscribe(&topic_command);
  ac_initially_settings();
}

void loop() {
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &topic_command) {
      if (0 == strcmp((char *)topic_command.lastread, "state")) {
        topic_state.publish(ac.toString().c_str());
      }
      if (0 == strcmp((char *)topic_command.lastread, "send")) {
        do_it();
      }
      if (0 == strcmp((char *)topic_command.lastread, "on") || 0 == strcmp((char *)topic_command.lastread, "off") || 0 == strcmp((char *)topic_command.lastread, "power")) {
        if (0 == strcmp((char *)topic_command.lastread, "on")) {
          ac.on();
        } else if (0 == strcmp((char *)topic_command.lastread, "off")) {
          ac.off();
        } else if (0 == strcmp((char *)topic_command.lastread, "power")) {
          if (ac.getPower() == false) {
            ac.on();
          } else {
            ac.off();
          }
        }
        do_it();
      }
      if (0 == strcmp((char *)topic_command.lastread, "cool") || 0 == strcmp((char *)topic_command.lastread, "dry") || 0 == strcmp((char *)topic_command.lastread, "fan") || 0 == strcmp((char *)topic_command.lastread, "mode") && ac.getPower() == true) {
        if (0 == strcmp((char *)topic_command.lastread, "cool")) {
          ac.setMode(kElectraAcCool);
        } else if (0 == strcmp((char *)topic_command.lastread, "dry")) {
          ac.setMode(kElectraAcDry);
        } else if (0 == strcmp((char *)topic_command.lastread, "fan")) {
          ac.setMode(kElectraAcFan);
        } else if (0 == strcmp((char *)topic_command.lastread, "mode")) {
          switch (ac.getMode()) {
            case kElectraAcFan:
              ac.setMode(kElectraAcCool);
              break;
            case kElectraAcCool:
              ac.setMode(kElectraAcDry);
              break;
            case kElectraAcDry:
              ac.setMode(kElectraAcFan);
              break;
          }
        }
        do_it();
      }
      if (0 == strcmp((char *)topic_command.lastread, "low") || 0 == strcmp((char *)topic_command.lastread, "medium") || 0 == strcmp((char *)topic_command.lastread, "high") || 0 == strcmp((char *)topic_command.lastread, "auto") || 0 == strcmp((char *)topic_command.lastread, "speed") && ac.getPower() == true && kElectraAcDry != ac.getMode()) {
        if (0 == strcmp((char *)topic_command.lastread, "low")) {
          ac.setFan(kElectraAcFanLow);
        } else if (0 == strcmp((char *)topic_command.lastread, "medium")) {
          ac.setFan(kElectraAcFanMed);
        } else if (0 == strcmp((char *)topic_command.lastread, "high")) {
          ac.setFan(kElectraAcFanHigh);
        } else if (0 == strcmp((char *)topic_command.lastread, "auto")) {
          ac.setFan(kElectraAcFanAuto);
        } else if (0 == strcmp((char *)topic_command.lastread, "speed")) {
          switch (ac.getFan()) {
            case kElectraAcFanAuto:
              ac.setFan(kElectraAcFanLow);
              break;
            case kElectraAcFanLow:
              ac.setFan(kElectraAcFanMed);
              break;
            case kElectraAcFanMed:
              ac.setFan(kElectraAcFanHigh);
              break;
            case kElectraAcFanHigh:
              ac.setFan(kElectraAcFanAuto);
              break;
          }
        }
        do_it();
      }
      if ((0 == strcmp((char *)topic_command.lastread, "+") || 0 == strcmp((char *)topic_command.lastread, "-")) && ac.getPower() == true && kElectraAcDry != ac.getMode() && kElectraAcFan != ac.getMode()) {
        if (0 == strcmp((char *)topic_command.lastread, "+")) {
          ac.setTemp(ac.getTemp() + 1);
        } else if (0 == strcmp((char *)topic_command.lastread, "-") && ac.getTemp() >= 19) {
          ac.setTemp(ac.getTemp() - 1);
        }
        do_it();
      }
    }
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {  // connect will return 0 for connected
    led_notify_long();
    mqtt.disconnect();
    //delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1)
        ;
    }
  }
  topic_state.publish(ac.toString().c_str());
  topic_emptying.publish("empty");
}

void led_notify() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
}

void led_notify_long() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
}

void do_it() {
  topic_state.publish(ac.toString().c_str());
  led_notify();
  ac.send();
}
