#include "WiFiS3.h"
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // atof 함수가 선언된 헤더 파일
#include <NewPing.h>

// WiFi and MQTT 셋팅
#define WLAN_SSID "class924"
#define WLAN_PASS "kosta90009"
#define MQTT_SERVER "192.168.0.154"
#define MQTT_PORT 1883

// LCD 주소, 행, 렬 선언
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(0x27, 16, 2);

// sonar(TrigPin, EchoPin, MaxDistance);
// TrigPin과 EchoPin과 최대제한거리(MaxDistance)의 값을 선언
NewPing sonar[2] = {
    NewPing(9, 8, 50),
    NewPing(11, 10, 50),
};

// LED
int RED = 3;
int YELLOW = 4;
int GREEN = 5;

WiFiClient ethClient;
PubSubClient mqtt(ethClient);

void callback_SUB(char* topic, byte* payload, unsigned int length);
void Output(String message, unsigned int messageCounter, unsigned int num);

void setup()
{
    Serial.begin(9600);

    // LCD
    lcd.init();
    lcd.clear();
    lcd.backlight();

    // button
    pinMode(6, INPUT);
    pinMode(7, INPUT);

    // LED
    pinMode(RED, OUTPUT);
    pinMode(YELLOW, OUTPUT);
    pinMode(GREEN, OUTPUT);

    delay(1000);

    // WiFi
    connectWiFi();
    delay(5000);

    // MQTT broker
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();
    mqtt.setCallback(callback_SUB);
}

void loop()
{
    if (!mqtt.connected())
    {
        connectMQTT();
    }
    mqtt.loop();
    delay(1000);
}

void connectWiFi()
{
    Serial.println("Connecting to WiFi");
    WiFi.begin(WLAN_SSID, WLAN_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
}

void connectMQTT()
{
    Serial.println("Connecting to MQTT");

    while (!mqtt.connected())
    {
        if (mqtt.connect("MQ135_Publish"))
        {
            Serial.println("Connected to MQTT");
            mqtt.subscribe("sensor/mq135/#");
        }
        else
        {
            Serial.print("Failed to connect to MQTT, rc=");
            Serial.print(mqtt.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

int cnt = 0;
void callback_SUB(char *topic, byte *payload, unsigned int length)
{
    if (cnt == 3)
    {
      callback_PUB();
      cnt = 0;
    }
    else
    {
        // --------------------MQ135_SUB--------------------
        String message;

        for (int i = 0; i < length; i++)
        {
            message += (char)payload[i];
        }
        Serial.println(message);

        // Counter to keep track of received messages
        int messageCounter;
        float num1, num2, num3;
        if ((strcmp(topic, "sensor/mq135/_1") == 0) and (cnt == 0))
        {
            messageCounter = 1;
            num1 = message.toFloat();
            Output(message, messageCounter, num1);
            cnt += 1;
        }
        else if ((strcmp(topic, "sensor/mq135/_2") == 0) and (cnt == 1))
        {
            messageCounter = 2;
            num2 = message.toFloat();
            Output(message, messageCounter, num2);
            cnt += 1;
        }
        else if ((strcmp(topic, "sensor/mq135/_3") == 0) and (cnt == 2))
        {
            messageCounter = 3;
            num3 = message.toFloat();
            Output(message, messageCounter, num3);
            cnt += 1;
        } else {
          Serial.println("One more time");
        }
        Serial.println(cnt);
    }
}

void Output(String message, unsigned int messageCounter, float num) {
    float num1, num2, num3 = 0 ;

    if (messageCounter == 1) {
      num1 = num;
    } else if (messageCounter == 2) {
      num2 = num;
    } else if (messageCounter == 3) {
      num3 = num;
    }

    lcd.setCursor(0, 0);
    lcd.print("Station: ");
    lcd.print(messageCounter);

    lcd.setCursor(0, 1);
    lcd.print("Co2: ");
    lcd.print(message);
    Serial.println("-------");
    Serial.println(num1);
    Serial.println(num2);
    Serial.println(num3);
    Serial.println("-------");
    // led
    if ((num1 > 13) || (num2 > 15) || (num3 > 14))
    {
        digitalWrite(RED, HIGH);
        digitalWrite(YELLOW, LOW);
        digitalWrite(GREEN, LOW);
    }
    else if ((num1 > 8) or (num2 > 10) or (num3 > 9))
    {
        digitalWrite(RED, LOW);
        digitalWrite(YELLOW, HIGH);
        digitalWrite(GREEN, LOW);
    }
    else if ((num1 > 0) or (num2 > 0) or (num3 > 0))
    {
        digitalWrite(RED, LOW);
        digitalWrite(YELLOW, LOW);
        digitalWrite(GREEN, HIGH);
    }
    delay(2000);
    lcd.clear();
}

void callback_PUB()
{
    // --------------------Door_Button_PUB--------------------
    long sensor1val, sensor2val;
    sensor1val = sonar[0].ping_cm();
    sensor2val = sonar[1].ping_cm();

    // 물체에 반사되어돌아온 초음파의 시간을 변수에 저장합니다.
    Serial.print("A Ping : ");
    // sonar.ping_cm() : 센서 거리를 'cm'로 계산된 값을 출력
    Serial.print(sensor1val);
    Serial.println("cm");

    Serial.print("B Ping : ");
    Serial.print(sensor2val);
    Serial.println("cm");

    char buffer1[15];
    char buffer2[15];
    char str[] = "cm";

    ltoa(sensor1val, buffer1, 10);
    ltoa(sensor2val, buffer2, 10);
    strcat(buffer1, str);
    strcat(buffer2, str);
    delay(100);

    // button
    int push1 = digitalRead(6);
    Serial.print("push1 = ");
    Serial.println(push1);

    int push2 = digitalRead(7);
    Serial.print("push2 = ");
    Serial.println(push2);

    // Publish ultrasonic 센서 값 MQTT 토픽 설정
    mqtt.publish("sensor/ultrasonic_1", buffer1);
    mqtt.publish("sensor/ultrasonic_2", buffer2);
    delay(1000);
    // Publish button 센서 값 MQTT 토픽 설정
    mqtt.publish("sensor/button_1", String(push1).c_str());
    mqtt.publish("sensor/button_2", String(push2).c_str());
    delay(1000);
}