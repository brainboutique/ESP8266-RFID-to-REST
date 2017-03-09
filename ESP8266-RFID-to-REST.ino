#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include "MFRC522.h"

/* wiring the MFRC522 to ESP8266 (ESP-12)
RST     = GPIO5
SDA(SS) = GPIO4 
MOSI    = GPIO13
MISO    = GPIO12
SCK     = GPIO14
GND     = GND
3.3V    = 3.3V
*/

#define RST_PIN	5  // RST-PIN for RC522 - RFID - SPI - Modul GPIO5 
#define SS_PIN	4  // SDA-PIN for RC522 - RFID - SPI - Modul GPIO4 

#define BUTTON1_PIN 0 // NodeMCU: ==D3 

#define LED     D4 // Heartbeat LED

//const int maxWifiConnectRetries=10;
struct config_t
{
                    // Feel free to enable your defaults here!
    char ssid[255];// =  "foo"; // change default according to your Network - cannot be longer than 32 characters!
    char pass[255];// =  "bar"; // change according to your Network
    char url[255];//  =  "http://192.168.2.117:8989";
} configuration;

MFRC522 mfrc522(SS_PIN, RST_PIN);	


String toStr(byte *buffer, byte bufferSize) {
  String s="";
  for (byte i = 0; i < bufferSize; i++) {
    if (i>0) s+=":";
    if (buffer[i]<10) s+="0";
    s+=String(buffer[i], HEX);
    
  }
  s.toUpperCase();
  return(s);
}
void storeStruct(void *data_source, size_t size)
{
  EEPROM.begin(size * 2);
  for(size_t i = 0; i < size; i++)
  {
    char data = ((char *)data_source)[i];
    EEPROM.write(i, data);
  }
  EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size)
{
    EEPROM.begin(size * 2);
    for(size_t i = 0; i < size; i++)
    {
        char data = EEPROM.read(i);
        ((char *)data_dest)[i] = data;
    }
}
void dumpConfig() {
  Serial.println("SSID: '"+String(configuration.ssid)+"'");
  Serial.println("PASS: *"); //'"+String(configuration.pass)+"'");
  Serial.println("URL: '"+String(configuration.url)+"'");
  if (WiFi.status()==WL_CONNECTED) {
    Serial.println("WiFi connected!");
  }
  else {
    Serial.println("WiFi **not** connected!");
  } 
}

void loadConfig() {
  //EEPROM.get(0,configuration);  // Warning: For whatever reason NOT working!
  loadStruct(&configuration,sizeof(configuration));
  Serial.println("Loaded config: ");
  dumpConfig();
}

void saveConfig() {
  //EEPROM.put(0,configuration);  // Warning: For whatever reason NOT working!
  storeStruct(&configuration,sizeof(configuration));
  Serial.println("Config saved.");
}


void connectWifi() {
  Serial.println("Connecting to Wifi "+String(configuration.ssid));

  WiFi.mode(WIFI_STA);
  WiFi.begin(configuration.ssid,configuration.pass);

  int retries=0;
  while (WiFi.status()!=WL_CONNECTED && retries<10) {
    retries++;
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  
  if (WiFi.status()==WL_CONNECTED) {
    Serial.println("WiFi connected to AP "+String(configuration.ssid));
  }
  else {
    Serial.println("Warning! Was not able to connect to WIFI:  "+String(configuration.ssid)+" - Status "+WiFi.status());
  }
}

void help() {
  Serial.println(F("======================================================")); 
  Serial.println(F("Configuration usage:")); 
  Serial.println(F("On serial port, you can issue following setup commands. Settings are persisted.")); 
  Serial.println(F("ssid=<YourSSID>")); 
  Serial.println(F("pass=<YourWLANPassword>")); 
  Serial.println(F("url=<YourURLForPOST>     - must be style like http://foo.bar.com:12345/exx")); 
  Serial.println(F("connect                  - Attempt to connect to WIFI. Only call after updating SSID and pass")); 
  Serial.println(F("info                     - Show configuration values")); 
  Serial.println(F("help                     - This screen")); 
  Serial.println(F("======================================================")); 
  dumpConfig();
  Serial.println(F("======================================================")); 
}
void setup() {
  Serial.begin(9600);    // Initialize serial communications
  Serial.setTimeout(200);
  while (!Serial) ;
  delay(3000);

  pinMode(LED,OUTPUT);
  
  pinMode(BUTTON1_PIN, INPUT);    
  digitalWrite(BUTTON1_PIN, HIGH);   
  
  SPI.begin();           // Init SPI bus
  mfrc522.PCD_Init();    // Init MFRC522

  Serial.print("Setting up...");
  loadConfig();

  help();
  connectWifi();

  //Serial.println("Posting found cards to "+String(configuration.url));
  Serial.println(F("Initialization completed. Waiting for card/tag..."));
}

int button1=0;
int counter=0;
int ledFlashFreq=5;

void post(String value) {
  digitalWrite(LED,LOW);
  Serial.println("POSTing to "+String(configuration.url)+": "+value);
  HTTPClient http;  
  http.begin(String(configuration.url)); 
  int httpCode =http.POST(value+"\n");
  Serial.print(F("HTTP POST Result: "));
  Serial.print(httpCode);
  Serial.println();
  delay(1000); // Wait a second to prevent brute-force attacks!
  http.end();
  digitalWrite(LED,HIGH);
}

void loop() { 
  counter++;
  if ((counter/ledFlashFreq) % 2 == 0) {
        digitalWrite(LED,LOW);
  }
  else
  {
        digitalWrite(LED,HIGH);
  }
  
  if (digitalRead(BUTTON1_PIN)!=HIGH) {
    if (button1==0)
      post("uid=button1");
    button1++;
  }
  else {
    button1=0;
  }

/*
if (digitalRead(BUTTON1_PIN)!=HIGH) {
  Serial.print("+"); } else { Serial.print("-");    }*/

  //Serial.print("?");
  if (Serial.available()) {
    
    String s=Serial.readString();
    s.trim();
    if (s.startsWith("ssid=")) {
      s.substring(5).toCharArray(configuration.ssid,s.length()-5+1);
      saveConfig();      
    } else if (s.startsWith("pass=")) {
      s.substring(5).toCharArray(configuration.pass,s.length()-5+1);
      saveConfig();
    } else if (s.startsWith("url=")) {
      s.substring(4).toCharArray(configuration.url,s.length()-4+1);
      saveConfig();
    } else if (s.startsWith("connect")) {
      Serial.println("Resetting WiFi... If this does not work, please power-cycle or reset board.");
      WiFi.disconnect(); 
      WiFi.mode(WIFI_OFF);
      delay(300);
      connectWifi();
    } else if (s.startsWith("info")) {
      dumpConfig();
    } else if (s.startsWith("help")) {
      help();
    }  else {
      Serial.println("Unknown command: "+s);
    }

  }
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  Serial.print(F("Card UID:"));
  String uid=toStr(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println(uid);
  Serial.println();

  post("uid="+uid);
}



