#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

// Configurações WiFi e MQTT
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);

// Pinos
const int yfPin = 34; // YF-S201 (simulado por potenciômetro)
const int trigPin = 26; // HC-SR04
const int echoPin = 27;
const int valvePin = 25; // Válvula solenoide (relé)

unsigned long startTime;
float flowTimes[4], levelTimes[4], valveTimes[4];
int measurementCount = 0;

void setup() {
  Serial.begin(115200);
  pinMode(yfPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(valvePin, OUTPUT);

  // Conectar WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado");

  // Conectar MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  startTime = millis();
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (String(topic) == "water/valve") {
    digitalWrite(valvePin, message == "ON" ? HIGH : LOW);
    if (measurementCount < 4) {
      valveTimes[measurementCount] = millis() - startTime;
      Serial.print("Tempo da válvula (ms): ");
      Serial.println(valveTimes[measurementCount]);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32Client")) {
      client.subscribe("water/valve");
      Serial.println("MQTT conectado");
    } else {
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulação YF-S201
  startTime = millis();
  int flow = analogRead(yfPin) % 100; // Valor simulado
  client.publish("water/flow", String(flow).c_str());
  if (measurementCount < 4) {
    flowTimes[measurementCount] = millis() - startTime;
    Serial.print("Tempo do YF-S201 (ms): ");
    Serial.println(flowTimes[measurementCount]);
  }

  // Simulação HC-SR04
  startTime = millis();
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;
  client.publish("water/level", String(distance).c_str());
  if (measurementCount < 4) {
    levelTimes[measurementCount] = millis() - startTime;
    Serial.print("Tempo do HC-SR04 (ms): ");
    Serial.println(levelTimes[measurementCount]);
  }

  // Exibir no Serial Monitor (em vez do LCD)
  Serial.print("Fluxo: ");
  Serial.print(flow);
  Serial.println(" L/min");
  Serial.print("Nivel: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Incrementar contagem após 4 medições
  if (millis() % 10000 < 50 && measurementCount < 4) {
    measurementCount++;
  }

  delay(2000);
}
