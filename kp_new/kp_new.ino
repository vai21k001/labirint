//RC522
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
//NTP & RTC
// #include <RTClib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define DS3231_ADDRESS 0x68
#define SECONDS_PER_DAY 86400L

//240 - start, 245 - finish, 248 - check, 249 - clear
#define STATION_NUMBER 37
#define REBOOT_TIME 5000

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} DateTime;

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

DateTime datetime;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ru.pool.ntp.org", 10800, 60000);

#define RC_CS_PIN 0
#define RC_RST_PIN 3
#define SDA_PIN 5
#define SCL_PIN 4
#define LED_PIN 2
#define BUZ_PIN 15 
MFRC522 mfrc522(RC_CS_PIN, RC_RST_PIN);  // Create MFRC522 instance

uint8_t StationNo = STATION_NUMBER;
#define STATION_NO_INCREMENT 0

int approvedKeyIndex = 0;
#define amountOfKeys 9
byte keysArr[amountOfKeys][MFRC522::MF_KEY_SIZE] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
  {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
  {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
  {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
  {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
  {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
  {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff} // AA BB CC DD EE FF
};



void setup() {
 	Serial.begin(115200);		// Initialize serial communications with the PC
	delay(5);
  // while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
  delay(5);
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_33dB); // было так в кп
  // mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_23dB_2);
  mfrc522.PCD_AntennaOff(); 
  delay(5);
  mfrc522.PCD_AntennaOn();
  delay(5);
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  Serial.println("\nScript for KP STATION ONLY!");
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZ_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);

    // Connect to WiFi
  char ssid[] = "wireless network"; // Replace with your WiFi SSID
  char password[] = "Vonf49TJxc3XnZRM"; // Replace with your WiFi password
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
    updateRTC(&timeClient, &datetime);
    Serial.println("RTC updated");
    //Stop All Connections
    timeClient.end(); 
  }
  else{
    Serial.println("No WIFI connection");
    signalyzeWifiError();
  }
  WiFi.disconnect();
  Serial.println("Stop connections");

  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg); //read register "VersionReg"
  if ((v == 0x00) || (v == 0xFF)){ 
    signalyzeError();
  } else {
    signalyzeReady();
  }
}

uint32_t  rebootTimer = millis();
uint32_t  tryTime = millis();

void loop() {
  if (millis() - rebootTimer >= REBOOT_TIME)
    rebootRFID();
  delay(50); // Упрощённо энергосбережение
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()){
    //Сигнализировать о том, что карта уже была считана
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  if (checkIn() == 1){
    signalyzeOK();
  }else {
    signalyzeError();
  }
  // bruteAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 2);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

uint8_t checkIn(){
  Serial.println("Начало отметки на станции");
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  int keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, 2, approvedKeyIndex);
  if (keyNum == -1){
    Serial.println("ошибка ключа перед записью кп");
    return -1;
  }
  approvedKeyIndex = keyNum;
  // Serial.println("Авторизовано");

  status = mfrc522.MIFARE_Read(2, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return -1;
  }

  // uint8_t prevStationNo = littleEndianToInt(&buffer[0], 1);
  // uint8_t addr = littleEndianToInt(&buffer[1], 1);
  uint8_t prevStationNo;
  uint8_t addr;
  memcpy(&prevStationNo, &buffer[0], 1);
  memcpy(&addr, &buffer[1], 1);
  Serial.println("Данные о записях:");
  Serial.println(prevStationNo);
  Serial.println(addr);
  
  // dump_byte_array(buffer, 16);
  if (prevStationNo == StationNo){
    //signal уже записано
    Serial.println("Уже записано");
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    return 1;
  }
  addr = getNextAddr(addr);
  if (addr < 4) {addr = 4;}
  else if (addr > 62){return 0;}
  memcpy(&buffer[0], &StationNo, 1);
  memcpy(&buffer[1], &addr,      1);
  // intToLittleEndian(StationNo, &buffer[0], 1);
  // intToLittleEndian(addr,     &buffer[1], 1);
  // Serial.println();
  // dump_byte_array(buffer, 16);
  //Запись номера станции и блока, куда будет она произведена
  if(!safe_write(2, buffer, 16)){
    return -1;
  }
  
  ds3231_getDateTime(&datetime);
  intToLittleEndian(dateTimeToUnix(&datetime), &buffer[1], 4);
  intToLittleEndian(millis()%1000, &buffer[5], 2);
  keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, addr, approvedKeyIndex); // getTrailerSectorAddr(addr)
  if (keyNum == -1){
    Serial.println("ошибка ключа при записи кп код -1");
    //signal ошибка ключа
    return -1;
  }
  if(!safe_write(addr, buffer, 16)){
    return -1;
  }
  /*status = mfrc522.MIFARE_Write(addr, buffer, 16);
  // iter = 0;
  if (status != MFRC522::STATUS_OK ) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }*/
  //signal success check in
  Serial.println("success check in");
  StationNo += STATION_NO_INCREMENT;
  return 1;
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

bool chipAuth(byte keyType, byte *key_, byte block){
  // int iters = 0;
  MFRC522::StatusCode status;
  MFRC522::MIFARE_Key key;
  for (int j=0; j<MFRC522::MF_KEY_SIZE; j++){
    key.keyByte[j] = key_[j];
  }
  Serial.println("Попытка использовать ключ:");
  // byte buffer[] = {key[0], key[1], key[2], key[3], key[4], key[5]}
  dump_byte_array(key.keyByte, 6);
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
  Serial.println("Использование последнего ключа");
  if (chipAuth(keyType, keys[keyIndex], block)){
    return keyIndex;
  }else{
    Serial.println("Использование последнего ключа не удалось. Идёт подбор...");
    return bruteAuth(keyType, keys, keysAmount_, block);
  }
}

void rebootRFID(){
  Serial.println("reboot");
  rebootTimer = millis();               // Обновляем таймер
  digitalWrite(RC_RST_PIN, HIGH);          // Сбрасываем модуль
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(RC_RST_PIN, LOW);           // Отпускаем сброс
  mfrc522.PCD_Init();                      // Инициализируем заного
  delay(5);
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_33dB);
  // mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_23dB_2);
  mfrc522.PCD_AntennaOff(); 
  delay(5);
  mfrc522.PCD_AntennaOn();
  delay(5);
  return;
}

void intToLittleEndian(uint32_t num, byte *array, int length){ //max 4 bytes
  for (int i=0; i<length; i++){
    array[i] = (num >> (8*i)) & 0xFF;
  }
}

//Не работает
uint32_t littleEndianToInt(byte* byteArray, int size) {
    uint32_t number = 0;
    for (int i = 0; i < size; i++) {
        number |= (uint32_t)byteArray[i] << (i * 8); // Собираем число из байтов
    }
    // return number;
    return -1;
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
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

void signalyzeReady(){
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
}
void signalyzeWifiError(){
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(400);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
}

void signalyzeError(){ 
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(75);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
  delay(75);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW); 
}
void signalyzeOK(){ 
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(BUZ_PIN, HIGH);
  delay(200);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZ_PIN, LOW);
 }

 
uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }

void printDateTime(DateTime *dt){
  char buffer[50];  // Буфер для хранения строки
  sprintf(buffer, "Date: %04d-%02d-%02d Time: %02d:%02d:%02d", dt->year, dt->month, dt->day, dt->hour, dt->minute, dt->second);
  Serial.println(buffer);
}

bool isLeapYear(uint16_t year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Инициализация DS3231
void ds3231_begin(uint8_t sda, uint8_t  scl) {
    Wire.begin(sda, scl);
}

void updateRTC(NTPClient *_timeClient, DateTime *dt)
{
    Serial.println("Time syncromizing...");
    (*_timeClient).update();  // Update time from NTP server
    unsigned long unix_epoch = (*_timeClient).getEpochTime();
    epochToDateTime(unix_epoch, dt);
    ds3231_setDateTime(dt);
    Serial.println("Time is successfully synchronized!");
}

// Установка времени на DS3231
void ds3231_setDateTime(const DateTime* dt) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x00); // Адрес регистра секунд
    Wire.write(bin2bcd(dt->second));
    Wire.write(bin2bcd(dt->minute));
    Wire.write(bin2bcd(dt->hour));
    Wire.write(0x07);
    Wire.write(bin2bcd(dt->day));
    Wire.write(bin2bcd(dt->month));
    Wire.write(bin2bcd(dt->year - 2000U)); // Год с 2000
    Wire.endTransmission();
}

// Чтение текущего времени с DS3231
void ds3231_getDateTime(DateTime* dt) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x00); // Адрес регистра секунд
    Wire.endTransmission();

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.requestFrom(DS3231_ADDRESS, 7);
    uint8_t time[7];
    uint8_t i = 0;
    while(Wire.available()) 
    {
      time[i] = Wire.read();
      i+=1;
    }
    if (i != 7){
      Serial.println(i);
    }
    Wire.endTransmission();
    
    Wire.beginTransmission(DS3231_ADDRESS); // начинаем новый обмен с DS3231
    Wire.write(byte(0x11)); // записываем адрес регистра, с которого начинается температура
    Wire.endTransmission(); // завершаем передачу
    i = 0; // обнуляем счётчик элементов массива
    byte temp[2]; // 2 байта для хранения температуры
    Wire.beginTransmission(DS3231_ADDRESS); // начинаем обмен с DS3231
    Wire.requestFrom(DS3231_ADDRESS, 2); // запрашиваем 2 байта у DS3231
    while(Wire.available()) 
    {
      temp[i] = Wire.read();
      i+=1;
    }
    Wire.endTransmission();

    dt->second = ((time[0] & 0x70) >> 4)*10 + (time[0] & 0x0F);
    dt->minute = ((time[1] & 0x70) >> 4)*10 + (time[1] & 0x0F);
    dt->hour   = ((time[2] & 0x30) >> 4)*10 + (time[2] & 0x0F);
    dt->day    = ((time[4] & 0x30) >> 4)*10 + (time[4] & 0x0F);
    dt->month  = ((time[5] & 0x30) >> 4)*10 + (time[5] & 0x0F);
    dt->year   = ((time[6] & 0xF0) >> 4)*10 + (time[6] & 0x0F) + 2000U;
}

void epochToDateTime(uint32_t epoch, DateTime* dt) {
    uint32_t seconds = epoch;
    // Расчет секунд, минут и часов
    dt->second = seconds % 60;
    seconds /= 60;
    dt->minute = seconds % 60;
    seconds /= 60;
    dt->hour = seconds % 24;

    // Расчет количества дней
    uint32_t days = seconds / 24;

    // Определяем год
    dt->year = 1970;
    while (days >= 365) {
        if (isLeapYear(dt->year)) {
            if (days >= 366) {
                days -= 366;
                dt->year++;
            } else {
                break;
            }
        } else {
            days -= 365;
            dt->year++;
        }
    }

    // Определяем месяц
    dt->month = 0;
    while (true) {
        uint8_t daysPerMonth = daysInMonth[dt->month];
        if (dt->month == 1 && isLeapYear(dt->year)) { // Если февраль и високосный год
            daysPerMonth = 29;
        }
        if (days < daysPerMonth) {
            break;
        }
        days -= daysPerMonth;
        dt->month++;
    }
    dt->month++; // Месяцы начинаются с 1

    // Определяем день
    dt->day = days + 1; // Дни начинаются с 1
}


uint32_t dateTimeToUnix(const DateTime* dt) {
    uint32_t unixTime = 0;
    uint16_t year = dt->year;
    
    // Добавляем секунды от 1970 до текущего года
    for (uint16_t y = 1970; y < year; y++) {
        unixTime += (isLeapYear(y) ? 366 : 365) * 86400; // Количество секунд в году
    }
    
    // Добавляем секунды за месяцы текущего года
    for (uint8_t m = 1; m < dt->month; m++) {
        unixTime += daysInMonth[m - 1] * 86400;
        // Если февраль и год високосный, добавляем 1 день
        if (m == 2 && isLeapYear(year)) {
            unixTime += 86400;
        }
    }

    // Добавляем секунды за дни текущего месяца
    unixTime += (dt->day - 1) * 86400;

    // Добавляем секунды за часы, минуты и секунды текущего дня
    unixTime += dt->hour * 3600;
    unixTime += dt->minute * 60;
    unixTime += dt->second;

    return unixTime;
}
