# Arduino_DAC


void setup() {
  Serial.begin(9600); // 9600bpsでシリアルポートを開く
  Serial.setTimeout(5);
}

void loop() {
  char str[10] = {0} ;
  if (Serial.available() > 0) { // 受信したデータが存在する
    if (Serial.read() == 'a') {
      if (Serial.readBytesUntil('x', str, 10) == 8) {
        Serial.println(str);
        hoge(str);
      } else Serial.println("err_length");
    } else Serial.println("err_header");
  Serial.println(-atoi(str));
  }
}

void hoge(char *str){
    char str2[4];
    char str3[4];
    str2[0] = str[0];
    str2[1] = str[1];
    str2[2] = str[2];
    str2[3] = '\0';
    
    str3[0] = str[4];
    str3[1] = str[5];
    str3[2] = str[6];
    str3[3] = '\0';
    
        Serial.println(str2);
        Serial.println(str3);
}
