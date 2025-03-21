#include <Wire.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <string> 
#include <Adafruit_NeoPixel.h>

// I2C address for RV3028
const int RV3028_ADDR = 0x52;

// Set up NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// Network credentials
const char* ssid = "combination to my luggage";
const char* password = "1 2 3 4 5";

// NTP Update variables
unsigned long lastNTPSync = 0;                  // Keeps track of the last time we queried the NTP server
const unsigned long ntpSyncInterval = 3600000;  // 1 hour

// Array for nixie digits
//                            0      1      2      3      4      5      6      7      8       9
unsigned int SymbolArray[10]={1,     2,     4,     8,     16,    32,    64,    128,   256,    512};

// Variables for shift register
const byte LEpin=5;                 // Latch Enable - data accepted while HIGH level
boolean UD, LD;                     // Separators
String stringToDisplay="000000";    // String displayed on nixies (must be 6 characters in length)
#define pinSHDN 16                  // Turns off nixie tubes when LOW
#define UpperDotsMask 0x80000000
#define LowerDotsMask 0x40000000

// Time variables
String thetime = "000000";  // Time displayed on nixie tubes (hrs+mins+sec)
int secs=0;                 // Seconds (used to determine if separators should be illuminated
String hrs;                 // Hours string for nixie tubes
String mins;                // Minutes string for nixie tubes
String sec;                 // Seconds string for nixie tubes

// Settings for UTC Offsets
// Example: Eastern is -5 Eastern Standard -4 Eastern Daylight Savings 
float UTCOffsetStandard = -5;    // UTC Offset Standard Time
float UTCOffsetDaylight = -4;    // UTC Offset Summer Time/Daylight Savings time

//Setup NeoPixel for the LEDs under the tubes
#define PIN            27
#define NUMPIXELS      8
#define LEDsSpeed      10
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    
    Serial.begin(115200);
    Serial.println(F("Starting"));

    // Setup the shift register for the nixie tubes
    pinMode(LEpin, OUTPUT);
    pinMode(pinSHDN, OUTPUT);
    digitalWrite(pinSHDN, HIGH);  //turn on the nixie tubes
  
    // SPI setup
    SPI.begin();  
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  
    // I2C setup
    Wire.begin();
    WiFi.begin(ssid, password);

    // WiFi setup
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Start NeoPixels and turn off all the RGB LEDs
    pixels.begin();
    pixels.clear();
    pixels.show();

    // Startup NTP
    timeClient.begin();
    timeClient.update();  // Sync NTP time immediately

    // Update RTC from NTP with timezone adjustment
    updateRTCFromNTP();
    lastNTPSync = millis();
}

void loop() {
    // Print RTC time every second
    printRTCTime();

    //Turn on the digit separators every even second
    if (secs%2 == 0){
      LD=true;
      UD=true;
    }
    else{
      LD=false;
      UD=false;
    }
    
    //Serial.println(thetime);

    stringToDisplay=thetime;
    doIndication();
  
    delay(1000);

    // Sync with NTP every hour
    if (millis() - lastNTPSync >= ntpSyncInterval) {
        timeClient.update();
        updateRTCFromNTP();
        lastNTPSync = millis();
    }
}

//Get time from NTP and send it to RTC module
void updateRTCFromNTP() {
    Serial.println("Forcing an NTP update...");
    timeClient.forceUpdate();  // Force sync with NTP server

    Serial.print("Updated Time: ");
    Serial.println(timeClient.getFormattedDate());
    String formattedDate = timeClient.getFormattedDate();  // YYYY-MM-DDTHH:MM:SSZ

    // Extract date and time components
    int year = formattedDate.substring(0, 4).toInt();
    int month = formattedDate.substring(5, 7).toInt();
    int day = formattedDate.substring(8, 10).toInt();
    int hour = formattedDate.substring(11, 13).toInt();
    int minute = formattedDate.substring(14, 16).toInt();
    int second = formattedDate.substring(17, 19).toInt();
    int dayOfWeek = timeClient.getDay();  // 0=Sunday, 6=Saturday

    // Sanity Check Date (NTP Sync fail will set date to 1970)
    if(year<1980){
      updateRTCFromNTP();
      Serial.println("Trying NTP Again");
    }

    // Apply Eastern Time (UTC-5 Standard / UTC-4 DST)
    bool isDST = isDaylightSavingTime(year, month, day, hour);
    int offset = isDST ? UTCOffsetDaylight : UTCOffsetStandard;  // Convert UTC to Eastern Time
    hour += offset;

    // Handle day/month/year rollovers
    if (hour < 0) {
        hour += 24;
        day--;
    } else if (hour >= 24) {
        hour -= 24;
        day++;
    }

    // Print adjusted time
    Serial.printf("Syncing RTC with NTP (Adjusted for EST/EDT): Year: %d Month: %d Day: %d Hour: %d Minute: %d Second: %d Day of Week: %d (DST: %s)\n",
                  year, month, day, hour, minute, second, dayOfWeek, isDST ? "Yes" : "No");

    // Write the time to the RTC (convert to BCD)
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(0x00);  // Start at seconds register
    Wire.write(toBCD(second));  
    Wire.write(toBCD(minute));  
    Wire.write(toBCD(hour));    
    Wire.write(toBCD(dayOfWeek + 1)); 
    Wire.write(toBCD(day));     
    Wire.write(toBCD(month));   
    Wire.write(toBCD(year % 100));
    Wire.endTransmission();

    Serial.println("RTC updated!");
}

//Read time from RTC, modify for 12hr display on nixie tubes, and give seconds to determine if separators should be illuminated
void printRTCTime() {
    Wire.beginTransmission(RV3028_ADDR);
    Wire.write(0x00);  // Start reading from register 0x00 (seconds)
    Wire.endTransmission();
    Wire.requestFrom(RV3028_ADDR, 7);  // Read 7 bytes (seconds to year)

    if (Wire.available() == 7) {
        int second = fromBCD(Wire.read());
        int minute = fromBCD(Wire.read());
        int hour = fromBCD(Wire.read());
        int dayOfWeek = fromBCD(Wire.read());
        int day = fromBCD(Wire.read());
        int month = fromBCD(Wire.read());
        int year = fromBCD(Wire.read()) + 2000;

    //    Serial.printf("RTC Time: %04d-%02d-%02d %02d:%02d:%02d (DOW: %d)\n", 
    //                  year, month, day, hour, minute, second, dayOfWeek);

    //12 hr time and leading zero logic
    
    if(hour>21){
      hrs = String(hour-12);
      }
    else if(hour>12){
      hrs = "0" + String(hour-12);
      }
    else if(hour==0){
      hrs = "12";
    }
    else if(hour<10){
      hrs = "0" + String(hour);
      }
    else{
      hrs = String(hour);
    }
  
    if(minute<10){
      mins = "0" + String(minute);
      }
    else{
      mins = String(minute);
    }
  
    if(second<10){
      sec = "0" + String(second);
      }
    else{
      sec = String(second);
    }

    //assemble the string that gets sent to the nixie tubes              
    thetime=hrs+mins+sec;

    //seconds used to determine the state of the digit separators
    secs=second;
    }
}

// Convert to Binary-Coded Decimal (BCD)
uint8_t toBCD(int val) {
    return ((val / 10) << 4) + (val % 10);
}

// Convert from Binary-Coded Decimal (BCD)
int fromBCD(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

// Function to determine if DST is active (USA rules)
bool isDaylightSavingTime(int year, int month, int day, int hour) {
    // DST starts at 2 AM on the second Sunday of March
    // DST ends at 2 AM on the first Sunday of November

    if (month < 3 || month > 11) return false;  // Jan, Feb, Dec -> No DST
    if (month > 3 && month < 11) return true;   // Apr - Oct -> DST

    int firstSunday = 1;
    while (dayOfWeek(year, month, firstSunday) != 0) firstSunday++;  // Find first Sunday

    if (month == 3) {  // March
        int secondSunday = firstSunday + 7;
        return (day > secondSunday || (day == secondSunday && hour >= 2));
    } else {  // November
        return !(day < firstSunday || (day == firstSunday && hour < 2));
    }
}

// Calculate the day of the week (0=Sunday, 6=Saturday) using Zeller's Congruence
int dayOfWeek(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int K = year % 100;
    int J = year / 100;
    return (day + (13 * (month + 1)) / 5 + K + (K / 4) + (J / 4) - (2 * J)) % 7;
}

//Send digits to nixie tubes & control separators
void doIndication(){
  unsigned long Var32=0;
  unsigned long New32_L=0;
  unsigned long New32_H=0;
  
  long digits=stringToDisplay.toInt();
  
  /**********************************************************
   * Data format incoming [H1][H2}[M1][M2][S1][Y1][Y2]
   *********************************************************/
   
 //-------- REG 1 ----------------------------------------------- 
  Var32=0;
 
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // s2
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10])<<10; //s1
  digits=digits/10;

  Var32|=(unsigned long)(SymbolArray[digits%10]); //m2
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;  

  for (int i=1; i<=32; i++)
  {
   i=i+32;
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4); 
   i=i-32;
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }
 //-------------------------------------------------------------------------

 //-------- REG 0 ----------------------------------------------- 
  Var32=0;
  
  Var32|=(unsigned long)(SymbolArray[digits%10])<<20; // m1
  digits=digits/10;

  Var32|= (unsigned long)(SymbolArray[digits%10])<<10; //h2
  digits=digits/10;

  Var32|= (unsigned long)SymbolArray[digits%10]; //h1
  digits=digits/10;

  if (LD) Var32|=LowerDotsMask;
    else  Var32&=~LowerDotsMask;
  
  if (UD) Var32|=UpperDotsMask;
    else Var32&=~UpperDotsMask;  

  for (int i=1; i<=32; i++)
  {
   int newindex=16*(3-(ceil((float)i/4)*4-i))+ceil((float)i/4); 
   if (newindex<=32) bitWrite(New32_L, newindex-1, bitRead(Var32, i-1));
    else bitWrite(New32_H, newindex-32-1, bitRead(Var32, i-1));
  }

  SPI.transfer((New32_H)>>24);
  SPI.transfer((New32_H)>>16);
  SPI.transfer((New32_H)>>8);
  SPI.transfer(New32_H);
  
  SPI.transfer((New32_L)>>24);
  SPI.transfer((New32_L)>>16);
  SPI.transfer((New32_L)>>8);
  SPI.transfer(New32_L);

  digitalWrite(LEpin, HIGH); 
  digitalWrite(LEpin, LOW); 
//-------------------------------------------------------------------------
}
