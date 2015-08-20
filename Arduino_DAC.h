
#ifndef spi
#define spi
#include <SPI.h>
#endif

void transmit(int mode, int ch, int DA_value);
void spi_transmit(int ch, int tmp);

extern int def_A;
extern int def_B;
extern int tmp_A;
extern int tmp_B;


/*=== class Jcode begin ===*/
class Jcode {
    char receive_raw[40];

    long escape_time;

    char mode[2];
    char value1[5];
    char value2[5];
    char value3[6];

    int mode_num;
    int value1_num;
    int value2_num;
    long value3_num;

  public:
    int receive_mode;           // 待機のモードは外部参照可能
    bool receive_valid = false; // 待機が有効な場合true

    Jcode() ;

    bool check();
    void convert();
    void show_raw();
    void show_div();
    void show_num();
    void echo();
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
          transmit(0, 0, 0);
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
  // {A ? B : C} => Aが trueならB, falseならC （三項演算子）
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
  transmit(mode_num, 0, def_A + value2_num);
  transmit(mode_num, 1, def_B + value1_num);
  while (1) {
    if (escape_time <= millis()) { // 維持時間を超過した
      if (receive_valid == true) { // 待機データが有効である
        show_raw();
        convert();   // 待機データを数値に変換，データの無効化
        show_num();
        transmit(mode_num, 0, def_A + value2_num);
        transmit(mode_num, 1, def_B + value1_num);
        Serial.println("convert");
      } else {       // データが無効なら関数離脱
        Serial.println("exit");
        return;
      }
      //transmit(mode_num, 0, def_A + value2_num);
      //transmit(mode_num, 1, def_B + value1_num);
    }
    check();
  }
}
/*=== class Jcode end ===*/



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

