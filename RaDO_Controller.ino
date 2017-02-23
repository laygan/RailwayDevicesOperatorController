#define MAX_LENGTH 10
#define BELL 8
#define BZ21 13

#include <Metro.h>

Metro ding = Metro(25);
Metro bz21 = Metro(250);

void setup() {
  // ピン出力設定
  for (int i=2; i<=13; i++)
  {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);  // DC100V系統は負論理制御のため

  // シリアル通信準備
  Serial.begin(115200);

  // プラグインとのハンドシェイク
  while (1) {
    digitalWrite(7, HIGH);

    Serial.println("1");
    if (Serial.available())
    {
      char temp = char(Serial.read());
      if (temp == '0') { break; }
    }
    delay(500);
    digitalWrite(7, LOW);
    delay(500);
  }
  
  while (Serial.read() != -1) ;
  digitalWrite(7, LOW);
  
  Serial.println("1");
}

void loop()
{
  // 単打ベル電源カットタイマー
  if (ding.check() == 1) { flash_down(BELL); }

  // BZ21警報器回路復帰タイマー
  if (bz21.check() == 1) { flash_down(BZ21); }

  // 制御信号解析
  if (Serial.available()) { parse_signal(); }
}

void parse_signal()
{
  char str[MAX_LENGTH];
  int p=0;
  do
  {
    while (Serial.available() <= 0);
    
    str[p] = Serial.read();
    p++;
  } while (str[p-1] != '\n');
  str[p-1] = '\0';

  const int port = atoi(strtok(str, "," ));
  const int value = atoi(strtok(NULL, "," ));

  // 機器操作
  put_signal(port, value);

  // アンサーバック
  Serial.println(String(port)+","+String(value));
}

void put_signal(int pin, int sig)
{
  /********* ATS-Sx 警報ベル制御 *********/
  if (pin == 12)
  {
    if (sig == 0)
    {
      digitalWrite(pin, HIGH);       // ATS-Sx 警報ベル鳴動
      flash_down(BZ21);              // ATS-Sx BZ21警報器回路開通
    }
    else { digitalWrite(pin, LOW); } // ATS-Sx 警報ベル停止
  }

  /** ATC/S-P 単打ベル,表示器 BZ21警報器回路 制御 **/
  else if (pin>=2 && pin<=13)
  {
    // フラッシュ制御機器動作(単打ベル,BZ21)
    if (pin == BELL | pin == BZ21) { flash_up(pin); }

    // その他機器制御
    else
    {
      if (sig == 0) { digitalWrite(pin, LOW); }
      else { digitalWrite(pin, HIGH); }
    }
  }

  // 解析失敗
  else
  {
    Serial.println("Opps!");
    digitalWrite(7, HIGH);
  }
  
}

void flash_up(int pin)
{
  digitalWrite(pin, LOW);             // 継続動作防止
  digitalWrite(pin, HIGH);
  if (pin == BELL) { ding.reset(); }  // 電源カットタイマーリセット
  else { bz21.reset(); }              // 回路復帰タイマーリセット
}

void flash_down(int pin)
{
  digitalWrite(pin, LOW);
}
