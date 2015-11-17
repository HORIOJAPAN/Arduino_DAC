
#include "pins_arduino.h"
#define spi
#include <SPI.h>

#define LDAC   12              // ラッチ動作出力ピン

bool serial_report = false; // デバッグ用のフラッグ（受信値のシリアル報告）

#include "Arduino_DAC.h"

/*
// シリアルバッファが16バイトである場合にバッファを変数に格納
// [mode] [forward/velocity] [crosswise/radius] [delay]
// [0] [0 0000] [0 0000] [00000]
// [mode]
// 0 : 即時停止
// 1 : 電圧のシリアル値（相対値，スムージングなし）
// 2 : 電圧のシリアル値（相対値，スムージングあり）
// 3 : ルンバ式シリアル値
//
// [mode == 0]
// 続く15バイトを無視して電圧を標準値にする
// 滑らかに停止させる場合，[2 00000 00000 xxxxx]
//
// [mode == 1 || mode == 2] [forward] [crosswise]
// -4095 ~ 04095
// 基準電圧 + (-5.00 ~ 5.00) [V]
// ただし，結果が 0 ~ 4095 を逸脱しない
// 符号は0かそれ以外かで判別する
//
// [mode == 3] [velocity] [radius]
// -9999 ~ 09999
// 速度は[mm/s]，半径は[mm]
// ±5000を上限の目安にする
// 後ろ向きに走行する場合，速度が負のとき進行方向，半径が負のとき進行方向左回り
// 符号は0かそれ以外かで判別する
//
// [delay]
// 00000 ~ 99999
// 受信したシリアル値を出力してからdelay[ms]維持する
// その後，新しい指令値がない場合は停止
// 割り込む場合は[mode == 0]で停止させてから新しいデータを送信する
*/

int A0_val = 0;
int def_A = 2047;
int def_B = 2047;
int tmp_A = def_A;
int tmp_B = def_B;

int fwd = 3000;
int bck = 500;
int rgt = 200;
int lft = 3900;

bool fwdState = false;
bool bckState = false;
bool rgtState = false;
bool lftState = false;

// Jcodeクラス
Jcode JoystickData;

int fwd_B, bck_B, rgt_B, lft_B;
int fwd_A, bck_A, rgt_A, lft_A;

void setup() {
  // 物理ボタンピン
  pinMode(3, INPUT_PULLUP );
  pinMode(4, INPUT_PULLUP );
  pinMode(5, INPUT_PULLUP );
  pinMode(6, INPUT_PULLUP );

  // ラッチ動作出力ピン
  pinMode(LDAC, OUTPUT) ;

  // SPIの初期化
  SPI.setBitOrder(MSBFIRST) ;          // ビットオーダー
  SPI.setClockDivider(SPI_CLOCK_DIV8) ;// クロック(CLK)をシステムクロックの1/8で使用(16MHz/8)
  SPI.setDataMode(SPI_MODE0) ;         // クロック極性０(LOW)　クロック位相０

  // 基準電圧の取得（入力は10bit，出力は12bit）
  A0_val = 0;

  for(int i = 0;i < 64;i++){
    A0_val += analogRead(A0);
    delay(1);
  }
  // 0 - 32736

  A0_val /= 16;

  // 0 - 4092

  // 出力目標値の設定
  def_A = def_B = A0_val;

  fwd_B = def_B - 1500;
  bck_B = def_B + 1500;
  rgt_B = no_order;
  lft_B = no_order;

  fwd_A = no_order;
  bck_A = no_order;
  rgt_A = def_A - 1500;
  lft_A = def_A + 1500;

  // スムージングなしでデフォルト値の出力
  transmit(1, def_A, def_B, 0);

  // シリアル通信のタイムアウトを5msに（デフォルトは1000ms）
  Serial.begin(9600);
  Serial.setTimeout(5);
  Serial.print("start_DAC ");
  Serial.println(A0_val);
}


void loop() {
  while (JoystickData.check() == true) {
    //JoystickData.show_raw();
    JoystickData.convert();
    JoystickData.show_num();
    JoystickData.echo();
  }

  // push_down => LOW : 0 : false
  fwdState = digitalRead(4);
  bckState = digitalRead(5);
  rgtState = digitalRead(6);
  lftState = digitalRead(3);

  // シリアル通信がないとき，物理ボタンの状態を参照
  if (digitalRead(3) != digitalRead(6)) {
    if (digitalRead(3) == LOW)  transmit(2, lft, no_order, 0);
    if (digitalRead(6) == LOW)  transmit(2, rgt, no_order, 0);
  } else {
    transmit(1, def_A, no_order, 0);
  }
  if (digitalRead(4) != digitalRead(5)) {
    if (digitalRead(4) == LOW)  transmit(2, no_order, fwd, 0);
    if (digitalRead(5) == LOW)  transmit(2, no_order, bck, 0);
  } else {
    transmit(1, no_order, def_B, 0);
  }
}


