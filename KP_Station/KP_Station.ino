//RC522
#include <SPI.h>
#include <MFRC522.h>
//NTP & RTC
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

RTC_DS3231 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ru.pool.ntp.org", 10800, 60000);

#define RST_PIN         D4
#define SS_PIN          D8
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
const int ledPin = D0;

uint8_t StationNo = 245; //240 - start, 245 - finish, 248 - check, 249 - clear
// MFRC522::MIFARE_Key KeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Лишнее
// MFRC522::MIFARE_Key KeyB = {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC};

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

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void updateRTC(NTPClient *_timeClient, RTC_DS3231 *_rtc){
  Serial.println("Time syncromizing...");
  (*_timeClient).update();  // Update time from NTP server
  unsigned long unix_epoch = (*_timeClient).getEpochTime();  // Get current epoch time
  (*_rtc).adjust(DateTime(unix_epoch));

  DateTime rtcTime = (*_rtc).now();
  Serial.println("Time is successfully synchronized!");
}

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
  Serial.begin(9600);  // Initialize serial communication with a baud rate of 9600
  Wire.begin();  // Begin I2C communication
  rtc.begin();  // Initialize DS3231 RTC module
  SPI.begin();		    // Init SPI bus
  mfrc522.PCD_Init();	// Init MFRC522 card
  delay(4);				    // Optional delay. Some board do need more time after init to be ready, see Readme
  Serial.println("\nScript for KP STATION ONLY!");
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);

  // Connect to WiFi
  char ssid[] = "bas"; // Replace with your WiFi SSID
  char password[] = "00009999"; // Replace with your WiFi password
  WiFi.begin(ssid, password);
  int tryes = 0;
  while (WiFi.status() != WL_CONNECTED && tryes < 30)
  {
    delay(500);
    Serial.print(".");
    tryes += 1;
  }
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("Connected to WiFi");
    timeClient.begin();  // Start NTP client
    updateRTC(&timeClient, &rtc);
    Serial.println("RTC updated");
    //Stop All Connections
    timeClient.end(); 
  }
  else{
    Serial.println("No WIFI connection");
  }
  WiFi.disconnect();
  Serial.println("Stop connections");
}

void loop() {
  delay(50); // Упрощённо энергосбережение
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()){
    //Сигнализировать о том, что карта уже была считана
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    Serial.println(F("This code only works with MIFARE 1K cards."));
    return;
  }

  checkIn();
  // bruteAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 2);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
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
    Serial.print(block);
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Serial.print(F("Success with key:"));
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
    // Serial.println("Использование последнего ключа не удалось. Идёт подбор...");
    return bruteAuth(keyType, keys, keysAmount_, block);
  }
}

byte getNextAddr(uint8_t prevAddr){
  if ((prevAddr + 2) % 4 == 0){
    return prevAddr + 2;
  }
  return prevAddr + 1;
}

byte getTrailerSectorAddr(byte addr){
  return addr/4*4+3;
}

uint8_t safe_write(byte block_addr, byte buffer[], byte bufferSize){
  MFRC522::StatusCode status;
  byte read_buffer[18];
  byte size = sizeof(read_buffer);
  bool FLAG = true;
  uint8_t iter = 0;
  // dump_byte_array(buffer, 16);
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

uint8_t checkIn(){
  Serial.println("Начало отметки на станции");
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  int keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 2, approvedKeyIndex);
  if (keyNum == -1){
    Serial.println("ошибка ключа перед записью кп");
    return 0;
  }
  approvedKeyIndex = keyNum;
  // Serial.println("Авторизовано");

  status = mfrc522.MIFARE_Read(2, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 0;
  }
  uint8_t prevStationNo = littleEndianToInt(&buffer[0], 1);
  uint8_t addr = littleEndianToInt(&buffer[1], 1);
  
  // dump_byte_array(buffer, 16);
  if (prevStationNo == StationNo){
    //signal уже записано
    Serial.println("Уже записано");
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    return 1;
  }
  addr = getNextAddr(addr);
  if (addr < 4) {addr = 4;}
  else if (addr > 62){return 0;}
  intToLittleEndian(StationNo, &buffer[0], 1);
  intToLittleEndian(addr,     &buffer[1], 1);
  // Serial.println();
  // dump_byte_array(buffer, 16);
  if(!safe_write(2, buffer, 16)){
    return 0;
  }
  /*
  status = mfrc522.MIFARE_Write(2, buffer, 16);
  // iter = 0;
  if (status != MFRC522::STATUS_OK ) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  */
  // buffer[1] = 0;
  intToLittleEndian(rtc.now().unixtime(), &buffer[1], 4);
  intToLittleEndian(millis()%1000, &buffer[5], 2);
  keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, addr, approvedKeyIndex); // getTrailerSectorAddr(addr)
  if (keyNum == -1){
    Serial.println("ошибка ключа при записи кп");
    //signal ошибка ключа
    return 0;
  }
  if(!safe_write(addr, buffer, 16)){
    return 0;
  }
  /*status = mfrc522.MIFARE_Write(addr, buffer, 16);
  // iter = 0;
  if (status != MFRC522::STATUS_OK ) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }*/
  //signal success check in
  digitalWrite(ledPin, HIGH);
  Serial.println("success check in");
  delay(50);
  digitalWrite(ledPin, LOW);
  // StationNo += 1;
  return 1;

}

























