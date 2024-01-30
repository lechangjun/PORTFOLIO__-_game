#include <MQUnifiedsensor.h>
#include "WiFiS3.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Arduino_FreeRTOS.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>


#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

#define SENSOR_COUNT 3

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
WiFiClient ethClient;
PubSubClient mqtt(ethClient);

struct SensorConfig {
  MQUnifiedsensor sensor;
  const char* label;
  int pin;
  int interval;
  float A;
  float B;
};

SensorConfig sensors[SENSOR_COUNT] = 
{
  { MQUnifiedsensor("Arduino UNO", 5, 10, A0, "MQ-135_1"), "Station: 1", A0, 2500, 110.47, -2.862 },
  { MQUnifiedsensor("Arduino UNO", 5, 10, A1, "MQ-135_2"), "Station: 2", A1, 6500, 110.47, -2.862 },
  { MQUnifiedsensor("Arduino UNO", 5, 10, A2, "MQ-135_3"), "Station: 3", A2, 8500, 110.47, -2.862 }
};

void sensorTask(void* pvParameters) {
  SensorConfig* config = (SensorConfig*)pvParameters;
  
  config->sensor.setRegressionMethod(1);
  config->sensor.init();
  config->sensor.setRL(1);
  config->sensor.setR0(3.12);

  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(config->interval));
    
    config->sensor.update();
    config->sensor.setA(config->A);
    config->sensor.setB(config->B);
    float CO2 = config->sensor.readSensor();

    lcd.setCursor(0, 0);
    lcd.print(config->label);

    lcd.setCursor(0, 1);
    lcd.print("Co2: ");
    lcd.print(CO2);
    
    String topic = "sensor/mq135/" + String(config->label);
    mqtt.publish(topic.c_str(), String(CO2).c_str());
    
    lcd.clear();
  }
}

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.clear();
  lcd.backlight();

  connectWiFi();
  delay(5000);

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();

  for (int i = 0; i < SENSOR_COUNT; i++) {
    xTaskCreate(sensorTask, sensors[i].label, 256, &sensors[i], tskIDLE_PRIORITY + 1, NULL);
  }

  vTaskStartScheduler();
}

void loop() {}

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