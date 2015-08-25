
#ifndef spi
#define spi
#include <SPI.h>
#endif

void transmit(int mode, int order1, int order2, int order3);
void spi_transmit(int ch, int tmp);

extern int def_A;
extern int def_B;
extern int tmp_A;
extern int tmp_B;

extern bool serial_report; // true:report  false:silent

/*=== class Jcode begin ===*/
class Jcode {
    char receive_raw[40];

    long escape_time;           // millis()と比較するための変数

    char mode[2];
    char data1[5];
    char data2[5];
    char data3[6];             // 分割した文字列を格納

    int mode_num;
    int data1_num;
    int data2_num;
    long data3_num;            // 数値化して格納

  public:
    int receive_mode;           // 待機のモードは外部参照可能
    bool receive_valid = false; // 待機が有効な場合true

    Jcode();

    void reset_dac();
    bool check();       // シリアルバッファのチェック，文字列格納，即時停止チェック
    void convert();     // 格納された文字列の数値化
    void show_raw();
    void show_div();
    void show_num();    // データのシリアル送信
    void echo();        // 維持時間の間，繰り返し処理
};

Jcode::Jcode() {
  ;
}

void Jcode::reset_dac() {
  transmit(0, 0, 0, 0);
  Serial.println("J_stop");
  escape_time = data3_num = 0;
  receive_valid = false;
}

bool Jcode::check() {
  /*
  // シリアルバッファのチェック，'j'の探索
  // 受信データをstrに格納，16バイトであればreceive_rawに複写
  // 受信データのmodeが'0'のとき即時停止
  // それ以外の時receive_valid = true
  // 受信データが有効な場合，関数としてtrueを返しSPIの送信処理に入る
  // 即時停止（escape_time初期化，ループ脱出）のときSPIの送信処理をしてからfalseを戻す
  */
  bool answer = false;
  while (Serial.available() > 0) {
    if (Serial.read() == 'j') {
      char str[40];
      if (Serial.readBytesUntil('x', str, 40) == 16) {
        for (int i = 0; i < 40; i++) {
          receive_raw[i] = str[i];
        }
        receive_raw[16] = '\0';

        if (receive_raw[0] == '0') {
          reset_dac();
          return false; // 待機データのmodeが0のとき即時停止して関数離脱
        }

        Serial.println("J_receive");
        show_raw();
        receive_valid = true;
        answer = true;
      }
    }
  }
  return answer;
}

void Jcode::convert() {
  mode[0] = receive_raw[0];
  mode[1] = '\0';

  data1[0] = receive_raw[2];
  data1[1] = receive_raw[3];
  data1[2] = receive_raw[4];
  data1[3] = receive_raw[5];
  data1[4] = '\0';

  data2[0] = receive_raw[7];
  data2[1] = receive_raw[8];
  data2[2] = receive_raw[9];
  data2[3] = receive_raw[10];
  data2[4] = '\0';

  data3[0] = receive_raw[11];
  data3[1] = receive_raw[12];
  data3[2] = receive_raw[13];
  data3[3] = receive_raw[14];
  data3[4] = receive_raw[15];
  data3[5] = '\0';

  mode_num = atoi(mode);
  // X = {A ? B : C} => Aが trueならB, falseならC （三項演算子）
  data1_num = (receive_raw[1] == '0') ? atoi(data1) : -atoi(data1);
  data2_num = (receive_raw[6] == '0') ? atoi(data2) : -atoi(data2);
  data3_num = atol(data3);
  // data3にマイナス値を許容するか？

  receive_valid = false;
}

void Jcode::show_raw() {
  if (!serial_report) return;
  Serial.println(receive_raw);
}

void Jcode::show_div() {
  if (!serial_report) return;
  Serial.println(mode);
  Serial.println(data1);
  Serial.println(data2);
  Serial.println(data3);
}

void Jcode::show_num() {
  if (!serial_report) return;
  Serial.println(mode_num);
  Serial.println(data1_num);
  Serial.println(data2_num);
  Serial.println(data3_num);
}

void Jcode::echo() {
  convert();
  Serial.println("J_convert");
  show_num();

  escape_time = millis() + data3_num;
  transmit(mode_num, (def_A + data2_num), (def_B + data1_num), 0);
  while (1) {
    if (escape_time >= millis()) { // 維持時間を超過しない
      check();
    } else { // 維持時間を超過した
      if (receive_valid == true) { // 待機データが有効である
        convert();   // 待機データを数値に変換，待機データの無効化
        Serial.println("J_convert");
        show_num();
        escape_time = millis() + data3_num;
        transmit(mode_num, (def_A + data2_num), (def_B + data1_num), 0);
      } else {       // 待機データが無効なら初期化して関数離脱
        reset_dac();
        Serial.println("J_exit");
        return;
      }
    }
  }
}
/*=== class Jcode end ===*/


// ブランク値判定のための定数
#define no_order -32768

// スムーズ化の刻み幅（左右，前後）
#define smt_pit_crs 30
#define smt_pit_fwd 20

void transmit(int mode, int order1, int order2, int order3) {
  /*
  // mode
  // 0: (0, , , )即時停止
  // 1: (1, ﾖｺA, ﾀﾃB, )スムージングなし
  // 2: (2, ﾖｺA, ﾀﾃB, )スムージングあり
  // 3: (3, vel, rad, )ルンバ的動作（開発中）
  // 4: (4,    , rad, dist)
  // 5: (5, phi1, phi2, dist)
  // 8: (...)
  //
  // ﾖｺ，ﾀﾃがno_orderのときブランク値として扱う
  // 基準値から±0.6Vは反応しないので，反応速度を高めるため，シリアル値で450飛ばす
  // スムージングする場合，ヨコは30ずつ，タテは20ずつ加減算
  // tmp_A, tmp_Bはグローバル変数で現在のシリアル値を保持（スムージング専用）
  // radは進行方向左の回転中心からの距離
  // phi2はphi1からの相対値（？）
  // 各distは+-32767[mm]まで対応しているが，30[m]を越えて入力しない
  */


  int velocity = order1;
  int radius = order2;
  int DA_vA = def_A;
  int DA_vB = def_B;

  switch (mode) {
    case 0:
      // 即時停止
      spi_transmit(0, tmp_A = def_A);
      spi_transmit(1, tmp_B = def_B);
      break;

    case 1:
      // スムージングなし
      if (order1 != no_order) spi_transmit(0, tmp_A = order1);
      if (order2 != no_order) spi_transmit(1, tmp_B = order2);

      //spi_transmit(order1, order2);
      //if (order1 == 0) tmp_A = order2;
      //if (order1 == 1) tmp_B = order2;
      break;

    case 2:
      // スムージングあり
      if ((order1 != no_order) && (order1 != tmp_A)) {
        if (tmp_A == def_A) tmp_A += (tmp_A < order1) ? 450 : -450;
        if (abs(tmp_A - order1) < smt_pit_crs * 2) tmp_A = order1;
        tmp_A += (tmp_A < order1) ? smt_pit_crs : -smt_pit_crs;
        spi_transmit(0, tmp_A);
      }
      if ((order2 != no_order) && (order2 != tmp_B)) {
        if (tmp_B == def_B) tmp_B += (tmp_B < order2) ? 450 : -450;
        if (abs(tmp_B - order2) < smt_pit_fwd * 2) tmp_B = order2;
        tmp_B += (tmp_B < order2) ? smt_pit_fwd : -smt_pit_fwd;
        spi_transmit(1, tmp_B);
      }

      /*
      if ((order1 == 0) && (tmp_A != order2)) {
        if (tmp_A == def_A) {
          if (tmp_A < order2) tmp_A += 450;
          else if (tmp_A > order2) tmp_A -= 450;
        }
        if (abs(tmp_A - order2) < 60) tmp_A = order2;
        else if (tmp_A < order2) tmp_A += 30;
        else if (tmp_A > order2) tmp_A -= 30;
      } else if ((order1 == 1) && (tmp_B != order2)) {
        if (tmp_B == def_B) {
          if (tmp_B < order2) tmp_B += 450;
          else if (tmp_B > order2) tmp_B -= 450;
        }
        if (abs(tmp_B - order2) < 40) tmp_B = order2;
        else if (tmp_B < order2) tmp_B += 20;
        else if (tmp_B > order2) tmp_B -= 20;
      }
      if (order1 == 0)  spi_transmit(0, tmp_A);
      else if (order1 == 1)  spi_transmit(1, tmp_B);
      */
      break;

    case 3:
      /*
      // [3][00000][00000][00000]
      // [3][速度][半径][0]
      // ルンバ的動作をエミュレートする関数
      // 速度値を(-500 - 500 mm/s)で指定
      // 半径値を(-2000 - 2000 mm)で指定
      // 半径値が 0 のとき，直進・後進
      // 半径値が -1 のとき，時計周りにその場で回転
      // 半径値が 1 のとき，反時計回りにその場で回転
      // 半径値が負のとき，右を回転中心として回転
      // 半径値が正のとき，左を回転中心として回転
      */

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

    case 4:
      /*
       * [4][00000][00000][00000]
       * [4][距離][半径][0]
       * ルンバ関数と同様であるが，速度ではなく距離を指定する
       * 到達予想時間をシリアル通信で返す
      */
      break;

    case 5:
      /*
       * [5][00000][00000][00000]
       * [5][方位][距離][方位]
      // 現在地と目的地でのその場回転，直進により移動を実現する
      //
      // 現在地から目的地までの相対値で指令する
      // 目的地の相対方位
      // 目的地までの距離
      // 目的地での自己の方位姿勢（移動方位からの相対値）
      // の3つの値を指示し，到達予定時間をシリアル通信で返す
      // 方位は0.1[deg]刻みで受け付ける（精度を保証するものではない）
      // 直進を0とし，左回りに3600までの値を受け付ける
      // 移動距離の制限はルンバ関数に準ずる
      // 割り込みでの急停止に対応する
      */

      /*
       * 目的地の方向を向く
       * 直進移動する
       * 目的地で姿勢を調整する
       *
       * ルンバ動作の再帰呼び出しによって実現
       * 即時停止以外のシリアル通信データは移動終了後に連続処理
       *
      */;
      break;

    case 6:
      /*
       * 直線移動，途中でのその場回転によって実現する
       * 自己方位姿勢と目的地での方位姿勢を元に
       * 接線の交点を計算し，交点で回転する
       * 90[deg]を越えた値はcase5を再帰呼び出しする
       * 2つの角度の正負が異なる場合，case5を再帰呼び出しする
      */;
      break;

    case 7:
      /*
       * 直線移動，円弧移動によって実現する
       * 自己方位姿勢と目的地での方位姿勢を元に
       * 半径2000[mm]以内の円弧と直線を組み合わせた経路を計算する
       * 停止しない滑らかな移動が可能
       * 2つの角度の正負が異なる場合，case5を再帰呼び出しする
      */;
      break;

    case 8:
      /*
       * 2つの円弧移動と直線移動によって実現する
       * 自己方位姿勢と目的地での方位姿勢を元に
       * 半径2000[mm]以内の円弧と直線を組み合わせた経路を計算する
       * 停止しない滑らかな移動が可能
       * 2つの角度の正負が異なる場合に対応する
      */;
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

