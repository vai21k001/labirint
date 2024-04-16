#include <WiFi.h>
#include <HTTPClient.h>
#include <RTClib.h>
#include <UnixTime.h>

UnixTime stamp(3);  // указать GMT (3 для Москвы)
RTC_DS3231 rtc;

char daysOfWeek[7][12] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char* ssid = "TP-LINK_E19E";
const char* password = "54199408";

//Your Domain name with URL path or IP address with path
String serverName = "https://www.timeapi.io/api/Time/current/zone?timeZone=Europe/Moscow";
String y,m;
String d,h;
String mn,s;

void setup() {
  Serial.begin(115200);
  pinMode(23, OUTPUT); 
 

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

   if (! rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    while (1);
  }
  setClock(httpRq());
  digitalWrite(23, HIGH);
}

void loop() {
  

  DateTime now = rtc.now();
  Serial.print("ESP32 RTC Date Time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);
  
  getUT(now.year(),now.month(),now.day(),now.hour(),now.minute(),now.second());

  delay(1000); // задержка 1 сек.
 
}

// cтчимся на сервак

String httpRq(){

  String payload,serverPath;

  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverName.c_str());
      
      // If you need Node-RED/server authentication, insert user and password below
      //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        payload = http.getString();                            
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }

return(payload);
} 

 //ставим время по серваку

void setClock (String payl){

  y=payl.substring(payl.indexOf('y')+6,payl.indexOf('y')+10); //год

  m =payl.substring(payl.indexOf('m')+7,payl.indexOf('m')+10); //месяц
  m.replace(",","");
  m.replace('"',' ');

  d =payl.substring(payl.indexOf('d')+5,payl.indexOf('d')+8); //день
  d.replace(",","");
  d.replace('"',' ');

  h =payl.substring(payl.indexOf('u')+4,payl.indexOf('u')+7); //часы
  h.replace(",","");
  h.replace('"',' ');

  mn =payl.substring(payl.indexOf('i')+7,payl.indexOf('i')+10); //минуты
  mn.replace(",","");
  mn.replace('"',' ');

  s =payl.substring(payl.indexOf('s')+9,payl.indexOf('s')+11); //секунды
  s.replace(",","");
  s.replace('"',' ');

  rtc.adjust(DateTime(y.toInt(), m.toInt(), d.toInt(),h.toInt(),mn.toInt(),s.toInt()));

}

 // Unix время

long getUT(int year,int month, int day, int hour,int min,int sc){
  stamp.setDateTime(year,month,day,hour,min,sc);                      
  uint32_t unix = stamp.getUnix();
  Serial.println(unix);
return(unix);
}