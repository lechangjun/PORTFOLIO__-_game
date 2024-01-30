#include <MQUnifiedsensor.h>
#include "WiFiS3.h"
#include <Ethernet.h>
#include <PubSubClient.h>

// WiFi and MQTT settings
#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

#define MQ135_PIN A0

char ssid[] = WLAN_SSID;
char pass[] = WLAN_PASS;
int keyIndex = 0;

WiFiClient ethClient;
PubSubClient mqtt(ethClient);

int status = WL_IDLE_STATUS;
WiFiServer server(80);

MQUnifiedsensor MQ135("Arduino UNO", 5, 10, A0, "MQ-135");
unsigned long previousMillis = 0;
const long interval = 500;

void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(MQ135_PIN, INPUT);
  connectWiFi();

  MQ135.setRegressionMethod(1);
  MQ135.init();
  MQ135.setRL(1);
  MQ135.setR0(3.12);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  printWifiStatus();
  delay(5000);

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
}

void loop() {
  unsigned long currentMillis = millis();
    delay(4000);

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    MQ135.update();
    MQ135.setA(110.47);
    MQ135.setB(-2.862);
    float CO2 = MQ135.readSensor();
    
    Serial.print("CO2= ");
    Serial.print(CO2);
    Serial.println(" ppm");

    mqtt.publish("sensor/mq135", String(CO2).c_str());

  }
}

void connectWiFi() {
  Serial.println("Connecting to WiFi");
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void connectMQTT() {
  Serial.println("Connecting to MQTT");
  while (!mqtt.connected()) {
    if (mqtt.connect("MQ135Client")) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed to connect to MQTT, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}