
#include "pins_arduino.h"
#define spi
#include <SPI.h>

#define LDAC   12              // ラッチ動作出力ピン

#include "Arduino_DAC.h"


int A0_val = 0;
int def_A = 2047;
int def_B = 2047;
int tmp_A = def_A;
int tmp_B = def_B;

int fwd = 3000;
int bck = 500;
int rgt = 200;
int lft = 3900;

// Jcodeクラス 現在の値，待機している値
Jcode current;
Jcode queue;

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
        current.set(str);
        current.show_raw();
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
  //
  // j0000000000010000x
  // j1010000100010000x
  // j1110001100010000x
  // j2020000000010000x
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
  //  Serial.println(velo);
  //  Serial.println(cros);
  //  Serial.println(dely);
  Serial.println(mode_num);
  Serial.println(velo_num);
  Serial.println(cros_num);
  Serial.println(dely_num);

  long escape_time = millis() + dely_num;

  do {
    transmit(mode_num, 0, def_A + cros_num);
    transmit(mode_num, 1, def_B + velo_num);
  } while ( (escape_time > millis() ) && (Serial.available() <= 0) ) ;
  Serial.println("exit");
}


