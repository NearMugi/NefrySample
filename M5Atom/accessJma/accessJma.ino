#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>

#include "amedas.h"

WiFiMulti WiFiMulti;
const String posID = "44132";
amedas amedasData(posID);

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("Buffalo-G-36F0", "pinkpanther48");

    // wait for WiFi connection
    Serial.print("Waiting for WiFi to connect...");
    while ((WiFiMulti.run() != WL_CONNECTED))
    {
        Serial.print(".");
    }
    Serial.println(" connected");
    amedasData.getData();
}

void loop()
{
}