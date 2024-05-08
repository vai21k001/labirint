#define led 22 
#define buz 16

void setup() {
 pinMode(led,OUTPUT);
 pinMode(buz,OUTPUT);
}

void loop() {

}

void scWrite(){
  tone(buz,700);
  digitalWrite(led,1);
  delay(100);
  tone(buz,0);
  digitalWrite(led,0);
}

void error(){
  tone(buz,200);
  digitalWrite(led,1);
  delay(200);
  tone(buz,0);
  digitalWrite(led,0);
}

void ons(){
  tone(buz,1200);
  digitalWrite(led,1);
  delay(200);
  tone(buz,0);
}


