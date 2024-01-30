#include "WiFiS3.h"
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient ethClient;
PubSubClient mqtt(ethClient);
void callback(char* topic, byte* payload, unsigned int length);

int messageCounter = 0; // Counter to keep track of received messages

void setup() {
  Serial.begin(9600);

  // LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();

  delay(1000); // prevents usb driver crash on startup, do not omit this
  
  connectWiFi();
  delay(5000);
  // MQTT broker
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();
  
  mqtt.setCallback(callback);
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();
  delay(1000);
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
    if (mqtt.connect("LCDClient")) {
      Serial.println("Connected to MQTT");
      mqtt.subscribe("sensor/mq135/#");
    } else {
      Serial.print("Failed to connect to MQTT, rc=");
      Serial.print(mqtt.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  int messageCounter;
  if (strcmp(topic, "sensor/mq135/_1") == 0) {
    messageCounter = 1; 
  } else if (strcmp(topic, "sensor/mq135/_2") == 0) {
    messageCounter = 2; 
  } else if (strcmp(topic, "sensor/mq135/_3") == 0) {
    messageCounter = 3; 
  }
  lcd.setCursor(0, 0);
  lcd.print("Station: ");
  lcd.print(messageCounter);

  lcd.setCursor(0, 1);
  lcd.print("Co2: ");
  lcd.print(message);

  delay(2000);
  lcd.clear();
}