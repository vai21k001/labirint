#include <MFRC522.h>
#include <SPI.h>
#include <TimeLib.h>

#define RST_PIN         3
#define SS_PIN          0
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
const int ledPin = 2;

byte action = 1;
int chipNo = 0;

int STATE = 1;

int approvedKeyIndex = 0;
#define amountOfKeys 8
byte keysArr[amountOfKeys][MFRC522::MF_KEY_SIZE] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC},
  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
  {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
  {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
  {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
  {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
  {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff} // AA BB CC DD EE FF
};

void intToLittleEndian(uint32_t num, byte *array, int length){ //max 4 bytes
  for (int i=0; i<length; i++){
    array[i] = (num >> (8*i)) & 0xFF;
  }
}

uint32_t littleEndianToInt(byte* byteArray, int size) {
    uint32_t number = 0;
    for (int i = 0; i < size; i++) {
        number |= (uint32_t)byteArray[i] << (i * 8); // Собираем число из байтов
    }
    return number;
}

void setup() {
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
  digitalWrite(ledPin, LOW);
}
uint8_t code = 0;
bool NewRead = false;

void loop() {
  NewRead = false;
  if (Serial.available() > 0){
    action = Serial.read();
    NewRead = true;
  }
  switch (action) {

    case '1':
      digitalWrite(ledPin, HIGH);
      delay(5);
      digitalWrite(ledPin, LOW);
      Serial.println(2);      
      action = 0;         
      break;

    case '2':
      digitalWrite(ledPin, HIGH);
      delay(5);
      digitalWrite(ledPin, LOW);
      readCard();      
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1(); 
           
      break;

    case '3':
      if (NewRead) {
        String a = Serial.readString();
        chipNo = a.toInt();
        Serial.print("Recived ");
        Serial.println(chipNo);
        
        }
      
      code = chipRegister(chipNo);
      if (code == 1){
        action = 0;
        Serial.print("Chip successefully registered No: ");
        Serial.println(chipNo);
        char buf[22];
        sprintf(buf, "d%d", chipNo);
        Serial.println(buf);
        Serial.println("\ne");
      }else if (code != 0) {
        Serial.print("Error in registering chip ");
        Serial.print(code);
        Serial.println("\ne");
      }
      
      break;

    case '4':
    // функция записи номера (переменная number)
      break;  

  }
  
  delay(10);

}

uint8_t safe_write(byte block_addr, byte buffer[], byte bufferSize){
  MFRC522::StatusCode status;
  byte read_buffer[18];
  byte size = sizeof(read_buffer);
  bool FLAG = true;
  uint8_t iter = 0;
  dump_byte_array(buffer, 16);
  do {
    FLAG = false;
    status = mfrc522.MIFARE_Write(block_addr, buffer, bufferSize);
    if (status != MFRC522::STATUS_OK ) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      continue;
    }

    status = mfrc522.MIFARE_Read(block_addr, read_buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print("MIFARE_Read() failed: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      continue;
    }

    for (uint8_t i = 0; i < 16; i++){
      if (read_buffer[i] != buffer[i]){
        FLAG = true;
        continue;
      }
    }

    iter += 1;
  }while(FLAG && iter < 10);
  if (iter == 10){return false;}
  return true;
}

uint8_t chipRegister(uint32_t chipNo){ 
  if ( ! mfrc522.PICC_IsNewCardPresent()){
    //Сигнализировать о том, что карта уже была считана
    return 0;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return 0;
  
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    // Serial.println(F("This code only works with MIFARE 1K cards."));
    return 4;
  }



  int keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 1, approvedKeyIndex);
  if (keyNum == -1){
      Serial.println("Ошибка ключа при авторизации чтении номера чипа");
      // sndtoPC(buf, sizeof(buf));
      return 5;
  }

  MFRC522::StatusCode status;
  byte buffer[] = {
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0 };
  // byte size = sizeof(buffer);

  intToLittleEndian(chipNo, &buffer[0], 4);
  if(safe_write(1, buffer, 16)){
    return 1;
  }
  return 0;
}

byte getTrailerSectorAddr(byte addr){
  return addr/4*4+3;
}

void readCard(){
  if ( ! mfrc522.PICC_IsNewCardPresent()){
    //Сигнализировать о том, что карта уже была считана
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;
  
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    // Serial.println(F("This code only works with MIFARE 1K cards."));
    return;
  }

  byte trailerBlock = 0;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  uint32_t startTime;
  uint32_t prevTime;
  uint8_t kpNo;
  uint32_t time;
  uint16_t ms;

  int keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 3, approvedKeyIndex);
  if (keyNum == -1){
      Serial.println("Ошибка ключа при авторизации чтении номера чипа");
      // sndtoPC(buf, sizeof(buf));
      return;
  }
  status = mfrc522.MIFARE_Read(1, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    // Serial.print(F("MIFARE_Read() failed: "));
    // String buf(mfrc522.GetStatusCodeName(status));
    Serial.println("Ошибка чтения");
    // sndtoPC(buf, sizeof(buf));
    return;
  }
  uint32_t chipNo = littleEndianToInt(buffer, 4);
  Serial.printf("nНомер чипа: %d\r\n", chipNo);

  status = mfrc522.MIFARE_Read(2, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    // Serial.print(F("MIFARE_Read() failed: "));
    // String buf(mfrc522.GetStatusCodeName(status));
    Serial.println("Ошибка чтения");
    // sndtoPC(buf, sizeof(buf));
    return;
  }
  uint8_t lastBlock = littleEndianToInt(&buffer[1], 1);
  if (lastBlock > 63){ lastBlock = 63; }


  for (uint8_t block = 4; block <= lastBlock; block++){
    if (trailerBlock != getTrailerSectorAddr(block)){
      trailerBlock = getTrailerSectorAddr(block);
      keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, trailerBlock, approvedKeyIndex);
      if (keyNum == -1){
        Serial.println(mfrc522.GetStatusCodeName(status));
        continue;
      }
      approvedKeyIndex = keyNum;
    }
    if (block != trailerBlock){
      status = mfrc522.MIFARE_Read(block, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        // Serial.print(F("MIFARE_Read() failed: "));
        // String buf(mfrc522.GetStatusCodeName(status));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      kpNo = littleEndianToInt(buffer, 1);
      time = littleEndianToInt(&buffer[1], 4);
      if (block == 4){
        startTime = time;
        prevTime = time;
      } 
      ms = littleEndianToInt(&buffer[5], 2);
      char buf[22];
      sprintf(buf, "d%3d %2d:%2d:%2d.%3d %2d:%2d", kpNo, hour(time-startTime), minute(time-startTime), second(time-startTime), ms, minute(time-prevTime), second(time-prevTime));
      prevTime = time;
      Serial.println(buf);
    }
  }

  Serial.println("\r\ne");   
 
}

// char* bytesToStringArr(byte *buffer, byte bufferSize) {
//   char str[32] = {};

//   for (byte i = 0; i < bufferSize; i+=2) {
//     str[i] = buffer[i] < 0x10 ? " 0" : " "
//     Serial.print(buffer[i] < 0x10 ? " 0" : " ");
//     Serial.print(buffer[i], HEX);
//   }

//   return str;
// }

void sndtoPC(char *msg, byte size){      //отправка одного пакета 

   Serial.print("d");           //начало пакета 

    for(int i = 0; i < size; i++){
      Serial.print(msg[i]);
    }
  Serial.print("\r\n");
    // Serial.println("e");           //конец пакета
}
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}



bool chipAuth(byte keyType, byte *key_, byte block){
  // int iters = 0;
  MFRC522::StatusCode status;
  MFRC522::MIFARE_Key key;
  for (int j=0; j<MFRC522::MF_KEY_SIZE; j++){
    key.keyByte[j] = key_[j];
  }
  // Serial.println("Попытка использовать ключ:");
  // byte buffer[] = {key[0], key[1], key[2], key[3], key[4], key[5]}
  // dump_byte_array(key.keyByte, 6);
  status = mfrc522.PCD_Authenticate(keyType, block, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Serial.print(F("Success with key"));
  // dump_byte_array((key).keyByte, MFRC522::MF_KEY_SIZE);
  return true;
}

int bruteAuth(byte keyType, byte keys[amountOfKeys][MFRC522::MF_KEY_SIZE], int keysAmount_, byte block){
  int iters;
  
  for (int i=0; i < keysAmount_; i++){
    
    if (chipAuth(keyType, keys[i], block)){
      return i;
    }
      // http://arduino.stackexchange.com/a/14316
    if ( ! mfrc522.PICC_IsNewCardPresent())
        break;
    if ( ! mfrc522.PICC_ReadCardSerial())
        break;
  }
  Serial.println("Ключ не найден");
  return -1;
}

int fastAuth(byte keyType, byte keys[amountOfKeys][MFRC522::MF_KEY_SIZE], int keysAmount_, byte block, int keyIndex){
  // Serial.println("Использование последнего ключа");
  if (chipAuth(keyType, keys[keyIndex], block)){
    return keyIndex;
  }else{
    Serial.println("Использование последнего ключа не удалось. Идёт подбор...");
    return bruteAuth(keyType, keys, keysAmount_, block);
  }
}
