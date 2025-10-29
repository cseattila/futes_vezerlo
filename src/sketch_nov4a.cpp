/*

Kincony KC868-A4

Relays example
*/



#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi beállítások
const char* ssid = "csengerilak";
const char* password = "fejbecsap6lak";

// MQTT szerver beállítások
const char* mqtt_server = "10.13.1.2";
const int mqtt_port = 1883;





// GPIO
#define KAZAN_PIN 2
#define SZELEP_ALSO_PIN 15
#define SZELEP_FELSO_PIN 5
#define SZELEP_KERINGETO_PIN 4
#define BUZZER_PIN 18

byte pins[] = {KAZAN_PIN, SZELEP_ALSO_PIN, SZELEP_FELSO_PIN, SZELEP_KERINGETO_PIN,BUZZER_PIN};
float currentTemperature=20;

int ido =0;

const int TONE_PWM_CHANNEL = 0;
#define ONE_WIRE_BUS 13
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);



// MQTT témák
const char* topic_also = "haz/homerseklet/also";
const char* topic_felso = "haz/homerseklet/felso";

const char* topic_cel_also =  "haz/kazan/celhomerseklet/also";
const char* topic_cel_felso = "haz/kazan/celhomerseklet/felso";

const char* homeasdistantConfigalso = "homeassistant/climate/felso/config";
const char* homeasdistantConfigfelso = "homeassistant/climate/felso/config";
const char* homeasdistantConfighomero = "homeassistant/sensor/vezerlohaz/config";
const char* topic_esemeny = "haz/kazan/esemeny";




// Állapot változók
bool kazan_be = false;
bool keringeto_nyitva = false;
bool szelep_also_nyitva = false;
bool szelep_felso_nyitva = false;

unsigned long szelep_also_nyitva_ido = 0;
unsigned long szelep_felso_nyitva_ido = 0;

// MQTT-n érkező értékek
float homerseklet_also = 20.0;
float homerseklet_felso = 20.0;
float cel_also = 21.0;
float cel_felso = 21.0;

// Hiszterézis érték (pl. +/-1 fok)
const float hiszterezis = 0.1;

// Szelep nyitási idő (ms)
const unsigned long szelep_nyitasi_ido = 15000;
// Replace with your network credentials

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  float ertek = message.toFloat();

   
  Serial.printf("qtt msg %s ertek %s\n", topic,message);
  
  if (String(topic).endsWith("/set")) {
     client.publish(String(topic).substring(0,strlen(topic)-4).c_str(), payload, true);
  }
  
  if (String(topic) == topic_also) homerseklet_also = ertek;
  else if (String(topic) == topic_felso) homerseklet_felso = ertek;

  else if (String(topic) == topic_cel_also) cel_also = ertek;
  else if (String(topic) == topic_cel_felso) cel_felso = ertek;
}

void initGPIO() {
  for (byte pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}


void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi csatlakozva");
}

 uint64_t chipid ;

void mqtt_register_entities() {
  // Home Assistant konfigurációk
  String config_also = String ("{")+
  "\"name\": \"Alsó szint\", "+
  "\"uniq_id\": \"termo_also\","+
  "\"current_temperature_topic\": \""+topic_cel_also+"\","+
  "\"temperature_command_topic\": "+topic_cel_also+"/set,"+
  "\"temperature_state_topic\": "+topic_also+","+
  "\"min_temp\": 16,"+
  "\"max_temp\": 28,"+
  "\"modes\": [\"heat\"]"+
  "}"
  ;

  String config_felso = String ("{")+
  "\"name\": \"Felso szint\", "+
  "\"uniq_id\": \"termo_also\","+
  "\"current_temperature_topic\": \""+topic_cel_felso+"\","+
  "\"temperature_command_topic\": "+topic_cel_felso+"/set,"+
  "\"temperature_state_topic\": "+topic_felso+","+
  "\"min_temp\": 16,"+
  "\"max_temp\": 28,"+
  "\"modes\": [\"heat\"]"+
  "}"
  ;

  String config_homero = String("{")+
  "\"name\": \"Vezérlo doboz hőmérséklet\","+
  "\"uniq_id\": \"vezdoboz_homer\","+
  "\"state_topic\": \"vezdoboz_homer\temp\","+
  "\"unit_of_measurement\": \"°C\","+
  "\"device_class\": \"temperature\""+
  "}";

  client.publish(homeasdistantConfigalso, config_also.c_str(), true);
  client.publish(homeasdistantConfigfelso, config_felso.c_str(), true);
  client.publish(homeasdistantConfighomero, config_homero.c_str(), true);
  Serial.printf("MQTT Home Assistant konfigurációk elküldve:\n%s\n%s\n%s\n",
                config_also.c_str(), config_felso.c_str(), config_homero.c_str());
}
void setup() {
  Serial.begin(9600);
  chipid = ESP.getEfuseMac();
  Serial.printf("Start csengeri futes verzerlo(%04X%08X)", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  initGPIO();
  sensors.begin();
  connectWiFi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
 
}


void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Kazán")) {
      client.subscribe(topic_also);
      client.subscribe(topic_felso);
      client.subscribe((String(topic_cel_also)+"/set").c_str());
      client.subscribe((String(topic_cel_felso)+"/set").c_str());
       mqtt_register_entities();
    } else {
       if (WiFi.status() != WL_CONNECTED) {
        Serial.print("WiFi nem csatlakozott, újra próbálkozás...");
        connectWiFi();
       }
        
      delay(2000);
    }
  }
}


// A szelepek impulzussal vezéreltek: HIGH = nyitás, LOW = zárás, 15 mp után kikapcsol magától
void nyit_szelep(int pin, bool& szelep_flag, unsigned long& nyitva_ido) {
  digitalWrite(pin, HIGH); // relé NO → NYITÁS
  szelep_flag = true;
  nyitva_ido = millis();
  
  client.publish(topic_esemeny, "Szelep %d NYITÁS (HIGH)");
  Serial.printf("Szelep %d NYITÁS (HIGH)\n", pin);
}

void zar_szelep(int pin, bool& szelep_flag) {
  digitalWrite(pin, LOW); // relé NC → ZÁRÁS
  szelep_flag = false;
  client.publish(topic_esemeny, "Szelep %d ZÁRÁS (LOW)");
  Serial.printf("Szelep %d ZÁRÁS (LOW)\n", pin);
}
String getDataFromAPI();


void handleSzelep(int pin, float homerseklet, float cel, bool& szelep_nyitva, unsigned long& szelep_nyitva_ido) {
  if (homerseklet < (cel - hiszterezis)) {
    if (!szelep_nyitva) nyit_szelep(pin, szelep_nyitva, szelep_nyitva_ido);
  } else if (homerseklet > (cel + hiszterezis)) {
    if (szelep_nyitva) zar_szelep(pin, szelep_nyitva);
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  handleSzelep(SZELEP_ALSO_PIN, homerseklet_also, cel_also, szelep_also_nyitva, szelep_also_nyitva_ido);
  handleSzelep(SZELEP_FELSO_PIN, homerseklet_felso, cel_felso, szelep_felso_nyitva, szelep_felso_nyitva_ido);

  

  unsigned long most = millis();
  // Ellenőrizzük, hogy a szelepek nyitva vannak-e és eltelt-e a nyitási idő
  bool felso_szelep_ok =      (szelep_felso_nyitva && (most - szelep_felso_nyitva_ido >= szelep_nyitasi_ido));
  bool szelep_ok = (szelep_also_nyitva && (most - szelep_also_nyitva_ido >= szelep_nyitasi_ido)) ||
                   felso_szelep_ok;

  if (!kazan_be && szelep_ok && (homerseklet_also < (cel_also - hiszterezis) || homerseklet_felso < (cel_felso - hiszterezis))) {
    digitalWrite(KAZAN_PIN, HIGH);
    kazan_be = true;
      client.publish(topic_esemeny, "Kazán bekapcsolva");
    Serial.println("Kazán bekapcsolva");
  } else if (kazan_be && (!szelep_ok || (homerseklet_also > (cel_also + hiszterezis) && homerseklet_felso > (cel_felso + hiszterezis)))) {
    digitalWrite(KAZAN_PIN, LOW);
    kazan_be = false;
      client.publish(topic_esemeny, "Kazán kikapcsolva");
    Serial.println("Kazán kikapcsolva");
  }

  if (!keringeto_nyitva  && felso_szelep_ok && ( homerseklet_felso < (cel_felso - hiszterezis))) {
    digitalWrite(SZELEP_KERINGETO_PIN, HIGH);
    keringeto_nyitva = true;
      client.publish(topic_esemeny, "Felo keringetőbe bekapcsolva");
    Serial.println("Felo keringetőbe bekapcsolva");
  } else if (keringeto_nyitva && (!felso_szelep_ok || ( homerseklet_felso > (cel_felso + hiszterezis)))) {
    digitalWrite(SZELEP_KERINGETO_PIN, LOW);
    keringeto_nyitva = false;
    client.publish(topic_esemeny, "Felo keringetőbe kikapcsolva");
    Serial.println("Felo keringetőbe kikapcsolva");
  }
  client.publish(topic_esemeny, "elek");
  delay(1000);
}