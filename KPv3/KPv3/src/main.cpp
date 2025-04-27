#include <Arduino.h>
#include <MFRC522.h>
#include <esp_task_wdt.h>


#define RST_PIN         3
#define SS_PIN          0
#define LED_PIN 2

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key key;         // Объект ключа
MFRC522::StatusCode status;      // Объект статуса
const int ledPin = 2;

uint8_t data[64][20] = {};
uint8_t readSize = 18;

bool read_sector(uint8_t numBlock);

void setup() 
{
  delay(500);
  Serial.begin(9600);  // Initialize serial communication with a baud rate of 9600
  SPI.begin();		    // Init SPI bus
  mfrc522.PCD_Init();	// Init MFRC522 card
  delay(5);
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_23dB_2);
  delay(5);
  mfrc522.PCD_AntennaOff(); 
  delay(5);
  mfrc522.PCD_AntennaOn();
  delay(5);
 
  delay(500);				    // Optional delay. Some board do need more time after init to be ready, see Readme
  // Serial.println("\nScript for CONTROL STATION ONLY!");
  pinMode(ledPin, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  Serial.println("Start programm");
  digitalWrite(LED_PIN, LOW);
  digitalWrite(ledPin, LOW);
  for (byte i = 0; i < 6; i++) 
  {   // Наполняем ключ
    key.keyByte[i] = 0xFF;         // Ключ по умолчанию 0xFFFFFFFFFFFF
  }
}

uint32_t rebootTimer = millis();

uint32_t  tryTime = millis();
uint32_t  stopTime = 0;
uint32_t  blockTime = 0;


uint64_t sectorTime_start = 0;
uint64_t sectorTime_stop = 0;

uint64_t blockTime_start = 0;


uint8_t res[16] = {};

void loop() 
{
  // Занимаемся чем угодно
  //rebootTimer = millis(); // Важный костыль против зависания модуля!
  if (millis() - rebootTimer >= 1000) 
  {   // Таймер с периодом 1000 мс
    Serial.println("reboot");
    rebootTimer = millis();               // Обновляем таймер
    digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
    delayMicroseconds(2);                 // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
    mfrc522.PCD_Init();                      // Инициализируем заного
  }
  
  if (!mfrc522.PICC_IsNewCardPresent()) return;  // Если новая метка не поднесена - вернуться в начало loop
  if (!mfrc522.PICC_ReadCardSerial()) return;    // Если метка не читается - вернуться в начало loop
  digitalWrite(LED_PIN, HIGH);


  sectorTime_start = millis();
  while( (millis() < sectorTime_start  + 2000)) 
  {
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (7), &key, &(mfrc522.uid)) == MFRC522::STATUS_OK)
    {
      for(uint8_t i = 4; i < 7; i++)
      {
        data[i][0] = 1;

        blockTime_start = millis();
        while( (millis() < blockTime_start  + 1000))
        {
          memset(&data[i][2], 0, 18);
          if(mfrc522.MIFARE_Read(i, &(data[i][2]), &readSize) == MFRC522::STATUS_OK)  
          {
            data[i][1] = 1;
            break;
          }
        }
      }
      sectorTime_stop = millis();
      break; 
    }
  }

  //res[1] = read_sector(1);

/*
  for(uint8_t i = 1; i < 16; i++) 
  {
   // while(!read_sector(i))

    res[i] = read_sector(i);
  }*/
  
/*
  status = MFRC522::STATUS_ERROR;
  tryTime = millis();
  while( (status != MFRC522::STATUS_OK) && (millis() < tryTime  + 200)) status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (7), &key, &(mfrc522.uid));
  stopTime = millis();

  if (status != MFRC522::STATUS_OK) data[4][0] = 0; 
  else
  {     // Если не окэй
       
    for(uint8_t j = 0; j < 3; j++)
    {
      data[4 + j][0] = 1;
      blockTime = millis();
      readSize = 18;
      status = MFRC522::STATUS_ERROR;
      while( (status != MFRC522::STATUS_OK) && (millis() < blockTime  + 200)) status = mfrc522.MIFARE_Read((4 + j), &(data[4 + j][2]), &readSize); // Читаем 6 блок в буфер
      
      if (status != MFRC522::STATUS_OK) 
      {             // Если не окэй
        data[4 + j][1] = 0;  
      }
      else data[4 + j][1] = 1;
    }
      
  }
*/

/*
  for(uint8_t i = 1; i < 16; i++)
  {
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (i*4 + 3), &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) data[i][0] = 0; 
    else
    {     // Если не окэй
       
      for(uint8_t j = 0; j < 3; j++)
      {
        data[i*4 + j][0] = 1;
        status = mfrc522.MIFARE_Read((i*4 + j), &(data[i*4 + j][2]), &readSize); // Читаем 6 блок в буфер
        if (status != MFRC522::STATUS_OK) 
        {             // Если не окэй
          data[i*4 + j][1] = 0;  
        }
        else data[i*4 + j][1] = 1;
      }
      
    }
  }
*/
/*

    uint8_t secBlockDump[16] = 
    {                   
      0xA1, 0xA2, 0xA3, 0xA4,
      0xAA, 0xBB, 0xCC, 0xDD,
      0xAA, 0xBB, 0xCC, 0xDD,
      0xD1, 0xD2, 0xD3, 0xD4
    };

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 7, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) Serial.println("Acess error");  
    else
    {     // Если не окэй
    
      status = mfrc522.MIFARE_Write(6, secBlockDump, 16); // Пишем массив в блок 7
      if (status != MFRC522::STATUS_OK) 
      {              // Если не окэй
        Serial.println("Write error");                 // Выводим ошибку
        return;
      }
    }
*/

  mfrc522.PICC_HaltA();                              // Завершаем работу с меткой
  mfrc522.PCD_StopCrypto1();
  digitalWrite(LED_PIN, LOW);
 
  /*
  for(uint8_t i = 1; i < 16; i++) 
  {
    Serial.print(i);
    Serial.print(" - ");
    Serial.println(res[i]);
  }*/
  

  /* Выводим содержимое блока в Serial */
  Serial.println("\nTime:");                          // Выводим 16 байт в формате HEX
  Serial.println(sectorTime_stop); 
  Serial.println(sectorTime_start);
  
  Serial.println("\nData:");                          // Выводим 16 байт в формате HEX
  
  for(uint8_t i = 4; i < 7; i++)
  {
    Serial.print(i);
    Serial.print(":\t");

    if(data[i][0] == 1)Serial.print("S ");
    else Serial.print("F ");

    if(data[i][1] == 1)
    {
      Serial.print("S ");
      for (uint8_t j = 0; j < 16; j++) 
      {
        Serial.print("0x");
        Serial.print(data[i][2+j], HEX);
  
        Serial.print(", ");
      }
    }
    else Serial.print("F ");

    memset(&data[i][0], 0, 20);
  Serial.println("");
  }
    delay(2000);
}



bool ReadSector(uint8_t numBlock)
{
  bool successFlag = false;

  sectorTime_start = millis();
  while( (millis() < sectorTime_start  + 5000)) 
  {
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (numBlock*4 + 3), &key, &(mfrc522.uid)) == MFRC522::STATUS_OK)
    {
      successFlag = true;
      for(uint8_t i = numBlock*4; i < numBlock*4 + 3; i++)
      {
        data[i][0] = 1;

        blockTime_start = millis();
        while( (millis() < blockTime_start  + 500))
        {
          if(mfrc522.MIFARE_Read(i, &(data[i][2]), &readSize) == MFRC522::STATUS_OK)  
          {
            data[i][1] = 1;
            break;
          }
        }
      }
      
      break; 
    }
    else  successFlag = false;
  }

  return  successFlag;
}

bool read_sector(uint8_t numBlock)
{
  bool flag = false;

  sectorTime_start = millis();
  while( (millis() < sectorTime_start  + 500)) 
  {
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, (numBlock*4 + 3), &key, &(mfrc522.uid)) == MFRC522::STATUS_OK)
    {
      for(uint8_t i = numBlock*4; i < numBlock*4 + 4; i++)
      {
        data[i][0] = 1;

        blockTime_start = millis();
        while( (millis() < blockTime_start  + 500))
        {
          if(mfrc522.MIFARE_Read(i, &(data[i][2]), &readSize) == MFRC522::STATUS_OK)  // Читаем 6 блок в буфер
          {
            data[i][1] = 1;
            break;
          }
        }
      } 
      break;
    }
    else data[numBlock][0] = 0;
  } 
  
  if(data[numBlock*4][1] + data[numBlock*4 + 1][1] + data[numBlock*4 + 2][1]  == 3)flag = true;

  return flag;
}