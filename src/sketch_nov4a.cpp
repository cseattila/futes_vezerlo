/*

Kincony KC868-A4

Relays example
*/
#include "wifi_tool.h"

#include "soundtool.h"

#include <HTTPClient.h>

#include <ArduinoJson.h>

#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Preferences preferences;

byte pins[] = {2, 15, 5, 4};
float currentTemperature=20;

int ido =0;

const int TONE_PWM_CHANNEL = 0;
#define ONE_WIRE_BUS 13
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Replace with your network credentials
const char* defaut_ssid = "csengerilak";
const char* default_password = "fejbecsap6lak";


void setup() {
  Serial.begin(115200);
  Serial.println(F("Start csengeri futes verzerlo"));

  pinMode(pins[0], OUTPUT);

  pinMode(pins[1], OUTPUT);

  pinMode(pins[2], OUTPUT);

  pinMode(pins[3], OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  sensors.begin();

}


int incomingByte = 0;
int incomedByte = 0;
int conection_lost =0;
char currentcmd[60];

void processCmd (char* cmd);

void habldeCmd () {
   if (Serial.available() > 0) {
    // read the incoming byte:
    incomingByte = Serial.read();

    // say what you got:
    Serial.printf("%c",incomingByte);
    
 
 if (incomingByte == 13) {
    currentcmd[incomedByte] =0;
    incomedByte = 0;
    
    Serial.printf("I received: %s\r\n", currentcmd);
    processCmd(currentcmd);
    } else {
    currentcmd[incomedByte] = incomingByte;
    incomedByte++;
    }
  }
}




void processCmd (char* cmd) {
  if (strcmp("relay 1 on",cmd) == 0) {
    Serial.printf("Relay-t kapcsoltam \r\n");
    digitalWrite(pins[0], HIGH);
  } else if (strcmp("relay 1 off",cmd) == 0) {
    Serial.printf("Relay-t kapcsoltam \r\n");
    digitalWrite(pins[0], LOW);
  } else if (strcmp("relay 2 on",cmd) == 0) {
    Serial.printf("Relay-t kapcsoltam \r\n");
    digitalWrite(pins[1], HIGH);
  } else if (strcmp("relay 2 off",cmd) == 0) {
    Serial.printf("Relay-t kapcsoltam \r\n");
    digitalWrite(pins[1], LOW);
  }else if (strcmp("csip",cmd) == 0) {
   beep();
  }else if (strcmp("save wifi",cmd) == 0) {
 

  }
  else if (strcmp("wifiinit",cmd) == 0) {
    initWifi();
  }
  else if (strcmp("scan",cmd) == 0) {
    wifiScan();
  }
  else if (strcmp("con",cmd) == 0){
    connectWiFi();

  }
}

String getDataFromAPI();

void loop() {
  habldeCmd();
  ido++;
  delay(10);
  if (ido > 200) {
    ido = 0;
    if (WiFi.status() == WL_CONNECTED) {
      String text = getDataFromAPI();
      //Display the result on the screen.
      Serial.printf("response: %s\n",text.c_str());
    } else {
      Serial.printf("Nincs wifi");
        beep();
        connectWiFi();
        bep();
    }
      sensors.requestTemperatures();  // Kérés a hőmérsékletet az érzékelőtől
currentTemperature = sensors.getTempCByIndex(0);  // Hőmérséklet kiolvasása Celsiusban


Serial.print("Temperature: "); Serial.print(currentTemperature);     Serial.println(" °C\n");


  }
}

void setState (char* name ,uint8_t pin,const JsonDocument& json) {
  bool R1_state = json[name];
    if (R1_state) {
        digitalWrite(pin, HIGH);
      Serial.printf("%s on\n", name);  
    } else {
        digitalWrite(pin, LOW);
      Serial.printf("%s off\n", name);  
    }
}

inline String getDataFromAPI() {
  HTTPClient http;
  JsonDocument json;
  //Set the url to be called.
  String serverName = "http://10.13.1.81/futes.php";
   String serverPath = serverName + "?temperature="+String(currentTemperature);
  http.begin(serverPath.c_str());
  //Send GET request.
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    String result = http.getString();
    deserializeJson(json, result);
    String textmessage = json["textmessage"];
    setState ("R1",pins[0],json);
    setState ("R2",pins[1],json);
    setState ("R3",pins[2],json);
    setState ("R4",pins[3],json);
  
    if (json["jelentes"]) {
        bep();
    }
    
    Serial.println(httpResponseCode);
    Serial.println(textmessage);
    //Close connection.
    http.end();
    conection_lost=0;
    return textmessage;
  } else {
    Serial.print("Response code: ");
    Serial.println(httpResponseCode);
    //Close connection.
    http.end();
     conection_lost++;
    if (conection_lost>10) {
        bep();
    }
    return "Http Code: " + httpResponseCode;
  }
}