#include <SPI.h>
#include <MFRC522.h>
#include <TimeLib.h>

#define RST_PIN         3
#define SS_PIN          0         // Configurable, see typical pin layout above

#define REBOOT_TIME 10000

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key KeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key KeyB = {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC};
MFRC522::Uid prev_uid;

uint32_t timer_start, timer_stop;

void setup() {
	Serial.begin(115200);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
  delay(5);
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_23dB_2);
  mfrc522.PCD_AntennaOff(); 
  delay(5);
  mfrc522.PCD_AntennaOn();
  delay(5);
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  prev_uid.size = 0x00;
  memset(prev_uid.uidByte, 0x00, sizeof(prev_uid.uidByte));
  prev_uid.sak = 0x00;
  
}

uint32_t  rebootTimer = millis();
uint32_t  tryTime = millis();
byte chip[64][16];


void loop(){
  // if (millis() - rebootTimer >= REBOOT_TIME)
    // rebootRFID();

  int prev_sector = 15;
  for (int i=0; i<64; i++){
    for (int j=0; j<16;j++){
      chip[i][j] = 0x00;
    }
  }



  timer_start = 0;
  while (prev_sector != -1 && memcmp(&prev_uid, &mfrc522.uid, sizeof(prev_uid)) == 0){
    if ( ! mfrc522.PICC_IsNewCardPresent()) 
      continue;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
      continue;

    // if (memcmp(&prev_uid, &mfrc522.uid, sizeof(prev_uid)) != 0){
    //   Serial.println("Change UID");
    //   prev_uid = mfrc522.uid;
    //   Serial.print("UID: ");
    //   dump_byte_array(&(mfrc522.uid.uidByte[0]), mfrc522.uid.size);
    //   break;
    // }

    if (timer_start == 0){
      timer_start = millis();
      Serial.println("reset timer");
    }
    prev_sector = read_chip(prev_sector);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();


  }
  timer_stop = millis();
  Serial.print("Time: ");
  Serial.println(timer_stop - timer_start);
  if (prev_sector == -1){
    dump_chip();
    represent_dump();
    delay(500);
  }

  if (memcmp(&prev_uid, &mfrc522.uid, sizeof(prev_uid)) != 0){
    Serial.println("Change UID");
    prev_uid = mfrc522.uid;
    Serial.print("UID: ");
    dump_byte_array(&(mfrc522.uid.uidByte[0]), mfrc522.uid.size);

  }

}



int read_chip(int prev_sector){
  Serial.println("Read Sector");
  byte sector         = 1;
  byte valueBlockA    = 5;
  byte valueBlockB    = 6;
  byte trailerBlock   = 3;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  int32_t value;
  
  for (int sector=prev_sector; sector>=0; sector--){
    trailerBlock = sector * 4 + 3;
    // Serial.println("");
    // Serial.println(sector);
    //auth read
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &KeyA, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return sector;
    }
    // Serial.println(sector);
    for (int block=3; block>=0; block--){
      trailerBlock = sector * 4 + 3;
      status = mfrc522.MIFARE_Read(sector * 4 + block, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));

        return sector;
      }
      // Serial.print("\t");
      // Serial.print(sector * 4 + block);
      // Serial.print("\t");
      memcpy(&(chip[sector * 4 + block]), &buffer, 16);
      // dump_byte_array(buffer, 16);
      // Serial.println();
    }
  }
  return -1;
}

void rebootRFID(){
  Serial.println("reboot");
  rebootTimer = millis();               // Обновляем таймер
  digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
  mfrc522.PCD_Init();                      // Инициализируем заного
  delay(5);
  mfrc522.PCD_AntennaOff(); 
  delay(5);
  mfrc522.PCD_AntennaOn();
  delay(5);
  return;
}

void dump_chip(){
  for (int sector=16; sector>=0; sector--){
    Serial.println(sector);
    for (int block=3; block>=0; block--){
      Serial.print("\t");
      Serial.print(sector * 4 + block);
      Serial.print("\t");
      dump_byte_array(&(chip[sector * 4 + block][0]), 16);
      Serial.println();
    }
    // delay(10);
  }
}

void represent_dump(){
  uint32_t startTime;
  uint32_t prevTime;
  uint8_t kpNo;
  uint32_t time;
  uint16_t ms;
  uint8_t  trailerBlock;

  uint32_t chipNo = littleEndianToInt(&chip[1][0], 4);
  Serial.printf("Номер чипа: %d\r\n", chipNo);
  for (int block=4; block<64; block++){
    trailerBlock = getTrailerSectorAddr(block);
    if (block != trailerBlock){
      kpNo = littleEndianToInt(&chip[block][0], 1);
      if (kpNo == 0){
        return;
      }
      time = littleEndianToInt(&chip[block][1], 4);
      if (block == 4){
        startTime = time;
        prevTime = time;
      } 

      ms = littleEndianToInt(&chip[block][5], 2);
      char buf[22];
      sprintf(buf, "%3d %2d:%2d:%2d.%3d %2d:%2d", kpNo, hour(time-startTime), minute(time-startTime), second(time-startTime), ms, minute(time-prevTime), second(time-prevTime));
      prevTime = time;
      Serial.println(buf);
    }
  }
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void intToLittleEndian(uint32_t num, byte *array, int length){ //max 4 bytes
  for (int i=0; i<length; i++){
    array[i] = (num >> (8*i)) & 0xFF;
  }
}

uint32_t littleEndianToInt(byte* byteArray, int size) {
    uint32_t number = 0;
    for (int i = 0; i < size; i++) {
        number = (uint32_t)byteArray[i] << (i * 8); // Собираем число из байтов
    }
    return number;
}

byte getTrailerSectorAddr(byte addr){
  return addr/4*4+3;
}