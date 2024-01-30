#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sqlite3.h>
#include <time.h>
#include <unistd.h>
#include <MQTTClient.h>

#define MQTT_HOST "192.168.0.154"
#define MQTT_PORT 1883
#define DATABASE_FILE "mqtt.db"

#define TOPIC_ULTRA_1 "sensor/ultrasonic_1"
#define TOPIC_ULTRA_2 "sensor/ultrasonic_2"
#define TOPIC_BUTTON_1 "sensor/button_1"
#define TOPIC_BUTTON_2 "sensor/button_2"

#define CLIENT_COUNT 4

typedef struct {
    MQTTClient client;
    const char *clientID;
    const char *topic;
} MQTTInfo;

typedef struct SensorDataNode {
    struct SensorDataNode *next;
    SensorData data;
} SensorDataNode;

typedef struct {
    SensorDataNode *front;
    SensorDataNode *rear;
    pthread_mutex_t mutex;
} SensorDataQueue;

sqlite3 *db;

static void printError(const char *errorMsg) {
    fprintf(stderr, "%s\n", errorMsg);
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

static int executeSQL(const char *sql) {
    char *zErrMsg = NULL;
    int sql_rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

    if (sql_rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return 0;
    } else {
        putchar('\n');
        fprintf(stdout, "Record inserted successfully\n");
        return 1;
    }
}

static int insertSensorData(const SensorData *data) {
    char insertSql[200];
    snprintf(insertSql, sizeof(insertSql),
             "INSERT INTO sensors_data (ultrasonic_1, ultrasonic_2, button_1, button_2) "
             "VALUES ('%s', '%s', '%s', '%s');",
             data->ultrasonic1, data->ultrasonic2, data->button1, data->button2);

    return executeSQL(insertSql);
}

static void onMessage(void *context, const char *topicName, MQTTClient_message *message) {
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("     message: ");
    printf("%s", message->payload);

    SensorData *sensorData = malloc(sizeof(SensorData));
    sscanf(message->payload, "%s %s %s %s", sensorData->ultrasonic1, sensorData->ultrasonic2, sensorData->button1, sensorData->button2);

    SensorDataNode *newNode = malloc(sizeof(SensorDataNode));
    newNode->data = *sensorData;
    newNode->next = NULL;

    // 센서 데이터를 큐에 넣기
    SensorDataQueue *queue = (SensorDataQueue *)context;
    pthread_mutex_lock(&(queue->mutex));

    if (queue->rear == NULL) {
        queue->front = newNode;
        queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }

    pthread_mutex_unlock(&(queue->mutex));

    // MQTT 라이브러리에서 메모리 할당
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
}

static void *mqttThread(void *data) {
    MQTTInfo *mqttInfo = (MQTTInfo *)data;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    mqttInfo->client = MQTTClient_create(&(mqttInfo->client), MQTT_HOST, mqttInfo->clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.keepAliveInterval = 10;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(mqttInfo->client, NULL, NULL, onMessage, mqttInfo);

    if (MQTTClient_connect(mqttInfo->client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        printError("Failed to connect to MQTT broker");
        return NULL;
    }

    if (MQTTClient_subscribe(mqttInfo->client, mqttInfo->topic, 0) != MQTTCLIENT_SUCCESS) {
        printError("Failed to subscribe to MQTT topic");
        return NULL;
    }

    while (1) {
        usleep(1000000);  
    }

    return NULL;
}

static void processQueue(SensorDataQueue *queue) {
    pthread_mutex_lock(&(queue->mutex));

    while (queue->front != NULL) {
        SensorDataNode *temp = queue->front;
        queue->front = queue->front->next;

        // 데이터를 처리하고 데이터베이스에 삽입
        insertSensorData(&(temp->data));

        free(temp);
    }

    queue->rear = NULL;
    pthread_mutex_unlock(&(queue->mutex));
}

int main() {
    int sql_rc;
    MQTTInfo mqttClients[CLIENT_COUNT];

    sql_rc = sqlite3_open(DATABASE_FILE, &db);
    if (sql_rc) {
        printError("Can't open database");
        return 0;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    char *sql = "CREATE TABLE IF NOT EXISTS sensors_data("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "ultrasonic_1 TEXT NOT NULL,"
                "ultrasonic_2 TEXT NOT NULL,"
                "button_1 TEXT NOT NULL,"
                "button_2 TEXT NOT NULL,"
                "created_at DATETIME DEFAULT (DATETIME('now', 'localtime')));";

    if (!executeSQL(sql)) {
        return 0;
    }

    // 센서 데이터 큐 초기화
    SensorDataQueue sensorDataQueue;
    sensorDataQueue.front = NULL;
    sensorDataQueue.rear = NULL;
    pthread_mutex_init(&(sensorDataQueue.mutex), NULL);

    // MQTT
    const char *topics[CLIENT_COUNT] = {TOPIC_ULTRA_1, TOPIC_ULTRA_2, TOPIC_BUTTON_1, TOPIC_BUTTON_2};
    const char *clientIDs[CLIENT_COUNT] = {MQTT_CLIENT_ID1, MQTT_CLIENT_ID2, MQTT_CLIENT_ID3, MQTT_CLIENT_ID4};

    pthread_t threads[CLIENT_COUNT];

    for (int i = 0; i < CLIENT_COUNT; i++) {
        mqttClients[i].clientID = clientIDs[i];
        mqttClients[i].topic = topics[i];

        if (pthread_create(&threads[i], NULL, mqttThread, (void *)&mqttClients[i]) != 0) {
            printError("Failed to create MQTT thread");
            return 0;
        }
    }

    // 대기열을 처리하는 메인 루프
    while (1) {
        usleep(1000000);  // sleep for 1 second

        // 데이터 큐 처리
        processQueue(&sensorDataQueue);
    }

    for (int i = 0; i < CLIENT_COUNT; i++) {
        MQTTClient_disconnect(mqttClients[i].client, 10000);
        MQTTClient_destroy(&(mqttClients[i].client));
    }

    sqlite3_close(db);

    return 0;
}