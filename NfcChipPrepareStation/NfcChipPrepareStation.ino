#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         D4
#define SS_PIN          D8
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

MFRC522::MIFARE_Key KeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Лишнее
MFRC522::MIFARE_Key KeyB = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key NewKeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key NewKeyB = {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC};

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

int accessTypeDataBlock = 0;    //0, 4
int accessTypeTrailerBlock = 1;  //1, 3 

void setup() {
  Serial.begin(9600);	// Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();		    // Init SPI bus
  mfrc522.PCD_Init();	// Init MFRC522 card
  delay(4);				    // Optional delay. Some board do need more time after init to be ready, see Readme

  // KeyA.keyByte = ;  //Set keys values
  // KeyB.keyByte = ;

  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  Serial.println("\nScript for PREPARE STATION ONLY!");
  Serial.println("Keys for write (A and B):");
  dump_byte_array(NewKeyA.keyByte, MFRC522::MF_KEY_SIZE);
  dump_byte_array(NewKeyB.keyByte, MFRC522::MF_KEY_SIZE);
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

  bool prepareStatus;
  prepareStatus = prepareMifare1k();
  if (!prepareStatus){
    Serial.println("Prepare error");
  }else{
    Serial.println("Prepare complete");
  }

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
  Serial.println("Попытка использовать ключ:");
  // byte buffer[] = {key[0], key[1], key[2], key[3], key[4], key[5]}
  dump_byte_array(key.keyByte, 6);
  status = mfrc522.PCD_Authenticate(keyType, block, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  Serial.print(F("Success with key:"));
  dump_byte_array((key).keyByte, MFRC522::MF_KEY_SIZE);
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

bool prepareMifare1k(){
  int trailerBlock = 3;
  MFRC522::StatusCode status;
  int iter;
  byte trailerBuffer[] = {
    NewKeyA.keyByte[0], NewKeyA.keyByte[1], NewKeyA.keyByte[2], NewKeyA.keyByte[3], NewKeyA.keyByte[4], NewKeyA.keyByte[5],
    0, 0, 0,
    0,
    NewKeyB.keyByte[0], NewKeyB.keyByte[1], NewKeyB.keyByte[2], NewKeyB.keyByte[3], NewKeyB.keyByte[4], NewKeyB.keyByte[5]};
  
  //Auth and prepare Sector 0

  int keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, trailerBlock, approvedKeyIndex);
  if (keyNum == -1){
    Serial.println("ошибка ключа перед записью кп");
    return false;
  }
  approvedKeyIndex = keyNum;

  Serial.println("Writting sector 0");
  // formatValueBlock(2); //format first station
  addrBlockPrepare(); //set to clear parameters of card status

  //set access conditions
  mfrc522.MIFARE_SetAccessBits(&trailerBuffer[6], 0, 0, accessTypeDataBlock, accessTypeTrailerBlock); //set custom acces for block 1
  writeTrailerSector(trailerBuffer, trailerBlock);
  //Номер чипа должен быть уже записан


  //prepare trailerBuffer for writing
  mfrc522.MIFARE_SetAccessBits(&trailerBuffer[6], accessTypeDataBlock, accessTypeDataBlock, accessTypeDataBlock, accessTypeTrailerBlock);
  for (int sector=1; sector<16; sector++){
    trailerBlock = sector * 4 + 3;
    Serial.print("Writting sector ");
    Serial.println(sector);
    //auth read
    keyNum = fastAuth(MFRC522::PICC_CMD_MF_AUTH_KEY_B, keysArr, amountOfKeys, trailerBlock, approvedKeyIndex);
    if (keyNum == -1){
      Serial.println("ошибка ключа перед записью кп");
      return false;
    }
    
    for (int block=0; block < 3; block++){
      formatValueBlock(sector * 4 + block);
    }
    writeTrailerSector(trailerBuffer, trailerBlock);//set access conditions
  }
  return true;
}


void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}



void writeTrailerSector(byte *trailerBuffer, byte trailerBlock){
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  int iter;

  // Read the sector trailer as it is currently stored on the PICC
  // Serial.println(F("Reading sector trailer..."));
  status = mfrc522.MIFARE_Read(trailerBlock, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  // Check if it matches the desired access pattern already;
  // because if it does, we don't need to write it again...
  if (    buffer[0] != trailerBuffer[0]
      ||  buffer[1] != trailerBuffer[1]
      ||  buffer[2] != trailerBuffer[2]
      ||  buffer[3] != trailerBuffer[3]
      ||  buffer[4] != trailerBuffer[4]
      ||  buffer[5] != trailerBuffer[5]
      ||  buffer[6] != trailerBuffer[6]
      ||  buffer[7] != trailerBuffer[7]
      ||  buffer[8] != trailerBuffer[8]
      ||  buffer[9] != trailerBuffer[9]
      ||  buffer[10] != trailerBuffer[10]
      ||  buffer[11] != trailerBuffer[11]
      ||  buffer[12] != trailerBuffer[12]
      ||  buffer[13] != trailerBuffer[13]
      ||  buffer[14] != trailerBuffer[14]
      ||  buffer[15] != trailerBuffer[15]) {
    // They don't match (yet), so write it to the PICC
    Serial.println(F("Writing new sector trailer..."));
    status = mfrc522.MIFARE_Write(trailerBlock, trailerBuffer, 16);
    // iter = 0;
    if (status != MFRC522::STATUS_OK ) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    delay(30);
  }
}

void formatValueBlock(byte blockAddr) {
    byte buffer[18];
    byte size = sizeof(buffer);
    MFRC522::StatusCode status;

    // Serial.print(F("Reading block ")); Serial.println(blockAddr);
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    if (    (buffer[0] == 0)
        &&  (buffer[1] == 0)
        &&  (buffer[2] == 0)
        &&  (buffer[3] == 0)
        &&  (buffer[4] == 0)
        &&  (buffer[5] == 0)
        &&  (buffer[6] == 0)
        &&  (buffer[7] == 0)
        &&  (buffer[8] == 0)
        &&  (buffer[9] == 0)
        &&  (buffer[10] == 0)
        &&  (buffer[11] == 0)
        &&  (buffer[12] == 0)
        &&  (buffer[13] == 0)
        &&  (buffer[14] == 0)
        &&  (buffer[15] == 0) ){
        // Serial.println(F("Block has correct Value Block format."));
    }
    else {
        // Serial.println(F("Formatting as Value Block..."));
        byte valueBlock[] = {
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0 };
        status = mfrc522.MIFARE_Write(blockAddr, valueBlock, 16);
        if (status != MFRC522::STATUS_OK) {
            Serial.print(F("MIFARE_Write() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
        }
        delay(30);
    }
}

void addrBlockPrepare(){
  Serial.println("Write addr");
  byte buff[2];
  intToLittleEndian(2, buff, 1);
  byte valueBlock[] = {
    0, buff[0], 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0 };
  dump_byte_array(valueBlock, 16);
  MFRC522::StatusCode status;
  status = mfrc522.MIFARE_Write(2, valueBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  delay(30);
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
