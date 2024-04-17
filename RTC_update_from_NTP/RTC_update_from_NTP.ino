#include <RTClib.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>  


RTC_DS3231 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ru.pool.ntp.org", 10800, 60000);


char t[32];
byte last_second, second_, minute_, hour_, day_, month_;
int year_;


void setup()
{
  Serial.begin(9600);  // Initialize serial communication with a baud rate of 9600
  Wire.begin();  // Begin I2C communication
  rtc.begin();  // Initialize DS3231 RTC module


  // Connect to WiFi
  char ssid[] = "bas"; // Replace with your WiFi SSID
  char password[] = "00009999"; // Replace with your WiFi password
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");


  timeClient.begin();  // Start NTP client
  timeClient.update();  // Retrieve current epoch time from NTP server
  unsigned long unix_epoch = 1776407602;  // Get epoch time
  rtc.adjust(DateTime(unix_epoch));  // Set RTC time using NTP epoch time

  // updateRTC(&timeClient, &rtc); //Update current time on rtc


  DateTime now = rtc.now();  // Get initial time from RTC
  last_second = now.second();  // Store initial second
}


void loop()
{
  DateTime rtcTime = rtc.now();  // Get current time from RTC

  // second_rtc = ;  // Extract second from epoch time
  if (last_second != rtcTime.second())
  {
    timeClient.update();  // Update time from NTP server
    unsigned long unix_epoch = timeClient.getEpochTime();  // Get current epoch time
    minute_ = minute(unix_epoch);  // Extract minute from epoch time
    hour_ = hour(unix_epoch);  // Extract hour from epoch time
    day_ = day(unix_epoch);  // Extract day from epoch time
    month_ = month(unix_epoch);  // Extract month from epoch time
    year_ = year(unix_epoch);  // Extract year from epoch time
    second_ = second(unix_epoch);  // Extract second from epoch time


    // Format and print NTP time on Serial monitor
    sprintf(t, "NTP Time: %02d:%02d:%02d %02d/%02d/%02d", hour_, minute_, second_, day_, month_, year_);
    Serial.println(t);
    // Serial.println(unix_epoch);

    // Format and print RTC time on Serial monitor
    sprintf(t, "RTC Time: %02d:%02d:%02d %02d/%02d/%02d", rtcTime.hour(), rtcTime.minute(), rtcTime.second(), rtcTime.day(), rtcTime.month(), rtcTime.year());
    Serial.println(t);
    // Serial.println(rtcTime.unixtime());

    // Compare NTP time with RTC time
    if (rtcTime == DateTime(year_, month_, day_, hour_, minute_, second_))
    {
      Serial.println("Time is synchronized!\n");  // Print synchronization status
    }
    else
    {
      Serial.println("Time is not synchronized!########\n");  // Print synchronization status
      updateRTC(&timeClient, &rtc);
    }


    last_second = second_;  // Update last second
  }

  delay(1000);  // Delay for 1 second before the next iteration
}

void updateRTC(NTPClient *_timeClient, RTC_DS3231 *_rtc){
  Serial.println("Time syncromizing...");
  (*_timeClient).update();  // Update time from NTP server
  unsigned long unix_epoch = (*_timeClient).getEpochTime();  // Get current epoch time
  (*_rtc).adjust(DateTime(unix_epoch));

  DateTime rtcTime = (*_rtc).now();
  Serial.println("Time is successfully synchronized!");
  sprintf(t, "Current: %02d:%02d:%02d %02d/%02d/%02d\n", rtcTime.hour(), rtcTime.minute(), rtcTime.second(), rtcTime.day(), rtcTime.month(), rtcTime.year());
  Serial.println(t);
}