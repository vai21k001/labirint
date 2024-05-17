/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read data from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
 * So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
 * details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
 * keep the PICCs at reading distance until complete.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         D4
#define SS_PIN          D8         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key KeyA = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
MFRC522::MIFARE_Key KeyB = {0x65, 0xB7, 0x2E, 0x22, 0x4D, 0xBC};

void setup() {
	Serial.begin(9600);		// Initialize serial communications with the PC
	while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop() {
	// Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  // MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  // mfrc522.PICC_DumpMifareClassicToSerial(&mfrc522.uid, piccType, &KeyA);
  
  byte sector         = 1;
  byte valueBlockA    = 5;
  byte valueBlockB    = 6;
  byte trailerBlock   = 3;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  int32_t value;
  
  for (int sector=15; sector>=0; sector--){
    trailerBlock = sector * 4 + 3;
    Serial.println("");
    // Serial.println(sector);
    //auth read
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &KeyB, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return ;
    }
    Serial.print(sector);
    for (int block=3; block>=0; block--){
      trailerBlock = sector * 4 + 3;
      status = mfrc522.MIFARE_Read(sector * 4 + block, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      Serial.print("\t");
      Serial.print(sector * 4 + block);
      Serial.print("\t");
      dump_byte_array(buffer, 16);
      Serial.println();
    }
  }
  

  // status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 11, &KeyA, &(mfrc522.uid));
  // if (status != MFRC522::STATUS_OK) {
  //   Serial.print(F("PCD_Authenticate() failed: "));
  //   Serial.println(mfrc522.GetStatusCodeName(status));
  //   return ;
  // }
  // for (int i = 11; i>7; i--){
  //   status = mfrc522.MIFARE_Read(i, buffer, &size);
  //   if (status != MFRC522::STATUS_OK) {
  //     Serial.print(F("MIFARE_Read() failed: "));
  //     Serial.println(mfrc522.GetStatusCodeName(status));
  //     return;
  //   }
  //   dump_byte_array(buffer, 16);
  //   Serial.println();
  // }
  // Serial.println();
	// // Dump debug info about the card; PICC_HaltA() is automatically called
  // status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 7, &KeyB, &(mfrc522.uid));
  // if (status != MFRC522::STATUS_OK) {
  //   Serial.print(F("PCD_Authenticate() failed: "));
  //   Serial.println(mfrc522.GetStatusCodeName(status));
  //   return ;
  // }
	// for (int i = 7; i>3; i--){
  //   status = mfrc522.MIFARE_Read(i, buffer, &size);
  //   if (status != MFRC522::STATUS_OK) {
  //     Serial.print(F("MIFARE_Read() failed: "));
  //     Serial.println(mfrc522.GetStatusCodeName(status));
  //     return;
  //   }
  //   dump_byte_array(buffer, 16);
  //   Serial.println();
  // }
  // Serial.println();
  // status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 3, &KeyB, &(mfrc522.uid));
  // if (status != MFRC522::STATUS_OK) {
  //   Serial.print(F("PCD_Authenticate() failed: "));
  //   Serial.println(mfrc522.GetStatusCodeName(status));
  //   return ;
  // }
	// for (int i = 3; i>=0; i--){
  //   status = mfrc522.MIFARE_Read(i, buffer, &size);
  //   if (status != MFRC522::STATUS_OK) {
  //     Serial.print(F("MIFARE_Read() failed: "));
  //     Serial.println(mfrc522.GetStatusCodeName(status));
  //     return;
  //   }
  //   dump_byte_array(buffer, 16);
  //   Serial.println();
  // }

  // status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, 7, &KeyB2, &(mfrc522.uid));
  // if (status != MFRC522::STATUS_OK) {
  //   Serial.print(F("PCD_Authenticate() failed: "));
  //   Serial.println(mfrc522.GetStatusCodeName(status));
  //   return ;
  // }

  // byte trailerBuffer[] = {
  //   0, 0, 0, 0, 0, 0,       // Keep default key A
  //   0, 0, 0,
  //   0,
  //   255, 255, 255, 255, 255, 255};      // Keep default key B
  // mfrc522.MIFARE_SetAccessBits(&trailerBuffer[6], 0, 0, 0, 1);
  // status = mfrc522.MIFARE_Write(7, trailerBuffer, 16);
  // if (status != MFRC522::STATUS_OK) {
  //           Serial.print(F("MIFARE_Write() failed: "));
  //           Serial.println(mfrc522.GetStatusCodeName(status));
  //       }

  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
