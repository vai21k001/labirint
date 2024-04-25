#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         D4
#define SS_PIN          D8
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

MFRC522::MIFARE_Key KeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key KeyB = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key NewKeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key NewKeyB = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int accessTypeDataBlock = 4;    //0, 4
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
  dump_byte_array(KeyA.keyByte, MFRC522::MF_KEY_SIZE);
  dump_byte_array(KeyB.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop() {
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

bool prepareMifare1k(){
  int trailerBlock = 3;
  MFRC522::StatusCode status;
  byte trailerBuffer[] = {
    NewKeyA.keyByte[0], NewKeyA.keyByte[1], NewKeyA.keyByte[2], NewKeyA.keyByte[3], NewKeyA.keyByte[4], NewKeyA.keyByte[5],       // Keep default key A
    0, 0, 0,
    0,
    NewKeyB.keyByte[0], NewKeyB.keyByte[1], NewKeyB.keyByte[2], NewKeyB.keyByte[3], NewKeyB.keyByte[4], NewKeyB.keyByte[5]};      // Keep default key B
  
  //Auth and prepare Sector 0
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &KeyB, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  //set access conditions
  mfrc522.MIFARE_SetAccessBits(&trailerBuffer[6], 1, 1, accessTypeDataBlock, accessTypeTrailerBlock); //set custom acces for block 1
  writeTrailerSector(trailerBuffer, trailerBlock);
  //Номер чипа должен быть уже записан
  formatValueBlock(2); //format first station

  //prepare trailerBuffer for writing
  mfrc522.MIFARE_SetAccessBits(&trailerBuffer[6], accessTypeDataBlock, accessTypeDataBlock, accessTypeDataBlock, accessTypeTrailerBlock);
  for (int sector=1; sector<16; sector++){
    trailerBlock = sector * 4 + 3;
    Serial.print("Writting sector ");
    Serial.println(sector);
    //auth read
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &KeyB, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
    writeTrailerSector(trailerBuffer, trailerBlock);//set access conditions
    //format 0-2 blocks in the sector
    for (int block=0; block < 3; block++){
      formatValueBlock(sector * 4 + block);
    }
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
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
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
    }
}