#include <ESP8266WiFi.h>
#include <fauxmoESP.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include "credentials.h"

#define LED_PIN 14

WiFiClient client;
fauxmoESP fauxmo;

char status[10];

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish pubTest = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/f/test");
Adafruit_MQTT_Subscribe subTest = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/f/test");

void testCallback(char *data, uint16_t len) {
  Serial.print("Callback entry! Value:");
  Serial.println(data);

  String message = String(data);
  message.trim();

  if (message == "ligado") {
    digitalWrite(LED_PIN, HIGH);
  } else if (message == "desligado") {
    digitalWrite(LED_PIN, LOW);
  }
}

void wifiSetup() {

  Serial.printf("[WiFi] Conectando com %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.println("[WiFi] Modo: STATION");
  Serial.printf("[WiFi] SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("[WiFi] Endereco IP: %s\n", WiFi.localIP().toString().c_str());
}

void mqttSetup() {
  subTest.setCallback(testCallback);
  mqtt.subscribe(&subTest);
}

void setup() {
  Serial.begin(115200);
  Serial.println(); Serial.println();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  wifiSetup();
  mqttSetup();

  fauxmo.addDevice("luz do computador");
  
  fauxmo.setPort(80);
  fauxmo.enable(true);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.printf("[Alexa] Valores recebidos para o dispositivo #%d (%s):\n[Alexa] Estado: %s\n[Alexa] Valor: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

    if (strcmp(device_name, "luz do computador") == 0) {
      String val = state ? "ligado" : "desligado";
      val.toCharArray(status, 10);
      
      if (!state) {
        digitalWrite(LED_PIN, LOW);
      } else {
        digitalWrite(LED_PIN, HIGH);
      }

      Serial.print(F("\n[MQTT] Atualizando led como "));
      Serial.print(val);
      Serial.print("...");

      if (!pubTest.publish(status)) {
        Serial.println(F("Falhou"));
      } else {
        Serial.println(F("Ok"));
      }
    }
  });
}

void loop() {
  fauxmo.handle();
  MQTT_connect();

  mqtt.processPackets(500);
}

void MQTT_connect() {
  int8_t ret;

  if (mqtt.connected()) {
    return;
  }

  Serial.print("[MQTT] Conectando... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("[MQTT] Tentando reconectar em 10 segundos...");
       mqtt.disconnect();
       delay(10000);
       retries--;
       if (retries == 0) {
         while (1);
       }
  }
  
  Serial.println("MQTT Connected!");
}
