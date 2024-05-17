String tst[45];
bool con = false;
String number = " "; 

void setup() {Serial.begin(9600);}

void loop() {

  switch (Serial.read()) {

    case '1':
      Serial.println(2);               
      break;

    case '2':
      for(int j = 0; j < 20; j++){
        tst[j] = random(1713893429, 1913893429);      // имитация чтения карты
      }
      sndtoPC(tst);                
      break;

    case '3':
    Serial.println(readfromPC());
      break;

    case '4':
    // функция записи номера (переменная number)
      break;  

  }
  
  delay(10);
}


void sndtoPC(String msg[]){      //отправка одного пакета 

   Serial.print("s");           //начало пакета 
   Serial.println(); 

      for(int i = 0; i < 45; i++){
        Serial.print(" ");
        Serial.println(msg[i]);
      }

    Serial.println("e");           //конец пакета
}



String readfromPC(){ 
  
  number = " ";  

  while(number.length() < 2){
        number = Serial.readString();
  }

  return(number);
}