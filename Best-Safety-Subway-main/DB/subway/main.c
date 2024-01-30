// gcc -g -o main main.c -lpaho-mqtt3c -lsqlite3
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// C언어로 구현된 SQL 데이터 베이스 엔진
#include <sqlite3.h>
#include <time.h>
#include <unistd.h>

//  MQ Telemetry Transport 버전 3.1 프로토콜에 대한
// C 구현의 클라이언트 기능이 포함된 32비트 Windows 라이브러리
#include <MQTTClient.h>

#define MQTT_HOST "192.168.0.154"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID1 "sqlClient1"
#define MQTT_CLIENT_ID2 "sqlClient2"
#define MQTT_CLIENT_ID3 "sqlClient3"

#define TOPIC_mq135_1 "sensor/mq135/_1"
#define TOPIC_mq135_2 "sensor/mq135/_2"
#define TOPIC_mq135_3 "sensor/mq135/_3"

#define DATABASE_FILE "subway_mqtt.db"

// SQLite3 데이터베이스를 다루기 위한 객체
sqlite3 *db;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

char mq135_1[5];
char mq135_2[5];
char mq135_3[5];

int sql_message() {
    // printf("%s", ultrasonic_1);
    // printf("%s", ultrasonic_2);
    // printf("%s", button_1);
    // printf("%s", button_2);

    char insertSql[200];
    // message->payload : payloadptr
    snprintf(insertSql, sizeof(insertSql), "INSERT INTO subway_data (mq135_1, mq135_2, mq135_3) VALUES ('%s', '%s', '%s');", mq135_1, mq135_2, mq135_3);
    char *zErrMsg = 0;
    printf("%s", insertSql);

    pthread_mutex_lock(&mutex);
    // SQL 명령을 실행
    // (open 한 DB, SQL 문장, 콜백함수 이름, 콜백함수 첫 번째 인자, ERROR 변수)
    int sql_rc = sqlite3_exec(db, insertSql, callback, 0, &zErrMsg);
    pthread_mutex_unlock(&mutex);

    // SQLITE_OK : 코드는 작업이 성공했고 오류가 없었음
    if (sql_rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        putchar('\n');
        fprintf(stdout, "Record inserted successfully\n");
    }
    return 1;
}

int on_message_mq135_1(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("     message: ");
    printf("%s", message->payload);
    strcpy(mq135_1, message->payload);

    // 메시지 페이로드에 할당된 추가 메모리를 포함하여 MQTT 메시지에 할당된 메모리를 해제
    MQTTClient_freeMessage(&message);
    //  MQTT C 클라이언트 라이브러리에서 할당한 메모리, 특히 topic 이름을 해제
    MQTTClient_free(topicName);
    return 1;
}

int on_message_mq135_2(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("     message: ");
    printf("%s", message->payload);
    strcpy(mq135_2, message->payload);

    // 메시지 페이로드에 할당된 추가 메모리를 포함하여 MQTT 메시지에 할당된 메모리를 해제
    MQTTClient_freeMessage(&message);
    //  MQTT C 클라이언트 라이브러리에서 할당한 메모리, 특히 topic 이름을 해제
    MQTTClient_free(topicName);
    return 1;
}

int on_message_mq135_3(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("     message: ");
    printf("%s", message->payload);
    strcpy(mq135_3, message->payload);

    sql_message();

    // 메시지 페이로드에 할당된 추가 메모리를 포함하여 MQTT 메시지에 할당된 메모리를 해제
    MQTTClient_freeMessage(&message);
    //  MQTT C 클라이언트 라이브러리에서 할당한 메모리, 특히 topic 이름을 해제
    MQTTClient_free(topicName);
    return 1;
}


int main()
{
    int sql_rc, mqtt_mq135_1, mqtt_mq135_2, mqtt_mq135_3 ;
    MQTTClient client1, client2, client3;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    // ------------------------------------------ SQLite ------------------------------------------
    // DB 파일을 연결하고 데이터베이스 객체에 대한 포인터를 반환
    sql_rc = sqlite3_open(DATABASE_FILE, &db);

    // 연결 실패시 오류 메시지 출력
    if (sql_rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    else
    {
        fprintf(stdout, "Opened database successfully\n");
    }

    char *sql = "CREATE TABLE IF NOT EXISTS subway_data("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "mq135_1 TEXT NOT NULL,"
                "mq135_2 TEXT NOT NULL,"
                "mq135_3 TEXT NOT NULL,"
                "created_at DATETIME DEFAULT (DATETIME('now', 'localtime')));";

    pthread_mutex_lock(&mutex);
    // SQL 명령을 실행
    // (open 한 DB, SQL 문장, 콜백함수 이름, 콜백함수 첫 번째 인자, ERROR 변수)
    sql_rc = sqlite3_exec(db, sql, callback, 0, 0);
    pthread_mutex_unlock(&mutex);


    // SQLITE_OK : 코드는 작업이 성공했고 오류가 없었음
    if (sql_rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }

    // ------------------------------------------ MQTT ------------------------------------------
    // (새로 작성한 클라이언트의 핸들을 가리키는 포인터, 수신되는 클라이언트 연결 요청을 모니터하는 MQTT 포트의 URI,
    // 클라이언트를 식별하는 데 사용되는 이름, 클라이언트 상태가 메모리에서 보류 중으로 시스템 장애가 발생하는 경우 손실)
    mqtt_mq135_1 = MQTTClient_create(&client1, MQTT_HOST, MQTT_CLIENT_ID1, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    mqtt_mq135_2 = MQTTClient_create(&client2, MQTT_HOST, MQTT_CLIENT_ID2, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    mqtt_mq135_3 = MQTTClient_create(&client3, MQTT_HOST, MQTT_CLIENT_ID3, MQTTCLIENT_PERSISTENCE_NONE, NULL);


    // TCP/IP 연결이 닫히지 않도록 작은 "활성 유지(keepalive)" 메시지를 20초마다 보내기
    conn_opts.keepAliveInterval = 10;
    // true로 설정되었기 때문에 이전 연결에서 남아 있는 작성 중이던 메시지가 완료되었는지 검사하지 않고 세션이 시작
    conn_opts.cleansession = 1;

    // 클라이언트를 서버에 연결하기 전에 콜백을 설정
    mqtt_mq135_1 = MQTTClient_setCallbacks(client1, NULL, NULL, on_message_mq135_1, NULL);
    mqtt_mq135_2 = MQTTClient_setCallbacks(client2, NULL, NULL, on_message_mq135_2, NULL);
    mqtt_mq135_3 = MQTTClient_setCallbacks(client3, NULL, NULL, on_message_mq135_3, NULL);
    
    // 클라이언트 핸들 및 포인터를 연결 옵션에 인수로 전달
    if (((mqtt_mq135_1 = MQTTClient_connect(client1, &conn_opts)) != MQTTCLIENT_SUCCESS) ||
    ((mqtt_mq135_2 = MQTTClient_connect(client2, &conn_opts)) != MQTTCLIENT_SUCCESS) ||
    ((mqtt_mq135_3 = MQTTClient_connect(client3, &conn_opts)) != MQTTCLIENT_SUCCESS))

    {
        fprintf(stderr, "Failed to connect, return code \n");
        return 0;
    }

    // 선택한 토픽에 클라이언트 애플리케이션을 구독 (client, TOPIC, QOS);
    // QoS 설정은 이 구독자에게 송신된 메시지에 적용되는 최대 서비스 품질(QoS)을 판별
    // 서버는 이 설정의 낮은 값 및 원래 메시지의 QoS에서 메시지를 송신
    mqtt_mq135_1 = MQTTClient_subscribe(client1, TOPIC_mq135_1, 0);
    mqtt_mq135_2 = MQTTClient_subscribe(client2, TOPIC_mq135_2, 0);
    mqtt_mq135_3 = MQTTClient_subscribe(client3, TOPIC_mq135_3, 0);

    // 연결 호출이 실패하는 경우 프로그램은 오류 코드 -1로 종료
    // 연결 호출이 실패하는 경우 프로그램은 오류 코드 -1로 종료
    if ((mqtt_mq135_1 != MQTTCLIENT_SUCCESS) || (mqtt_mq135_2 != MQTTCLIENT_SUCCESS) ||
        (mqtt_mq135_3 != MQTTCLIENT_SUCCESS))
    {
        fprintf(stderr, "Failed to subscribe, return code \n");
        return 0;
    }

    printf("Subscribed to topic: %s\n", TOPIC_mq135_1);
    printf("Subscribed to topic: %s\n", TOPIC_mq135_2);
    printf("Subscribed to topic: %s\n", TOPIC_mq135_3);

    for (;;)
    {
        usleep(1000000); // Sleep for 1 second
    }

    // 클라이언트의 연결 끊기 (client, 제한시간)
    // 클라이언트는 서버에서 연결을 끊고 콜백 함수에서 작성 중인 메시지가 완료되기를 기다림
    // 제한시간을 밀리초 단위로 지정 (연결을 끊기 전에 수행해야 하는 다른 작업이 완료될 때까지 최대 10초 동안 기다림)
    MQTTClient_disconnect(client1, 10000);
    MQTTClient_disconnect(client2, 10000);
    MQTTClient_disconnect(client3, 10000);

    // 클라이언트에 사용된 메모리를 비우고 프로그램을 종료
    MQTTClient_destroy(&client1);
    MQTTClient_destroy(&client2);
    MQTTClient_destroy(&client3);

    // 데이터베이스 연결 닫기
    sqlite3_close(db);
    return 0;
}