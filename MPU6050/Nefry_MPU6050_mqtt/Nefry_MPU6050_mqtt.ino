//VCC -> 5v
//GND -> GND
//SCL -> A5
//SDA -> A4
#include <Nefry.h>
#include <NefryDisplay.h>
#include <NefrySetting.h>
void setting(){
  Nefry.disableDisplayStatus();
  //Nefry.disableWifi();
}
NefrySetting nefrySetting(setting);

#include "MPU6050_Manage.h"
MPU6050_Manage mpu_main;

//Calibration ON/OFF
bool isCalibration;

//MPU6050の初期化時に使用するオフセット
//CalibrationがOFFの時に適用される
int CalOfs[4] = {0, 0, 0, 0}; //Gyro x,y,z, Accel z

//MPU6050から取得するデータ
float mpu6050_EulerAngle[3];  //[x,y,z]
float mpu6050_Quaternion[4];  //[w,x,y,z]
int mpu6050_RealAccel[3];        //[x,y,z]
int mpu6050_WorldAccel[3];       //[x,y,z]
uint8_t mpu6050_teapotPacket[14];

const unsigned int LOOP_TIME_US = 20000;  //ループ関数の周期(μsec)
int processingTime; //loopの頭から最後までの処理時間

void DispNefryDisplay() {
  NefryDisplay.clear();
  String text;
  //取得したデータをディスプレイに表示
  NefryDisplay.setFont(ArialMT_Plain_10);

  NefryDisplay.drawString(0, 0, mpu_main.GetErrMsg());

  text = "[Ofs]";
  text +=String(CalOfs[0]);
  text += ",";
  text +=String(CalOfs[1]);
  text += ",";
  text +=String(CalOfs[2]);
  text += ",";
  text +=String(CalOfs[3]);
  NefryDisplay.drawString(0, 10, text);
  
  NefryDisplay.drawString(0, 20, "[ANGLE]");
  NefryDisplay.drawString(0, 30, "X : ");
  NefryDisplay.drawString(15, 30, String(mpu6050_EulerAngle[0]));
  NefryDisplay.drawString(0, 40, "Y : ");
  NefryDisplay.drawString(15, 40, String(mpu6050_EulerAngle[1]));
  NefryDisplay.drawString(0, 50, "Z : ");
  NefryDisplay.drawString(15, 50, String(mpu6050_EulerAngle[2]));

  NefryDisplay.display();
  Nefry.ndelay(20);
}

void setup() {
  NefryDisplay.begin();
  NefryDisplay.setAutoScrollFlg(true);//自動スクロールを有効
  
  NefryDisplay.clear();
  NefryDisplay.display();
  Nefry.ndelay(10);
  
  //キャリブレーションする必要ない場合は指定したオフセットを渡す
  isCalibration = false;
  CalOfs[0] = -263;
  CalOfs[1] = -36;
  CalOfs[2] = -13;
  CalOfs[3] = 1149;
  mpu_main.init(isCalibration, CalOfs);
  
}

void loop() {
  processingTime = micros();
  mpu_main.updateValue();

  mpu_main.Get_EulerAngle(mpu6050_EulerAngle);
  mpu_main.Get_Quaternion(mpu6050_Quaternion);
  mpu_main.Get_RealAccel(mpu6050_RealAccel);
  mpu_main.Get_WorldAccel(mpu6050_WorldAccel);
  mpu_main.Get_teapotPacket(mpu6050_teapotPacket);

  NefryDisplay.autoScrollFunc(DispNefryDisplay);

  //一連の処理にかかった時間を考慮して待ち時間を設定する
  wait_ConstantLoop();
}

void wait_ConstantLoop() {
  processingTime = micros() - processingTime;
  long loopWaitTime = LOOP_TIME_US - processingTime;

  if (loopWaitTime < 0)  return;

  long start_time = micros();
  while ( micros() - start_time < loopWaitTime) {};
}