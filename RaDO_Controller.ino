#define MAX_LENGTH 10
#define MCP23S17_SSPIN 9

#define ERROR_LAMP 7
#define BELL 8
#define BZ21 13

#include <Metro.h>
#include <Spi.h>
#include <Mcp23s17.h>     // https://github.com/dreamcat4/Mcp23s17

MCP23S17 ioic_0 = MCP23S17( MCP23S17_SSPIN, 0x0 );
/**************************************************************************************
 *                    __________  ____________ 
 *   ATS-Sx Power ( 9)  1     B0     A7     28  ( 8) (C:10uF + R:4k)-> ding
 *  ATS-SX Active (10)  2     B1     A6     27  ( 7) 
 *    ATS-Sx Bell (11)  3     B2     A5     26  ( 6) ATS-P Cutoff
 * ATS-Sx BZ21Cut (12)  4  i  B3     A4  i  25  ( 5) ATS-P Active
 *                (13)  5  /  B4     A3  /  24  ( 4) ATS-P Purge
 *                (14)  6  o  B5     A2  o  23  ( 3) ATS-P Braking
 *                (15)  7     B6     A1     22  ( 2) ATS-P Pattern
 *                (16)  8 ___ B7     A0 ___ 21  ( 1) ATS-P Power
 *                      9     V+   intA     20  
 *                     10     GND  intB     19  
 *                     11     SS    RST     18  
 *                     12     CLK    A2     17  
 *                     13     SI     A1     16  
 *                     14     SO     A0     15  
 *                     ￣￣￣￣￣￣￣￣￣￣￣￣￣￣ 
 **************************************************************************************/

const int pinMatrix[] = 
  {
    ERROR_LAMP,    // arduinoPin 0
    ERROR_LAMP,    // arduinoPin 1
    1,             // arduinoPin 2
    2,             // arduinoPin 3
    3,             // arduinoPin 4
    4,             // arduinoPin 5
    5,             // arduinoPin 6
    6,             // arduinoPin 7
    8,             // arduinoPin 8
    9,             // arduinoPin 9
    10,            // arduinoPin 10
    ERROR_LAMP,    // arduinoPin 11
    11,            // arduinoPin 12
    12             // arduinoPin 13
  };

Metro ding = Metro(25);
Metro bz21 = Metro(250);

void setup() {
  // MCP23S17 init
  ioic_0.pinMode(OUTPUT);

  // all port pulldown to LOW
  ioic_0.port(0x00);

  ioic_0.digitalWrite(11, HIGH);  // DC100V系統は負論理制御のため

  // シリアル通信準備
  Serial.begin(115200);

  // プラグインとのハンドシェイク
  while (1) {
    ioic_0.digitalWrite(ERROR_LAMP, HIGH);

    Serial.println("1");
    if (Serial.available())
    {
      char temp = char(Serial.read());
      if (temp == '0') { break; }
    }
    delay(500);
    ioic_0.digitalWrite(ERROR_LAMP, LOW);
    delay(500);
  }
  
  while (Serial.read() != -1) ;
  ioic_0.digitalWrite(ERROR_LAMP, LOW);
  
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

  const int index = atoi(strtok(str, "," ));
  const int value = atoi(strtok(NULL, "," ));

  // 機器操作
  put_signal(pinMatrix[index], value);

  // アンサーバック
  Serial.println(String(index)+","+String(value));
}

void put_signal(int pin, int sig)
{
  /********* ATS-Sx 警報ベル制御 *********/
  if (pin == 12)
  {
    if (sig == 0)
    {
      ioic_0.digitalWrite(pin, HIGH);       // ATS-Sx 警報ベル鳴動
      flash_down(BZ21);              // ATS-Sx BZ21警報器回路開通
    }
    else { ioic_0.digitalWrite(pin, LOW); } // ATS-Sx 警報ベル停止
  }

  /** ATC/S-P 単打ベル,表示器 BZ21警報器回路 制御 **/
  else if (pin>=2 && pin<=13)
  {
    // フラッシュ制御機器動作(単打ベル,BZ21)
    if (pin == BELL | pin == BZ21) { flash_up(pin); }

    // その他機器制御
    else
    {
      if (sig == 0) { ioic_0.digitalWrite(pin, LOW); }
      else { ioic_0.digitalWrite(pin, HIGH); }
    }
  }

  // 解析失敗
  else
  {
    Serial.println("Opps!");
    ioic_0.digitalWrite(ERROR_LAMP, HIGH);
  }
  
}

void flash_up(int pin)
{
  ioic_0.digitalWrite(pin, LOW);             // 継続動作防止
  ioic_0.digitalWrite(pin, HIGH);
  if (pin == BELL) { ding.reset(); }  // 電源カットタイマーリセット
  else { bz21.reset(); }              // 回路復帰タイマーリセット
}

void flash_down(int pin)
{
  ioic_0.digitalWrite(pin, LOW);
}