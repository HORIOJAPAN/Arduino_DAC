
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
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);
  delay(1);
  A0_val += analogRead(A0);

  // 出力目標値の設定
  def_A = def_B = A0_val;
  fwd = def_B + 1300;
  bck = def_B - 1500;
  rgt = def_A - 1500;
  lft = def_A + 1500;

  // スムージングなしでデフォルト値の出力
  transmit(1, 0, def_A);
  transmit(1, 1, def_B);

  // シリアル通信のタイムアウトを5msに（デフォルトは1000ms）
  Serial.begin(9600);
  Serial.setTimeout(5);
  Serial.println("start_DAC");
}


void loop() {
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
  char str[40] = {0};
  while (Serial.available() > 0) {
    if (Serial.read() == 'j') {
      if (Serial.readBytesUntil('x', str, 40) == 16) {
        convert(str);
        // バッファがあるとき'j'が検出できるまで読み取り
        // jの次から文字列の終端かxの直前までが16字の場合のみ動作
      }
    }
  }

  // シリアル通信がないとき，物理ボタンの状態を参照
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


void convert(char *str) {
  /*
  // 取得した文字列の内，データ部分を数値に変換する
  // 数値化した指令値をそのままSPIに指示する
  // 待機継続時間は5桁なのでlong型
  // j8012340678955555x
  // j1143211987600000x
  // j0000000000010000x
  */
  char mode[2];
  char velo[5];
  char cros[5];
  char dely[6];

  mode[0] = str[0];
  mode[1] = '\0';

  velo[0] = str[2];
  velo[1] = str[3];
  velo[2] = str[4];
  velo[3] = str[5];
  velo[4] = '\0';

  cros[0] = str[7];
  cros[1] = str[8];
  cros[2] = str[9];
  cros[3] = str[10];
  cros[4] = '\0';

  dely[0] = str[11];
  dely[1] = str[12];
  dely[2] = str[13];
  dely[3] = str[14];
  dely[4] = str[15];
  dely[5] = '\0';

  int mode_num;
  int velo_num;
  int cros_num;
  long dely_num;

  int velo_pm = 1;
  int cros_pm = 1;

  if (str[1] != '0') velo_pm = -1;
  if (str[6] != '0') cros_pm = -1;

  mode_num = atoi(mode);
  velo_num = velo_pm * atoi(velo);
  cros_num = cros_pm * atoi(cros);
  dely_num = atol(dely);

  Serial.println(str);
  //  Serial.println(mode);
  //  Serial.println(mode_num);
  //  Serial.println(velo);
  //  Serial.println(velo_num);
  //  Serial.println(cros);
  //  Serial.println(cros_num);
  //  Serial.println(dely);
  //  Serial.println(dely_num);

  int escape_time = millis() + dely_num;

  do while ( (escape_time > millis() ) && (Serial.available() <= 0) ) {
    transmit(mode_num, velo_num, cros_num);
  }
}


void transmit(int mode, int ch, int DA_value) {
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
  int tmp_val;

  switch (mode) {
    case 0:
      // 即時停止
      spi_transmit(0, def_A);
      spi_transmit(1, def_B);
      tmp_A = def_A;
      tmp_B = def_B;
      break;

    case 1:
      // スムージングなし
      if (ch == 0) tmp_A = DA_value;
      else if (ch == 1) tmp_B = DA_value;
      spi_transmit(ch, DA_value);
      break;

    case 2:
      // スムージングあり
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

