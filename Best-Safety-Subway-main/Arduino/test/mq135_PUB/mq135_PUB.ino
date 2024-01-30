#include <MQUnifiedsensor.h>
#include "WiFiS3.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Arduino_FreeRTOS.h>

#define INCLUDE_vTaskDelayUntil = 1

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// WiFi and MQTT settings
#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

#define MQ135_PIN_1 A0
#define MQ135_PIN_2 A1
#define MQ135_PIN_3 A2

#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient ethClient;
PubSubClient mqtt(ethClient);

// MQ135 센서 선언 (placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type)
MQUnifiedsensor MQ135_1("Arduino UNO", 5, 10, A0, "MQ-135_1");
MQUnifiedsensor MQ135_2("Arduino UNO", 5, 10, A1, "MQ-135_2");
MQUnifiedsensor MQ135_3("Arduino UNO", 5, 10, A2, "MQ-135_3");

unsigned long previousMillis = 0;
const long interval = 500;

TaskHandle_t Handle_aTask;
TaskHandle_t Handle_bTask;
TaskHandle_t Handle_cTask;

static void taskMQ135_1(void* pvParameters) {
    // _PPM = a*ratio^b (PPM 농도와 상수 값을 계산하기 위한 수학 모델 설정)
    MQ135_1.setRegressionMethod(1);
    MQ135_1.init();
    MQ135_1.setRL(1);     // RL 값이 1K
    MQ135_1.setR0(3.12);  // 캘리브레이션하여 이 값을 구함 : MQ135.setR0(calcR0/10);

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 1;
    xLastWakeTime = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        // 데이터를 업데이트하면 arduino가 아날로그 핀의 전압을 읽음
        MQ135_1.update();
        // CO2 농도를 얻기 위해 ecuation 값 구성
        MQ135_1.setA(110.47);
        MQ135_1.setB(-2.862);
        // 설정된 모델과 a 및 b 값을 사용하여 PPM 농도를 읽음
        float CO2_1 = MQ135_1.readSensor();

        // lcd 출력
        lcd.setCursor(0, 0);
        lcd.print("Station: 1");

        lcd.setCursor(0, 1);
        lcd.print("Co2: ");
        lcd.print(CO2_1);
        mqtt.publish("sensor/mq135/_1", String(CO2_1).c_str());

        vTaskDelay(pdMS_TO_TICKS(3000));
        lcd.clear();
    }
}

static void taskMQ135_2(void* pvParameters) {
    MQ135_2.setRegressionMethod(1);
    MQ135_2.init();
    MQ135_2.setRL(1);
    MQ135_2.setR0(3.12);
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 4;
    xLastWakeTime = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        MQ135_2.update();
        MQ135_2.setA(110.47);
        MQ135_2.setB(-2.862);
        float CO2_2 = MQ135_2.readSensor();

        // lcd 출력
        lcd.setCursor(0, 0);
        lcd.print("Station: 2");

        lcd.setCursor(0, 1);
        lcd.print("Co2: ");
        lcd.print(CO2_2);
        mqtt.publish("sensor/mq135/_2", String(CO2_2).c_str());

        vTaskDelay(pdMS_TO_TICKS(5000));
        lcd.clear();
    }
}
static void taskMQ135_3(void* pvParameters) {
    MQ135_3.setRegressionMethod(1);
    MQ135_3.init();
    MQ135_3.setRL(1);
    MQ135_3.setR0(3.12);

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 7;
    xLastWakeTime = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        MQ135_3.update();
        MQ135_3.setA(110.47);
        MQ135_3.setB(-2.862);
        float CO2_3 = MQ135_3.readSensor();

        // lcd 출력
        lcd.setCursor(0, 0);
        lcd.print("Station: 3");

        lcd.setCursor(0, 1);
        lcd.print("Co2: ");
        lcd.print(CO2_3);       
        mqtt.publish("sensor/mq135/_3", String(CO2_3).c_str());

        vTaskDelay(pdMS_TO_TICKS(5000));
        lcd.clear();
    }
    vTaskDelete(NULL);
}
void setup() {

  Serial.begin(9600);
  
  // LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();

  delay(1000);
  // WiFi
  connectWiFi();
  delay(5000);

  // MQTT broker
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();

  
  xTaskCreate(taskMQ135_1,"taskMQ135_1",256, NULL, tskIDLE_PRIORITY + 3, &Handle_aTask);
  xTaskCreate(taskMQ135_2,"taskMQ135_2",256, NULL, tskIDLE_PRIORITY + 2, &Handle_bTask);
  xTaskCreate(taskMQ135_3, "taskMQ135_3", 256, NULL, tskIDLE_PRIORITY + 1, &Handle_cTask);

  vTaskStartScheduler();
}

void loop() 
{

}


void connectWiFi() 
{
  Serial.println("Connecting to WiFi");
  WiFi.begin(WLAN_SSID, WLAN_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void connectMQTT() 
{
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