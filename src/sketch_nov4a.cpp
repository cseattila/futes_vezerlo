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

byte pins[] = {KAZAN_PIN, SZELEP_ALSO_PIN, SZELEP_FELSO_PIN, SZELEP_KERINGETO_PIN};
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
const char* topic_cel_also = "haz/kazan/celhomerseklet/also";
const char* topic_cel_felso = "haz/kazan/celhomerseklet/felso";

const char* topic_cel_felso = "haz/kazan/esemeny";




// Állapot változók
bool kazan_be = false;
bool szelep_also_nyitva = false;
bool szelep_felso_nyitva = false;

unsigned long szelep_also_nyitva_ido = 0;
unsigned long szelep_felso_nyitva_ido = 0;

// MQTT-n érkező értékek
float homerseklet_also = 0.0;
float homerseklet_felso = 0.0;
float cel_also = 21.0;
float cel_felso = 21.0;

// Hiszterézis érték (pl. +/-1 fok)
const float hiszterezis = 1.0;

// Szelep nyitási idő (ms)
const unsigned long szelep_nyitasi_ido = 15000;
// Replace with your network credentials

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  float ertek = message.toFloat();

   
  Serial.printf("qtt msg %s ertek %s\n", topic,message);
  if (String(topic) == topic_also) homerseklet_also = ertek;
  else if (String(topic) == topic_felso) homerseklet_felso = ertek;
  else if (String(topic) == topic_cel_also) cel_also = ertek;
  else if (String(topic) == topic_cel_felso) cel_felso = ertek;
}

void setup() {
  Serial.begin(9600);
  
  Serial.println(F("Start csengeri futes verzerlo"));

  pinMode(pins[0], OUTPUT);

  pinMode(pins[1], OUTPUT);

  pinMode(pins[2], OUTPUT);

  pinMode(pins[3], OUTPUT);

   digitalWrite(KAZAN_PIN, LOW);
  digitalWrite(SZELEP_ALSO_PIN, HIGH);
  digitalWrite(SZELEP_FELSO_PIN, HIGH);
  digitalWrite(SZELEP_KERINGETO_PIN, LOW);

  pinMode(BUZZER_PIN, OUTPUT);
  sensors.begin();
 WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi csatlakozva");

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Kazán")) {
      client.subscribe(topic_also);
      client.subscribe(topic_felso);
      client.subscribe(topic_cel_also);
      client.subscribe(topic_cel_felso);
    } else {
      delay(2000);
    }
  }
}


// A szelepek impulzussal vezéreltek: HIGH = nyitás, LOW = zárás, 15 mp után kikapcsol magától
void nyit_szelep(int pin, bool& szelep_flag, unsigned long& nyitva_ido) {
  digitalWrite(pin, HIGH); // relé NO → NYITÁS
  szelep_flag = true;
  nyitva_ido = millis();
  Serial.printf("Szelep %d NYITÁS (HIGH)\n", pin);
}

void zar_szelep(int pin, bool& szelep_flag) {
  digitalWrite(pin, LOW); // relé NC → ZÁRÁS
  szelep_flag = false;
  Serial.printf("Szelep %d ZÁRÁS (LOW)\n", pin);
}
String getDataFromAPI();

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Alsó szelep vezérlése
  if (homerseklet_also < (cel_also - hiszterezis)) {
    if (!szelep_also_nyitva) nyit_szelep(SZELEP_ALSO_PIN, szelep_also_nyitva, szelep_also_nyitva_ido);
  } else if (homerseklet_also > (cel_also + hiszterezis)) {
    if (szelep_also_nyitva) zar_szelep(SZELEP_ALSO_PIN, szelep_also_nyitva);
  }

  // Felső szelep vezérlése
  if (homerseklet_felso < (cel_felso - hiszterezis)) {
    if (!szelep_felso_nyitva) nyit_szelep(SZELEP_FELSO_PIN, szelep_felso_nyitva, szelep_felso_nyitva_ido);
  } else if (homerseklet_felso > (cel_felso + hiszterezis)) {
    if (szelep_felso_nyitva) zar_szelep(SZELEP_FELSO_PIN, szelep_felso_nyitva);
  }

  unsigned long most = millis();
  bool szelep_ok = (szelep_also_nyitva && (most - szelep_also_nyitva_ido >= szelep_nyitasi_ido)) ||
                   (szelep_felso_nyitva && (most - szelep_felso_nyitva_ido >= szelep_nyitasi_ido));

  if (!kazan_be && szelep_ok && (homerseklet_also < (cel_also - hiszterezis) || homerseklet_felso < (cel_felso - hiszterezis))) {
    digitalWrite(KAZAN_PIN, HIGH);
    kazan_be = true;
    Serial.println("Kazán bekapcsolva");
  } else if (kazan_be && (!szelep_ok || (homerseklet_also > (cel_also + hiszterezis) && homerseklet_felso > (cel_felso + hiszterezis)))) {
    digitalWrite(KAZAN_PIN, LOW);
    kazan_be = false;
    Serial.println("Kazán kikapcsolva");
  }

  delay(1000);
}