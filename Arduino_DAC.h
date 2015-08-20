
#ifndef spi
#define spi
#include <SPI.h>
#endif

void transmit(int mode, int value1, int value2, int value3);
void spi_transmit(int ch, int tmp);

extern int def_A;
extern int def_B;
extern int tmp_A;
extern int tmp_B;


/*=== class Jcode begin ===*/
class Jcode {
    char receive_raw[40];

    long escape_time;           // millis()と比較するための変数

    char mode[2];
    char value1[5];
    char value2[5];
    char value3[6];             // 分割した文字列を格納

    int mode_num;
    int value1_num;
    int value2_num;
    long value3_num;            // 数値化して格納

  public:
    int receive_mode;           // 待機のモードは外部参照可能
    bool receive_valid = false; // 待機が有効な場合true

    Jcode() ;

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

bool Jcode::check() {
  /*
  // シリアルバッファのチェック，'j'の探索
  // 受信データをstrに格納，16バイトであればreceive_rawに複写
  // 受信データのmodeが'0'のとき即時停止
  // それ以外の時receive_valid = true
  // 受信データが有効な場合，関数としてtrueを返しSPIの送信処理に入る
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
          transmit(0, 0, 0, 0);
          Serial.println("stop");
          escape_time = 0;
          return false;
          // 待機データのmodeが0のとき即時停止して関数離脱
        }
        receive_valid = true;
        Serial.println("receive");
        show_raw();
        answer = true;
      }
    }
  }
  return answer;
}

void Jcode::convert() {
  mode[0] = receive_raw[0];
  mode[1] = '\0';

  value1[0] = receive_raw[2];
  value1[1] = receive_raw[3];
  value1[2] = receive_raw[4];
  value1[3] = receive_raw[5];
  value1[4] = '\0';

  value2[0] = receive_raw[7];
  value2[1] = receive_raw[8];
  value2[2] = receive_raw[9];
  value2[3] = receive_raw[10];
  value2[4] = '\0';

  value3[0] = receive_raw[11];
  value3[1] = receive_raw[12];
  value3[2] = receive_raw[13];
  value3[3] = receive_raw[14];
  value3[4] = receive_raw[15];
  value3[5] = '\0';

  mode_num = atoi(mode);
  // X = {A ? B : C} => Aが trueならB, falseならC （三項演算子）
  value1_num = (receive_raw[1] == '0') ? atoi(value1) : -atoi(value1);
  value2_num = (receive_raw[6] == '0') ? atoi(value2) : -atoi(value2);
  value3_num = atol(value3);

  receive_valid = false;
}

void Jcode::show_raw() {
  Serial.println(receive_raw);
}

void Jcode::show_div() {
  Serial.println(mode);
  Serial.println(value1);
  Serial.println(value2);
  Serial.println(value3);
}

void Jcode::show_num() {
  Serial.println(mode_num);
  Serial.println(value1_num);
  Serial.println(value2_num);
  Serial.println(value3_num);
}

void Jcode::echo() {
  escape_time = millis() + value3_num;
  transmit(mode_num, 0, def_A + value2_num, 0);
  transmit(mode_num, 1, def_B + value1_num, 0);
  while (1) {
    if (escape_time <= millis()) { // 維持時間を超過した
      if (receive_valid == true) { // 待機データが有効である
        convert();   // 待機データを数値に変換，データの無効化
        Serial.println("convert");
        show_num();
        escape_time = millis() + value3_num;
        transmit(mode_num, 0, def_A + value2_num, 0);
        transmit(mode_num, 1, def_B + value1_num, 0);
      } else {       // データが無効なら関数離脱
        Serial.println("exit");
        return;
      }
      //transmit(mode_num, 0, def_A + value2_num, 0);
      //transmit(mode_num, 1, def_B + value1_num, 0);
    }
    check();
  }
}
/*=== class Jcode end ===*/





void transmit(int mode, int value1, int value2, int value3) {
  /*
  // mode
  // 0: 即時停止
  // 1: スムージングなし
  // 2: スムージングあり
  // 3: ルンバ的動作（開発中）
  // 4: 相対移動
  //
  // ch(value1) 0:A(ヨコ) 1:B(タテ)
  // 基準値から±0.6Vは反応しないので，反応速度を高めるため
  // シリアル値で450だけ飛ばす
  // スムージングする場合，ヨコは30ずつ，タテは20ずつ加減算
  // tmp_A, tmp_Bはグローバル変数で現在のシリアル値を保持
  */

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
      spi_transmit(value1, value2);
      if (value1 == 0) tmp_A = value2;
      if (value1 == 1) tmp_B = value2;
      break;

    case 2:
      // スムージングあり
      if ((value1 == 0) && (tmp_A != value2)) {
        if (tmp_A == def_A) {
          if (tmp_A < value2) tmp_A += 450;
          else if (tmp_A > value2) tmp_A -= 450;
        }
        if (abs(tmp_A - value2) < 60) tmp_A = value2;
        else if (tmp_A < value2) tmp_A += 30;
        else if (tmp_A > value2) tmp_A -= 30;
      } else if ((value1 == 1) && (tmp_B != value2)) {
        if (tmp_B == def_B) {
          if (tmp_B < value2) tmp_B += 450;
          else if (tmp_B > value2) tmp_B -= 450;
        }
        if (abs(tmp_B - value2) < 40) tmp_B = value2;
        else if (tmp_B < value2) tmp_B += 20;
        else if (tmp_B > value2) tmp_B -= 20;
      }
      if (value1 == 0)  spi_transmit(0, tmp_A);
      else if (value1 == 1)  spi_transmit(1, tmp_B);
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

      int velocity = value1;
      int radius = value2;
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

    case 4:
      /*
       * ルンバ関数と同様であるが，速度ではなく距離を指定する
       * 到達予想時間をシリアル通信で返す
      */
      break;

    case 5:
      /*
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

