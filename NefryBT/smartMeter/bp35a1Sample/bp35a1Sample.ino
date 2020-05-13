#include <Nefry.h>
#include <NefryDisplay.h>
#include <NefrySetting.h>
#include <time.h>
#include <ArduinoJson.h>
#include "intervalMs.h"
#include "bp35a1.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "mqttConfig.h"

void setting()
{
    Nefry.disableDisplayStatus();
}
NefrySetting nefrySetting(setting);

bp35a1 bp;
#define JST 3600 * 9

// Nefry Environment data

#define beebotteSmartMeterIdx 7
#define beebotteSmartMeterTag "Beebotte SmartMeter Token"
#define smartMeterIDIdx 8
#define smartMeterIDTag "Smart Meter ID"
#define smartMeterPWIdx 9
#define smartMeterPWTag "Smart Meter Password"

// ループ周期(ms)

// 即時電力値・電流値・累積電力値を取得
#define LOOPTIME_GET_EP_VALUE 60 * 1000
// 接続確認
#define LOOPTIME_CHECK_CONNECT 30 * 1000

// MQTT
const char *host = "mqtt.beebotte.com";
int QoS = 1;
const char *clientId;
String bbt_token;
WiFiClientSecure espClient;
PubSubClient mqttClient(host, 8883, espClient);

bool reconnect()
{
    Serial.print(F("\nAttempting MQTT connection..."));
    // Attempt to connect
    const char *user = bbt_token.c_str();
    if (mqttClient.connect(clientId, user, NULL))
    {
        Serial.println("connected");
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.println(mqttClient.state());
    }
    return mqttClient.connected();
}

void setup()
{
    Nefry.setProgramName("Access SmartMeter");
    Nefry.setStoreTitle(beebotteSmartMeterTag, beebotteSmartMeterIdx);
    Nefry.setStoreTitle(smartMeterIDTag, smartMeterIDIdx);
    Nefry.setStoreTitle(smartMeterPWTag, smartMeterPWIdx);

    // MQTT
    espClient.setCACert(beebottle_ca_cert);
    uint64_t chipid = ESP.getEfuseMac();
    String tmp = "ESP32-" + String((uint16_t)(chipid >> 32), HEX);
    clientId = tmp.c_str();
    //NefryのDataStoreに書き込んだToken(String)を(const char*)に変換
    bbt_token = "token:";
    bbt_token += Nefry.getStoreStr(beebotteSmartMeterIdx);

    //　date
    configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

    // シリアル通信するポート(RX:D2=23, TX:D3=19);
    int PIN_RX = 23;
    int PIN_TX = 19;
    String serviceID = Nefry.getStoreStr(smartMeterIDIdx);
    String servicePW = Nefry.getStoreStr(smartMeterPWIdx);
    bp.init(PIN_RX, PIN_TX, serviceID, servicePW);
    bp.connect();
}

void loop()
{
    bp.connect();

    // MQTT Clientへ接続
    if (!mqttClient.connected())
    {
        reconnect();
    }
    else
    {
        mqttClient.loop();
    }

    interval<LOOPTIME_CHECK_CONNECT>::run([] {
        bp.chkConnect();
    });

    interval<LOOPTIME_GET_EP_VALUE>::run([] {
        bp.getEPValue();

        if (bp.epA > 0.0f)
        {
            //日付を取得する
            time_t t = time(NULL);
            struct tm *tm;
            tm = localtime(&t);
            char getDate[15] = "";
            sprintf(getDate, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

            // mqtt送信向けにJsonデータを生成する
            char bufferData[200];
            StaticJsonDocument<200> root;
            root["date"] = getDate;
            root["a"] = bp.epA;
            root["kw"] = bp.epkW;
            root["ts"] = t;
            serializeJson(root, bufferData);
            Serial.println(bufferData);

            char bufferTotal[200];
            StaticJsonDocument<200> rootTotal;
            rootTotal["date"] = bp.date;
            rootTotal["tkw"] = bp.totalkWh;
            rootTotal["ts"] = t;
            serializeJson(rootTotal, bufferTotal);
            Serial.println(bufferTotal);

            if (mqttClient.connected())
            {
                mqttClient.publish(topicData, bufferData, QoS);
                mqttClient.publish(topicTotal, bufferTotal, QoS);
                Serial.println(F("MQTT publish!"));
            }
        }
    });
}