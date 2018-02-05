/* MCP23S17 Pin Assign
 * 
 * B0 (0x80) A7 <-> Ding
 * B1 (0x40) A6
 * B2 (0x20) A5
 * B3 (0x10) A4 <-> ATS-P Fail
 * B4 (0x08) A3 <-> ATS-P Active
 * B5 (0x04) A2 <-> ATS-P Break
 * B6 (0x02) A1 <-> ATS-P Pattern
 * B7 (0x01) A0 <-> ATS-P Power
 * 
 */

#define MAX_LENGTH 10

#define BELL 7
#define BZ21 13

// SPI IF
#define DATAOUT 11//MOSI
#define DATAIN  12//MISO 
#define SPICLOCK  13//sck
#define SLAVESELECT 10//ss

// Interrupt
#define INTPIN 2

// MCP23S17 Device OPCODE
#define HWADDW 0x40
#define HWADDR 0x41

#include <Metro.h>
Metro ding = Metro(40);
Metro bz21 = Metro(250);

volatile byte address = 0;
volatile byte data = 0;
volatile byte deviceState = 0x00;

//SPI 関数
byte spi_transfer(volatile byte data)
{
  SPDR = data;                    // Start the transmission

  while (!(SPSR & (1 << SPIF)))   // Wait the end of the transmission
  {
  };

  return SPDR;                    // return the received byte
}

void initMCP23S17()
{
  //--------------------------------------------------------------
  //pinmode 設定
  
  pinMode(DATAOUT, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(SPICLOCK, OUTPUT);
  pinMode(SLAVESELECT, OUTPUT);
  digitalWrite(SLAVESELECT, HIGH); //disable device

  //--------------------------------------------------------------
  //SPI設定
  
  // SPCR = 01010000
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on rising edge of clk,system clock/2 rate (fastest)
  SPCR = 0x50;
  SPSR = 0x01; // SPI2X:1

  //--------------------------------------------------------------
  //MCP23S17 初期化
  
  // IOCON(0x0A):0x20
  //  BANK  :0
  //  MIRROR:0
  //  SEQOP :1
  //  DISSLW:0
  //  HAEN  :0
  //  ODR   :0
  //  INTPOL:0
  //  -     :0
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x0A;
  spi_transfer(address);   //send MSByte address first
  data = 0x20;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip

  // GPA -----------------------------------------------
  // Set as outputs
  // IODIRA(0x00): 1:input 0:output
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x00;
  spi_transfer(address);   //send MSByte address first
  data = 0x00;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip

  // GPB -----------------------------------------------
  // Set as outputs
  // IODIRB(0x01): 1:input 0:output
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x01;
  spi_transfer(address);   //send MSByte address first
  data = 0x00;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip
/*
  // Pull-up
  // GPPUB(0x0D): 1:pull-up enable 0:pull-up disable
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x0D;
  spi_transfer(address);   //send MSByte address first
  data = 0xFF;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip
  
  // 入力ピンの論理値を反転
  // IPOLB(0x03): 1:opposite 0:same 
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x03;
  spi_transfer(address);   //send MSByte address first
  data = 0xFF;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip
  
  // Interrupt-on-change enable
  // GPINTENB(0x05): 1:enable 0:disable
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);   //send MSByte address first
  address = 0x05;
  spi_transfer(address);   //send MSByte address first
  data = 0xFF;
  spi_transfer(data);      //send MSByte DATA
  digitalWrite(SLAVESELECT, HIGH); //release chip
  
  //--------------------------------------------------------------
  //外部割込み
  pinMode(INTPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTPIN), dataChanged, FALLING);
*/
}


int deviceMap[9] = {0, 0, 1, 2, 3, 4, 5, 6, 7};

void setup()
{
  initMCP23S17();
  
  // シリアル通信準備
  Serial.begin(115200);

  // プラグインとのハンドシェイク
  while (1) {
    driveDevice(6, true);

    Serial.println("1");
    if (Serial.available())
    {
      char temp = char(Serial.read());
      if (temp == '0') { break; }
    }
    delay(500);
    driveDevice(6, false);
    delay(500);
  }
  
  while (Serial.read() != -1) ;
  driveDevice(6, false);
  
  Serial.println("1");
}

void driveDevice(int dev, bool value)
{
  digitalWrite(SLAVESELECT, LOW);
  spi_transfer(HWADDW);
  spi_transfer(0x12);
  if(value) {
    deviceState = deviceState | 1<<dev;
  } else {
    deviceState = deviceState & ~(1<<dev);
  }
  spi_transfer(deviceState);
  digitalWrite(SLAVESELECT, HIGH);
}

bool ding_state = false;
bool bz21_state = false;

void loop()
{
  // 単打ベル電源カットタイマー
  if (ding_state){
    if (ding.check() == 1) put_signal(BELL, false);
  }

  // BZ21警報器回路復帰タイマー
//  if (bz21.check() == 1) { flash_down(BZ21); }

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

  const int device = atoi(strtok(str, "," ));
  const int value = atoi(strtok(NULL, "," ));

  // 機器操作
  put_signal(deviceMap[device], value);
}

void put_signal(int device, int value)
{
  switch(device) {
    case BELL:
      if(!value) {
        ding_state = false;
      }
      else {
        ding_state = true;
        ding.reset();
      }
      driveDevice(device, ding_state);
      break;

    default:
      if (value == 0) driveDevice(device, false);
      else driveDevice(device, true);
  }
}