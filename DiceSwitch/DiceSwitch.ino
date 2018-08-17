//箱型スイッチ
//publishの上限があるので送りすぎないようにする。変化がない時は送らないなど。
//50,000/day -> 34/min

#include <Nefry.h>
#include <NefryDisplay.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NefrySetting.h>
void setting() {
  Nefry.disableDisplayStatus();
}
NefrySetting nefrySetting(setting);

//mqtt
#define NEFRY_DATASTORE_BEEBOTTE_DICE 0
#define BBT "mqtt.beebotte.com"
#define QoS 0
String bbt_token;
#define Channel "Dice"
#define Res "act"
char topic[64];
WiFiClient espClient;
PubSubClient client(espClient);
String msg;

//date
#include <time.h>
#define JST     3600*9

//mpu6050
#include "MPU6050_Manage.h"
MPU6050_Manage mpu_main;
bool isCalibration; //Calibration ON/OFF
int CalOfs[4] = { -263, -36, -13, 1149}; //Gyro x,y,z, Accel z
float mpu6050_Quaternion[4];  //[w,x,y,z]

//NefryDisplayMessage
String MsgMqtt;
String MsgMpu6050;
String ipStr; //ipアドレス

//ループ周期(us)
#include <interval.h>
#define LOOPTIME_MPU6050 10000
#define LOOPTIME_SEND 30000

//connect mqtt broker
void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  //NefryのDataStoreに書き込んだToken(String)を(const char*)に変換
  bbt_token = "token:";
  bbt_token += Nefry.getStoreStr(NEFRY_DATASTORE_BEEBOTTE_DICE);
  const char* tmp = bbt_token.c_str();
  // Attempt to connect
  if (client.connect(clientId.c_str(), tmp, "")) {
    Serial.println("connected");
    MsgMqtt = "Mqtt Connected";
  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());
    MsgMqtt = "Mqtt DisConnected";
  }
}

void DispNefryDisplay() {
  NefryDisplay.clear();
  //取得したデータをディスプレイに表示
  NefryDisplay.setFont(ArialMT_Plain_10);
  NefryDisplay.drawString(0, 0, ipStr);
  NefryDisplay.drawString(0, 10, MsgMqtt);
  NefryDisplay.drawString(0, 20, MsgMpu6050);
  NefryDisplay.drawString(0, 30, msg);

  NefryDisplay.display();
  Nefry.ndelay(10);
}

void setup() {
  Nefry.setProgramName("DiceSwitch");

  NefryDisplay.begin();
  NefryDisplay.setAutoScrollFlg(true);//自動スクロールを有効

  NefryDisplay.clear();
  NefryDisplay.display();
  Nefry.ndelay(10);

  NefryDisplay.autoScrollFunc(DispNefryDisplay);

  //mqtt
  client.setServer(BBT, 1883);
  sprintf(topic, "%s/%s", Channel, Res);
  Nefry.setStoreTitle("Dice_Token", NEFRY_DATASTORE_BEEBOTTE_DICE);

  //mpu6050
  //キャリブレーションする必要ない場合は指定したオフセットを渡す
  isCalibration = false;
  mpu_main.init(isCalibration, CalOfs);

  //date
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  //displayMessage
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
}

void loop() {
  if (!client.connected()) reconnect();

  //mpu6050のデータを解析
  interval<LOOPTIME_MPU6050>::run([] {
    mpu_main.updateValue();
    mpu_main.Get_Quaternion(mpu6050_Quaternion);
    msg = "";
    MsgMpu6050 = mpu_main.GetErrMsg();
  });

  //データ送信
  interval<LOOPTIME_SEND>::run([] {
    if (client.connected()) {
      client.loop();
      publish(Res, msg);
    }
  });
}


void publish(const char* resource, String data)
{
  StaticJsonBuffer<128> jsonOutBuffer;
  JsonObject& root = jsonOutBuffer.createObject();
  root["data"] = data;
  root["ispublic"] = true;
  root["ts"] = time(NULL);

  // Now print the JSON into a char buffer
  char buffer[128];
  root.printTo(buffer, sizeof(buffer));

  // Now publish the char buffer to Beebotte
  client.publish(topic, buffer, QoS);
}

