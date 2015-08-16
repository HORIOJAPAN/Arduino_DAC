
#include "pins_arduino.h"
#include <SPI.h>

#define LDAC   12              // ラッチ動作出力ピン

int A0_val = 0;
int def_A = 2047;
int def_B = 2047;
int tmp_A = def_A;
int tmp_B = def_B;

int fwd = 3000;
int bck = 500;
int rgt = 200;
int lft = 3900;

char *inBytes;


void setup() {
  pinMode(3, INPUT_PULLUP );
  pinMode(4, INPUT_PULLUP );
  pinMode(5, INPUT_PULLUP );
  pinMode(6, INPUT_PULLUP );

  // 制御するピンは全て出力に設定する
  pinMode(LDAC, OUTPUT) ;
  // SPIの初期化処理を行う
  SPI.setBitOrder(MSBFIRST) ;          // ビットオーダー
  SPI.setClockDivider(SPI_CLOCK_DIV8) ;// クロック(CLK)をシステムクロックの1/8で使用(16MHz/8)
  SPI.setDataMode(SPI_MODE0) ;         // クロック極性０(LOW)　クロック位相０

  A0_val = 0;
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);

  def_A = def_B = A0_val;
  fwd = def_B + 1300;
  bck = def_B - 1500;
  rgt = def_A - 1500;
  lft = def_A + 1500;

  transmit(1, 0, def_A);
  transmit(1, 1, def_B);

  Serial.begin(9600);
  Serial.println("start_DAC");
}


void loop() {
  if (Serial.readBytes(inBytes, 16) != 0) {
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
    //a = inBytes;
    /*　// ↓このへんを要検討
    char mode_char = inByte.substring(0, 1);
    char fwd_char = inByte.substring(2, 6);
    char crs_char = inByte.substring(7, 11);
    char delay_char = inByte.substring(11);

    mode_val = atoi(mode_char);
    fwd_val = atoi(fwd_char);
    crs_val = atoi(crs_char);
    delay_val = atoi(delay_char);

    if (getBytes(inBytes, 1, 1) != 0) fwd_val = -fwd_val;
    if (getBytes(inBytes, 1, 6) != 0) fwd_val = -fwd_val;

    Serial.println(1);

    transmit(mode_val, fwd_val, crs_val);
*/
  }
  else { // シリアル値がある場合は送信しない
    if (digitalRead(3) != digitalRead(6)) {
      if (digitalRead(3) == LOW)  transmit(2, 0, lft);
      if (digitalRead(6) == LOW)  transmit(2, 0, rgt);
    } else {
      transmit(2, 0, def_A);
    }
    if (digitalRead(4) != digitalRead(5)) {
      if (digitalRead(4) == LOW)  transmit(2, 1, fwd);
      if (digitalRead(5) == LOW)  transmit(2, 1, bck);
    } else {
      transmit(2, 1, def_B);
    }
  }
}

void transmit(int mode, int ch, int DA_value) {
  int tmp_val;
  /*
  // mode
  // 0: 即時停止
  // 1: スムージングなし
  // 2: スムージングあり
  // 3: ルンバ的動作（開発中）
  // 
  // ch 0:A(ヨコ) 1:B(タテ)
  // 基準値から±0.6Vは反応しないので，反応速度を高めるため
  // シリアル値で450だけ飛ばす
  // スムージングする場合，ヨコは30ずつ，タテは20ずつ加減算
  // tmp_A, tmp_Bはグローバル変数で現在のシリアル値を保持
  */
  switch (mode) {
    case 0:// 即時停止
      spi_transmit(0, def_A);
      spi_transmit(1, def_B);
      tmp_A = def_A;
      tmp_B = def_B;
      break;

    case 1:// スムージングなし
      if (ch == 0) tmp_A = DA_value;
      else if (ch == 1) tmp_B = DA_value;
      spi_transmit(ch, DA_value);
      break;

    case 2:// スムージングあり
      if ((ch == 0) && (tmp_A != DA_value)) {
        if (tmp_A == def_A) {
          if (tmp_A < DA_value) tmp_A += 450;
          else if (tmp_A > DA_value) tmp_A -= 450;
        }
        if (abs(tmp_A - DA_value) < 60) tmp_A = DA_value;
        else if (tmp_A < DA_value) tmp_A += 30;
        else if (tmp_A > DA_value) tmp_A -= 30;
      } else if ((ch == 1) && (tmp_B != DA_value)) {
        if (tmp_B == def_B) {
          if (tmp_B < DA_value) tmp_B += 450;
          else if (tmp_B > DA_value) tmp_B -= 450;
        }
        if (abs(tmp_B - DA_value) < 40) tmp_B = DA_value;
        else if (tmp_B < DA_value) tmp_B += 20;
        else if (tmp_B > DA_value) tmp_B -= 20;
      }
      if (ch == 0)  spi_transmit(0, tmp_A);
      else if (ch == 1)  spi_transmit(1, tmp_B);
      break;

    case 3:
      /*
      // ルンバ的動作をエミュレートする関数
      // 速度値を(-500 - 500 mm/s)で指定
      // 半径値を(-2000 - 2000 mm)で指定
      // 半径値が 0 のとき，直進・後進
      // 半径値が -1 のとき，時計周りにその場で回転
      // 半径値が 1 のとき，反時計回りにその場で回転
      // 半径値が負のとき，右を回転中心として回転
      // 半径値が正のとき，左を回転中心として回転
      */

      int velocity = ch;
      int radius = DA_value;
      int DA_vA = def_A;
      int DA_vB = def_B;

      if (radius == 0) {
        DA_vB = def_B + velocity / 10 * 10;
        spi_transmit(0, def_A);
        spi_transmit(1, DA_vB);
      } else if (radius == -1) {
        DA_vA = def_A - velocity / 10 * 10; // 角速度右回り
        spi_transmit(0, DA_vA);
        spi_transmit(1, def_B);
      } else if (radius == 1) {
        DA_vA = def_A + velocity / 10 * 10; // 角速度左回り
        spi_transmit(0, DA_vA);
        spi_transmit(1, def_B);
      } else if (abs(radius) < 250) { // 角速度
        while (0);
      } else { // 速度
        while (0);
      }
      break;
  }
}

void spi_transmit(int ch, int tmp) {
  SPI.begin() ;
  if (ch == 0) {
    digitalWrite(LDAC, HIGH) ;
    digitalWrite(SS, LOW) ;
    SPI.transfer(0b01110000 | (0b00001111 & highByte(tmp)));
    SPI.transfer(lowByte(tmp));
    digitalWrite(SS, HIGH) ;
    digitalWrite(LDAC, LOW) ;       // ラッチ信号を出す
  } else if (ch == 1) {
    digitalWrite(LDAC, HIGH) ;
    digitalWrite(SS, LOW) ;
    SPI.transfer(0b11110000 | (0b00001111 & highByte(tmp)));
    SPI.transfer(lowByte(tmp));
    digitalWrite(SS, HIGH) ;
    digitalWrite(LDAC, LOW) ;       // ラッチ信号を出す
  }
  SPI.end() ;
  delay(5);
}

