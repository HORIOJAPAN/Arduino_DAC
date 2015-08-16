
#include "pins_arduino.h"
#include <SPI.h>

#define LDAC   12              // ラッチ動作出力ピン

//int low_val = 1247;
//int high_val = 2847;

int A0_val = 0;
int def_A = 2047;
int def_B = 2047;
int tmp_A = def_A;
int tmp_B = def_B;

int fwd = 3000;
int bck = 500;
int rgt = 200;
int lft = 3900;

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

  transmit(0, def_A);
  transmit(1, def_B);
}


void loop() {
  if (digitalRead(3) != digitalRead(6)) {
    if (digitalRead(3) == LOW)  transmit(0, lft);
    if (digitalRead(6) == LOW)  transmit(0, rgt);
  } else {
    transmit(0, def_A);
  }
  if (digitalRead(4) != digitalRead(5)) {
    if (digitalRead(4) == LOW)  transmit(1, fwd);
    if (digitalRead(5) == LOW)  transmit(1, bck);
  } else {
    transmit(1, def_B);
  }
}

void transmit_r(int velocity, int radius) {
  /*
  // roomba的動作をエミュレートする関数
  // 速度値を(-500 - 500 mm/s)で指定
  // 半径値を(-2000 - 2000 mm)で指定
  // 半径値が 0 のとき，直進・後進
  // 半径値が -1 のとき，時計周りにその場で回転
  // 半径値が 1 のとき，反時計回りにその場で回転
  // 半径値が負のとき，右を回転中心として回転
  // 半径値が正のとき，左を回転中心として回転
  */

  int DA_vA = def_A;
  int DA_vB = def_B;

  if (radius == 0) {
    DA_vB = def_B + velocity / 10 * 10;
    transmit(0, def_A);
    transmit(1, DA_vB);
  } else if (radius == -1) {
    DA_vA = def_A - velocity / 10 * 10; // 角速度右回り
    transmit(0, DA_vA);
    transmit(1, def_B);
  } else if (radius == 1) {
    DA_vA = def_A + velocity / 10 * 10; // 角速度左回り
    transmit(0, DA_vA);
    transmit(1, def_B);
  } else if (abs(radius) < 250){ // 角速度
    i++;
  } else { // 速度
    
  }
}

void transmit(int ch, int DA_value) { //ch 0:A(ヨコ) 1:B(タテ)
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
  SPI.begin() ;
  if (ch == 0) {
    digitalWrite(LDAC, HIGH) ;
    digitalWrite(SS, LOW) ;
    SPI.transfer(0b01110000 | (0b00001111 & highByte(tmp_A)));
    SPI.transfer(lowByte(tmp_A));
    digitalWrite(SS, HIGH) ;
    digitalWrite(LDAC, LOW) ;       // ラッチ信号を出す
  } else if (ch == 1) {
    digitalWrite(LDAC, HIGH) ;
    digitalWrite(SS, LOW) ;
    SPI.transfer(0b11110000 | (0b00001111 & highByte(tmp_B)));
    SPI.transfer(lowByte(tmp_B));
    digitalWrite(SS, HIGH) ;
    digitalWrite(LDAC, LOW) ;       // ラッチ信号を出す
  }
  SPI.end() ;
  delay(5);
}

