
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

extern Jcode Current;
extern Jcode Queue;

/*=== class Jcode begin ===*/
class Jcode {
    char raw_code[40];

    char mode[2];
    char velo[5];
    char cros[5];
    char dely[6];

    int velo_num;
    int cros_num;
    long dely_num;
    long escape_time;

    int velo_pm = 1;
    int cros_pm = 1;

  public:
    int mode_num;    // 0の参照のためpublic
    bool valid = false;    // queueが有効な場合true

    Jcode() ;

    void set(char raw[40]);

    void convert();

    void show_raw();
    void show_num();

    void echo();
};

Jcode::Jcode() {
  //set(raw);
}

void Jcode::set(char raw[40]) {
  for (int i = 0; i < 40; i++) {
    if (raw[i]) break;
    raw_code[i] = raw[i];
  }
}

void Jcode::convert() {
  mode[0] = raw_code[0];
  mode[1] = '\0';

  velo[0] = raw_code[2];
  velo[1] = raw_code[3];
  velo[2] = raw_code[4];
  velo[3] = raw_code[5];
  velo[4] = '\0';

  cros[0] = raw_code[7];
  cros[1] = raw_code[8];
  cros[2] = raw_code[9];
  cros[3] = raw_code[10];
  cros[4] = '\0';

  dely[0] = raw_code[11];
  dely[1] = raw_code[12];
  dely[2] = raw_code[13];
  dely[3] = raw_code[14];
  dely[4] = raw_code[15];
  dely[5] = '\0';

  if (raw_code[1] != '0') velo_pm = -1;
  if (raw_code[6] != '0') cros_pm = -1;

  mode_num = atoi(mode);
  velo_num = velo_pm * atoi(velo);
  cros_num = cros_pm * atoi(cros);
  dely_num = atol(dely);
}

void Jcode::show_raw() {
  Serial.println(raw_code);
  Serial.println(mode);
  Serial.println(velo);
  Serial.println(cros);
  Serial.println(dely);
}

void Jcode::show_num() {
  Serial.println(mode_num);
  Serial.println(velo_num);
  Serial.println(cros_num);
  Serial.println(dely_num);
}

extern char str;

void Jcode::echo() {
  escape_time = millis() + dely_num;
  transmit(mode_num, 0, def_A + cros_num);
  transmit(mode_num, 1, def_B + velo_num);
  while (1) {
    if (escape_time <= millis()) { // 維持時間を超過した
      if (Queue.valid == true) { // 待機データが有効である
        // Queue を Current に代入
        // Queue をクリア
      } else {
        return 0;
      }
    }
    while (Serial.available() > 0) {
      if (Serial.read() == 'j') {
        if (Serial.readBytesUntil('x', str, 40) == 16) {
          Queue.set(str);
          Queue.convert();
          if (Queue.mode_num == 0) {
            transmit(0, 0, 0);
            // Current,Queueクリア
            return 0;
          }
          Queue.valid = true;
        }
      }
    }
    transmit(mode_num, 0, def_A + cros_num);
    transmit(mode_num, 1, def_B + velo_num);
  }
  Serial.println("exit");
}
/*=== class Jcode end ===*/



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
