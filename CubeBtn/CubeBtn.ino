//磁気センサーを用いた、キューブ型ボタン

#include <Nefry.h>
#include <NefryDisplay.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NefrySetting.h>
void setting() {
  Nefry.disableDisplayStatus();
}
NefrySetting nefrySetting(setting);


//++++++++++++++++++++++++++++++++++++++++++++
//プロジェクト全体の定義
//++++++++++++++++++++++++++++++++++++++++++++
//使用ピン
#define PIN_JIKI A3
#define PIN_BTN D8
#define PIN_LED_WAIT D0
#define PIN_LED_PTN1 D1
#define PIN_LED_PTN2 D2
#define PIN_LED_PTN3 D3
#define PIN_LED_PTN4 D4
#define PIN_LED_PTN5 D5
#define PIN_LED_PTN6 D6

//ループ周期(us)
#include "interval.h"
#define LOOPTIME_DISP  100000
#define LOOPTIME_JIKI  10000
#define LOOPTIME_BTN   100000
#define LOOPTIME_LED   100000
#define LOOPTIME_MQTT  10000

//ステータス
#define STATUS_NONE "NONE"
#define STATUS_TAIKI "Wait"
#define STATUS_BTN_ON "Press"
#define STATUS_MQTT_SUC "Send"
String nowStatus = STATUS_NONE;

//MQTT送信中の待ち時間(ms)
#define WAIT_MQTT_PUBLISH 5000
unsigned long waitingTime;

//++++++++++++++++++++++++++++++++++++++++++++
//磁気センサ
//++++++++++++++++++++++++++++++++++++++++++++
#define JIKI_NONE 0
#define JIKI_PTN1 1
#define JIKI_PTN2 2
#define JIKI_PTN3 3
#define JIKI_PTN4 4
#define JIKI_PTN5 5
#define JIKI_PTN6 6

//閾値
int jikiPtnNone[2] = {490, 510};
int jikiPtn1[2] = {490, 510};
int jikiPtn2[2] = {490, 510};
int jikiPtn3[2] = {490, 510};
int jikiPtn4[2] = {490, 510};
int jikiPtn5[2] = {490, 510};
int jikiPtn6[2] = {490, 510};

//保存するデータ数は1秒分
#define JIKI_SIZE (1000000 / LOOPTIME_JIKI)
int jiki[JIKI_SIZE];
int jikiAvg;
int jikiPtn = JIKI_NONE;

//++++++++++++++++++++++++++++++++++++++++++++
//ボタン
//++++++++++++++++++++++++++++++++++++++++++++
#define BTN_OFF 0
#define BTN_DOWN 1
int btnPtn = BTN_OFF;

//++++++++++++++++++++++++++++++++++++++++++++
//ディスプレイ
//++++++++++++++++++++++++++++++++++++++++++++
String ipStr;
String MsgMqtt;
String MsgPublishData;

//++++++++++++++++++++++++++++++++++++++++++++
//折れ線グラフ
//++++++++++++++++++++++++++++++++++++++++++++
#include "dispGraphLine.h"
//static変数を定義
int graph_line::valueSIZE;

//折れ線グラフの領域
#define GRAPH_LINE_POS_X 27
#define GRAPH_LINE_POS_Y 30
#define GRAPH_LINE_LEN_X 100
#define GRAPH_LINE_LEN_Y 30
#define GRAPH_LEN_DPP 10 //点をプロットする間隔(1なら1ドットにつき1点、2なら2ドットにつき1点)

#define VALUE_LINE_MIN 0
#define VALUE_LINE_MAX 1023
#define LINE_PLOT_SIZE (GRAPH_LINE_LEN_X / GRAPH_LEN_DPP) + 1
int x[LINE_PLOT_SIZE];  //x座標
int t[LINE_PLOT_SIZE];  //一定間隔に垂線を引くための配列(垂線の有無)
graph_line grline = graph_line(
                      LOOPTIME_DISP,
                      GRAPH_LINE_POS_X,
                      GRAPH_LINE_POS_Y,
                      GRAPH_LINE_LEN_X,
                      GRAPH_LINE_LEN_Y,
                      GRAPH_LEN_DPP,
                      VALUE_LINE_MIN,
                      VALUE_LINE_MAX,
                      LINE_PLOT_SIZE,
                      &x[0],
                      &t[0]
                    );

//グラフの設定
//頂点の数が可変なので外部で配列を用意している
int v1[LINE_PLOT_SIZE]; //頂点の値

//グラフの初期化
void dispGraphLine_init() {
  grline.initGraphTime();
  grline.setGraph(0, &v1[0], VERTEX_CIR);
}

//グラフの描画
void dispGraphLine_update() {
  grline.dispArea();
  grline.updateGraph();
}

//++++++++++++++++++++++++++++++++++++++++++++
//MQTT
//++++++++++++++++++++++++++++++++++++++++++++
#define NEFRY_DATASTORE_BEEBOTTE_CUBEBTN 1
#define BBT "mqtt.beebotte.com"
#define QoS 1
String bbt_token;
#define Channel "CubeBtn"
#define Res "ptn"
char topic[64];
WiFiClient espClient;
PubSubClient client(espClient);

//connect mqtt broker
void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP32Client-";
  clientId += String(random(0xffff), HEX);

  //NefryのDataStoreに書き込んだToken(String)を(const char*)に変換
  bbt_token = "token:";
  bbt_token += Nefry.getStoreStr(NEFRY_DATASTORE_BEEBOTTE_CUBEBTN);
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

void publish()
{
  //日付を取得する
  time_t  t = time(NULL);

  StaticJsonBuffer<128> jsonOutBuffer;
  JsonObject& root = jsonOutBuffer.createObject();

  root["ptn"] = jikiPtn;
  root["ispublic"] = true;
  root["ts"] = t;

  // Now print the JSON into a char buffer
  char buffer[128];
  root.printTo(buffer, sizeof(buffer));

  MsgPublishData = String(buffer);

  // Now publish the char buffer to Beebotte
  client.publish(topic, buffer, QoS);
}

//date
#include <time.h>
#define JST     3600*9






void setup() {
  Nefry.setProgramName("Trigger CubeButton");

  NefryDisplay.begin();
  NefryDisplay.setAutoScrollFlg(true);//自動スクロールを有効

  NefryDisplay.clear();
  NefryDisplay.display();
  Nefry.ndelay(10);

  //pin
  pinMode(PIN_JIKI, INPUT);
  pinMode(PIN_BTN, INPUT);
  pinMode(PIN_LED_WAIT, OUTPUT);
  pinMode(PIN_LED_PTN1, OUTPUT);
  pinMode(PIN_LED_PTN2, OUTPUT);
  pinMode(PIN_LED_PTN3, OUTPUT);
  pinMode(PIN_LED_PTN4, OUTPUT);
  pinMode(PIN_LED_PTN5, OUTPUT);
  pinMode(PIN_LED_PTN6, OUTPUT);

  //mqtt
  client.setServer(BBT, 1883);
  sprintf(topic, "%s/%s", Channel, Res);
  Nefry.setStoreTitle("Token_CubeButton", NEFRY_DATASTORE_BEEBOTTE_CUBEBTN);

  //date
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  //IPアドレス(ディスプレイに表示)
  IPAddress ip = WiFi.localIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
}





void loopDisplay() {
  //グラフの更新
  grline.addGraphData(0, jiki[JIKI_SIZE - 1]);
  grline.setAvg(0, jikiAvg);
  grline.updateGraphTime();

  //描画
  NefryDisplay.clear();

  NefryDisplay.setFont(ArialMT_Plain_10);
  NefryDisplay.drawString(0, 0, ipStr);
  NefryDisplay.drawString(0, 10, MsgMqtt);
  NefryDisplay.drawString(0, 20, String(jikiPtn));
  NefryDisplay.drawString(15, 20, nowStatus);
  //  NefryDisplay.drawString(0, 30, MsgPublishData);

  grline.dispArea();
  grline.updateGraph();

  NefryDisplay.display();
}

void loopJikiSensor() {
  //データ保存と平均値の算出
  jikiAvg = 0;
  for (int i = 0; i < JIKI_SIZE - 1; i++) {
    jiki[i] = jiki[i + 1];
    jikiAvg += jiki[i + 1];
  }
  jiki[JIKI_SIZE - 1] = analogRead(PIN_JIKI);
  jikiAvg += jiki[JIKI_SIZE - 1];
  jikiAvg /= JIKI_SIZE;

  //パターンの判定
  if (jikiAvg >= jikiPtnNone[0] && jikiAvg <= jikiPtnNone[1]) {
    jikiPtn = JIKI_NONE;
    return;
  }
  if (jikiAvg >= jikiPtn1[0] && jikiAvg <= jikiPtn1[1]) {
    jikiPtn = JIKI_PTN1;
    return;
  }
  if (jikiAvg >= jikiPtn2[0] && jikiAvg <= jikiPtn2[1]) {
    jikiPtn = JIKI_PTN2;
    return;
  }
  if (jikiAvg >= jikiPtn3[0] && jikiAvg <= jikiPtn3[1]) {
    jikiPtn = JIKI_PTN3;
    return;
  }
  if (jikiAvg >= jikiPtn4[0] && jikiAvg <= jikiPtn4[1]) {
    jikiPtn = JIKI_PTN4;
    return;
  }
  if (jikiAvg >= jikiPtn5[0] && jikiAvg <= jikiPtn5[1]) {
    jikiPtn = JIKI_PTN5;
    return;
  }
  if (jikiAvg >= jikiPtn6[0] && jikiAvg <= jikiPtn6[1]) {
    jikiPtn = JIKI_PTN6;
    return;
  }

  //どの条件にも引っかからなかった
  jikiPtn = JIKI_NONE;
  return;
}

void loopBtn() {
  bool btn = digitalRead(PIN_BTN);
  if (btn) {
    Nefry.setLed(255, 128, 0);
  } else {
    Nefry.setLed(0, 0, 0);
  }

  //MQTT送信中の時はボタンを無効にする
  if (nowStatus == STATUS_MQTT_SUC) return;

  if (btnPtn == BTN_OFF && btn) {
    btnPtn = BTN_DOWN;
    return;
  }

  if (btnPtn == BTN_DOWN && !btn) {
    btnPtn = BTN_OFF;
    nowStatus = STATUS_BTN_ON;
    return;
  }

}

void loopLED() {
  if (nowStatus == STATUS_MQTT_SUC) {
    digitalWrite(PIN_LED_WAIT, HIGH);
  } else {
    digitalWrite(PIN_LED_WAIT, LOW);
  }

  digitalWrite(PIN_LED_PTN1, LOW);
  digitalWrite(PIN_LED_PTN2, LOW);
  digitalWrite(PIN_LED_PTN3, LOW);
  digitalWrite(PIN_LED_PTN4, LOW);
  digitalWrite(PIN_LED_PTN5, LOW);
  digitalWrite(PIN_LED_PTN6, LOW);

  switch (jikiPtn) {
    case JIKI_PTN1:
      digitalWrite(PIN_LED_PTN1, HIGH);
      break;
    case JIKI_PTN2:
      digitalWrite(PIN_LED_PTN2, HIGH);
      break;
    case JIKI_PTN3:
      digitalWrite(PIN_LED_PTN3, HIGH);
      break;
    case JIKI_PTN4:
      digitalWrite(PIN_LED_PTN4, HIGH);
      break;
    case JIKI_PTN5:
      digitalWrite(PIN_LED_PTN5, HIGH);
      break;
    case JIKI_PTN6:
      digitalWrite(PIN_LED_PTN6, HIGH);
      break;
  }

}

void loopMQTT() {
  //送信中の待ち
  if (nowStatus == STATUS_MQTT_SUC) {
    unsigned long _t = millis();
    if ( abs(_t - waitingTime) >= WAIT_MQTT_PUBLISH) {
      nowStatus = STATUS_TAIKI;
    }
    return;
  }

  //ボタンを押した直後
  if (nowStatus == STATUS_BTN_ON) {

    //どの面が上なのか分からない
    if (jikiPtn == JIKI_NONE) {
      nowStatus = STATUS_TAIKI;
      return;
    }

    publish();
    nowStatus = STATUS_MQTT_SUC;
    waitingTime = millis();
  }
}


void loop() {
  if (!client.connected()) reconnect();

  //Display
  interval<LOOPTIME_DISP>::run([] {
    loopDisplay();
  });

  //JikiSensor
  interval<LOOPTIME_JIKI>::run([] {
    loopJikiSensor();
  });

  //Button
  interval<LOOPTIME_BTN>::run([] {
    loopBtn();
  });

  //LED
  interval<LOOPTIME_LED>::run([] {
    loopLED();
  });

  //MQTT publish
  interval<LOOPTIME_MQTT>::run([] {
    loopMQTT();
  });
}
