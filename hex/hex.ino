void setup() {
  Serial.begin(9600);
  
  uint32_t number = 100; // Ваше число
  byte byteArray[4]; // Массив для хранения байт
  for (int i=0; i<4; i++){
    byteArray[i] = 0;
  }
  intToLittleEndian(number, byteArray, sizeof(byteArray));

  Serial.print("Массив байт: ");
  dump_byte_array(byteArray, 4);

  byte newByteArray[4];
  // uint32_t a = 
  Serial.println();
  Serial.print(littleEndianToInt(byteArray, sizeof(byteArray)));
}

void loop() {
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
        number |= (uint32_t)byteArray[i] << (i * 8); // Собираем число из байтов
    }
    return number;
}
