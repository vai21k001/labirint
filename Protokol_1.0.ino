String tst[45];
bool con = false;
String number = " "; 
int a = 0;
int var = 0;

void setup() {Serial.begin(9600);}

void loop() {
    
       if(Serial.read() == '1'){
          Serial.println(2);               //автопоиск
          con = true;
        }

    if(Serial.read() == 'f'){
      
      // for(int j = 0; j < 20; j++){
       // tst[j] = random(1713893429, 1913893429);      // имитация чтения карты
      //}
      if(con){
        sndtoPC(tst);             
      }
    }
    if(Serial.read() == 'r'){
      while(Serial.available() > 0){
        number = Serial.readString();
        Serial.println(number);
      }
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

