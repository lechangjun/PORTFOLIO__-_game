#include "WiFiS3.h"
#include <PubSubClient.h>
#include <NewPing.h>

// WiFi and MQTT 셋팅
#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

// sonar(TrigPin, EchoPin, MaxDistance);
// TrigPin과 EchoPin과 최대제한거리(MaxDistance)의 값을 선언
NewPing sonar[2] = { 
  NewPing(9, 8, 50), 
  NewPing(11, 10, 50), 
};

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = WLAN_SSID; // your network SSID (name)
char pass[] = WLAN_PASS; // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;

WiFiClient ethClient;
PubSubClient mqtt(ethClient);

int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
    Serial.begin(9600);
    
    // button
    pinMode(6,INPUT);
    pinMode(7,INPUT);

    delay(1000); // prevents usb driver crash on startup, do not omit this
    
    connectWiFi();
    delay(5000);
    // MQTT broker
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();
}

void loop() {
    // UltraSonic
    long sensor1val, sensor2val;
    sensor1val = sonar[0].ping_cm();
    sensor2val = sonar[1].ping_cm();
    
    char buffer1[15];
    char buffer2[15];
    char str[] = "cm";
    ltoa(sensor1val, buffer1, 10);
    strcat(buffer1, str);
    ltoa(sensor2val, buffer2, 10);
    strcat(buffer2, str);
    
    // button
    int push1=digitalRead(6);  
    int push2=digitalRead(7);

    Serial.println("============================");
    Serial.println(buffer1);
    Serial.println(buffer2);
    Serial.println(push1);
    Serial.println(push2);

    mqtt.publish("sensor/ultrasonic_1", String(buffer1).c_str());
    mqtt.publish("sensor/ultrasonic_2", String(buffer2).c_str());
    mqtt.publish("sensor/button_1", String(push1).c_str());
    mqtt.publish("sensor/button_2", String(push2).c_str());
    delay(4000);
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