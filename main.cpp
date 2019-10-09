#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <SSD1306.h>

#include "WebService.h"
#include "DubButton.h"
#include "Globals.h"
#include "utils.h"


#define MQTT_HOST "192.168.1.157"   // MQTT host (e.g. m21.cloudmqtt.com)
#define MQTT_PORT 11883             // MQTT port (e.g. 18076)

#define DEVICE_ID "usbtimer"        // Used for MQTT topics

#define PIN_RELY        2
#define PIN_SPEAKER     0

const char * Globals::appVersion = "1.01";

const ulong DEFAULT_PERIOD = 3*60*1000;

SSD1306             display(0x3c, 1 /*TX*/, 3 /*RX*/, GEOMETRY_128_32);
WiFiClient          client; // WiFi Client
WebService          webService;

bool    wifiMelodyPlayed         = false;
int Globals::remainingTime       = DEFAULT_PERIOD;

Adafruit_MQTT_Client mqtt(&client, MQTT_HOST, MQTT_PORT);   // MQTT client

// Setup MQTT topics
String                  mqttOutputTopic     = String() + "wifi2mqtt/" + DEVICE_ID;
String                  mqttCmdTopic        = String() + "wifi2mqtt/" + DEVICE_ID + "/set";
Adafruit_MQTT_Publish   mqttPublish         = Adafruit_MQTT_Publish     (&mqtt, mqttOutputTopic.c_str());
Adafruit_MQTT_Subscribe mqttSubscribe       = Adafruit_MQTT_Subscribe   (&mqtt, mqttCmdTopic.c_str(), MQTT_QOS_1);

DubButton button(PIN_SPEAKER);

void myTone(int freq, int duration)
{
    tone(PIN_SPEAKER, freq, duration);
    delay(duration);
    pinMode(PIN_SPEAKER, INPUT);
}

void turnOnForPeriod(int periodMillis)
{
    Globals::remainingTime   = periodMillis;
    digitalWrite(PIN_RELY, HIGH);
}

void turnOff()
{
    digitalWrite(PIN_RELY, LOW);
}

String butifyTime(int millis)
{
    char str[8];
    float seconds = (float)millis / 1000;
    int minutes = seconds / 60;
    // if(!minutes)
    //     return String((float)millis/1000, 1) + " s";

    sprintf(str, "%02d:%04.1f", minutes, seconds - 60 * minutes);
    return String(str);

    //return String(minutes) + " m " + String(seconds - 60 * minutes, 1) + " s";
}

ulong  infoMsgUntil = 0;
String infoMsg = "";

void displayLoop() 
{
    display.clear();
    display.setFont(ArialMT_Plain_24);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    if(millis() < infoMsgUntil)
    {
        display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display.drawString(64, 16, infoMsg);
    }
    else
    {
        if(Globals::remainingTime > 0){
            display.drawString(0, 0, butifyTime(Globals::remainingTime));
        }
        else{
            display.drawString(0, 0, "OFF");
        }


        if (WiFi.status() == WL_CONNECTED) 
        {
            display.setFont(ArialMT_Plain_10);
            display.setTextAlignment(TEXT_ALIGN_RIGHT);
            display.drawString(128, 22, WiFi.localIP().toString());
        }

    }

    display.display();
}



void setup()
{
    
    pinMode(PIN_SPEAKER, INPUT);
    pinMode(PIN_RELY, OUTPUT);
    digitalWrite(PIN_RELY, LOW);

    button.init();
    button.onClick([](){
        Globals::remainingTime += 10*1000;
        infoMsg = "+ 10 sec";
        infoMsgUntil = millis() + 500;
        myTone(400, 50);
    });

    button.onDoubleClick([](){
        Globals::remainingTime -= 10*1000;
        infoMsg = "- 10 sec";
        infoMsgUntil = millis() + 500;
        myTone(500, 50);
    });

    button.onPressedFor(1000, [](){
        Globals::remainingTime = 0;
        infoMsg = "stopped";
        infoMsgUntil = millis() + 1000;
        myTone(500, 300);
    });


    // Play first start melody
    myTone(800, 100);
    myTone(400, 100);
    myTone(1200, 100);


    String apName = String("esp-") + DEVICE_ID + "-v" + Globals::appVersion + "-" + ESP.getChipId();
    apName.replace('.', '_');
    WiFi.hostname(apName);

    WiFi.begin("OpenWrt_2GHz", "qT8LjA70");

    
    // init web service
    webService.init();


    // spash screen    
    display.init();
    display.setContrast(1, 5, 0);
    display.flipScreenVertically();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, String("usb timer v") + Globals::appVersion);
    display.display();
    delay(500);
    


    // Setup MQTT subscription for the 'set' topic.
    mqtt.subscribe(&mqttSubscribe);
    
    // subscribe 'set' topic
    mqttSubscribe.setCallback([](char *payload, uint16_t len){

        
        if(String("restart").equals(payload)){
            Serial.println("Going to restart..");
            ESP.restart();
            return;
        }

        if(String("off").equals(payload)){
            turnOff();
            mqttPublish.publish("turned off");
            return;
        }

        if(String(payload).startsWith("on ")){
            String secondsStr = String(payload).substring(3);
            int seconds = secondsStr.toInt();

            turnOnForPeriod(seconds * 1000);

            String msg = String("turned on for ") + seconds + String(" sec");
            mqttPublish.publish(msg.c_str());

            return;
        }

        // publish test
        String text = String("Uptime: ") + millis()/1000;
        text += String(" Remaining: ") + Globals::remainingTime / 1000;

        mqttPublish.publish(text.c_str());
    });

    mqttPublish.publish("usb timer started!");

    ArduinoOTA.begin();

    turnOnForPeriod(DEFAULT_PERIOD);
}

ulong loopEvnerTs = 0;

void loop()
{
    ulong prevLoopEvnerTs = loopEvnerTs;
    loopEvnerTs = millis();

    Globals::remainingTime -= millis() - prevLoopEvnerTs;

    if(Globals::remainingTime < 0) Globals::remainingTime = 0;

    displayLoop();
    button.loop();
    webService.loop();
    ArduinoOTA.handle();

    if(Globals::remainingTime <= 0)
    {
        digitalWrite(PIN_RELY, LOW);
    }
    else{
        digitalWrite(PIN_RELY, HIGH);
    }
    

    if(WiFi.status() == WL_CONNECTED)
    {

        if(!wifiMelodyPlayed){
            wifiMelodyPlayed = true;
            myTone(1000, 100);
            myTone(500, 100);
            myTone(1500, 100);
        }

        // Ensure the connection to the MQTT server is alive (this will make the first
        // connection and automatically reconnect when disconnected).  See the MQTT_connect()
        MQTT_connect(&mqtt);
        
        // wait X milliseconds for subscription messages
        mqtt.processPackets(10);
    }

}
