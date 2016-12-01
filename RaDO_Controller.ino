void setup() {
  int i;
  for (i=2; i<7; i++)
  {
    pinMode(i, OUTPUT);
    
  }
  for (i=8; i<13; i++)
  {
    pinMode(i, OUTPUT);
  }
  Serial.begin(115200);
 
  
  while (1) {
    digitalWrite(8, HIGH);
    Serial.println("1");
    if (Serial.available())
    {
      char temp = char(Serial.read());
      if (temp == '0')
      {
        break;
      }
    }
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
  }
  
  while (Serial.read() != -1) ;
  digitalWrite(8, LOW);
  
  Serial.println("1");
}

void loop()
{
  if (Serial.available())
  {
    String buf;
    int pin_num, value;
    
    while (1)
    {
      if (Serial.available())
      {
        char temp = char(Serial.read());
        if(temp != ',')
        {
          buf += temp;
        }
        else
        {
          pin_num = buf.toInt();
          while (1)
          {
            if (Serial.available())
            {
              buf = char(Serial.read());
              value = buf.toInt();
              break;
            }
          }
        }
        
        if(temp == '\n')
        {
          break;
        }
      }
    }
    put_signal(pin_num, value);
  }
}

void put_signal(int pin, int sig)
{
  if (pin == 12)
  {
    if (sig == 1)
    {
      digitalWrite(12, HIGH);
    }
    else
    {
      digitalWrite(12, LOW);
    }
    
  }
  
  else if (pin>=2 && pin<=9)
  {
    if (sig == 0)
    {
      digitalWrite(pin, LOW);
    }
    else
    {
      digitalWrite(pin, HIGH);
    }
    bell();
  }
  else if (pin == 10)
  {
    bell();
  }
  else
  {
    Serial.println("Opps!");
    digitalWrite(8, HIGH);
  }
}

void bell(void)
{
  digitalWrite(10, HIGH);
  delay(40);
  digitalWrite(10, LOW);
}
