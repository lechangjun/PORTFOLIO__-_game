#include "WiFiS3.h"
#include <PubSubClient.h>
#include <Arduino_FreeRTOS.h>
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

TaskHandle_t Handle_aTask;
TaskHandle_t Handle_bTask;
TaskHandle_t Handle_cTask;

static void taskUltrasonic_1(void* pvParameters) {
    while (1) {
        long sensor1val;
        sensor1val = sonar[0].ping_cm();
        
        char buffer1[15];
        char str[] = "cm";
        ltoa(sensor1val, buffer1, 10);
        strcat(buffer1, str);
        Serial.println("Thread A: taskUltrasonic_1");
        Serial.println(buffer1); 

        mqtt.publish("sensor/ultrasonic_1", String(buffer1).c_str());

        vTaskDelay( 500 / portTICK_PERIOD_MS ); 
        delay(2000);
    }
}

static void taskUltrasonic_2(void* pvParameters) {
    while (1) {
        long sensor2val;
        sensor2val = sonar[1].ping_cm();
        
        char buffer2[15];
        char str[] = "cm";
        ltoa(sensor2val, buffer2, 10);
        strcat(buffer2, str);
        Serial.println("Thread B: taskUltrasonic_2");
        Serial.println(buffer2); 

        mqtt.publish("sensor/ultrasonic_2", String(buffer2).c_str());

        vTaskDelay( 500 / portTICK_PERIOD_MS ); 
        delay(2000);
    }
}   

static void taskButton(void* pvParameters) {
    // button
    pinMode(6,INPUT);
    pinMode(7,INPUT);
  
    while (1) {
        // button
        int push1=digitalRead(6);  
        int push2=digitalRead(7);

        Serial.println("Thread C: taskButton");
        Serial.print("push1 = ");
        Serial.println(push1);
        Serial.print("push2 = ");
        Serial.println(push2);

        mqtt.publish("sensor/button_1", String(push1).c_str());
        mqtt.publish("sensor/button_2", String(push2).c_str());

        vTaskDelay( 500 / portTICK_PERIOD_MS ); 
        delay(2000);
    }
    vTaskDelete(NULL);
}

void setup() {
    Serial.begin(9600);

    delay(1000); // prevents usb driver crash on startup, do not omit this
    while(!Serial);  // Wait for Serial terminal to open port before starting program
    
    connectWiFi();
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE)
    {
      Serial.println("Communication with WiFi module failed!");
      // don't continue
      while (true)
        ;
    }

    // WiFi.firmwareVersion() : 모듈에서 실행 중인 펌웨어 버전을 문자열로 반환
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
      Serial.println("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED)
    {
      Serial.print("Attempting to connect to Network named: ");
      Serial.println(ssid); // print the network name (SSID);

      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);
      // wait 10 seconds for connection:
      delay(5000);
    }
    printWifiStatus();
    delay(5000);

    // MQTT broker
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();

    Serial.println("");
    Serial.println("******************************");
    Serial.println("        Program start         ");
    Serial.println("******************************");

    // Create the threads that will be managed by the rtos
    // Sets the stack size and priority of each task
    // Also initializes a handler pointer to each task, which are important to communicate with and retrieve info from tasks
    xTaskCreate(taskUltrasonic_1,"taskUltrasonic_1",256, NULL, tskIDLE_PRIORITY +3, &Handle_aTask);
    xTaskCreate(taskUltrasonic_2,"taskUltrasonic_2",256, NULL, tskIDLE_PRIORITY + 2, &Handle_bTask);
    xTaskCreate(taskButton,"taskButton",256, NULL, tskIDLE_PRIORITY + 1, &Handle_cTask);

    // Start the RTOS, this function will never return and will schedule the tasks.
    vTaskStartScheduler();
}

void loop() {
    // NOTHING
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