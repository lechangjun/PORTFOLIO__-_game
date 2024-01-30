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
#define MQTT_CLIENT_ID "sqlClient"
#define TOPIC "sensor/#"
#define DATABASE_FILE "mqtt.db"

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

int on_message(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("     message: ");
    printf("%s", message->payload);

    // char *payloadptr = message->payload; 
    // payloadptr[message->payloadlen] = '\0';

    char insertSql[200];
    // message->payload : payloadptr
    snprintf(insertSql, sizeof(insertSql), "INSERT INTO sensors_data (topic, sensor_data) VALUES ('%s', '%s');", topicName, message->payload);

    char *zErrMsg = 0;

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
    
    // 메시지 페이로드에 할당된 추가 메모리를 포함하여 MQTT 메시지에 할당된 메모리를 해제
    MQTTClient_freeMessage(&message);
    //  MQTT C 클라이언트 라이브러리에서 할당한 메모리, 특히 topic 이름을 해제
    MQTTClient_free(topicName);
    return 1;
}

int main()
{
    int sql_rc, mqtt_rc;
    MQTTClient client;
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

    char *sql = "CREATE TABLE IF NOT EXISTS sensors_data("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "topic TEXT NOT NULL,"
                "sensor_data TEXT NOT NULL,"
                "created_at DATETIME DEFAULT (DATETIME('now', 'localtime')));";

    // SQL 명령을 실행
    // (open 한 DB, SQL 문장, 콜백함수 이름, 콜백함수 첫 번째 인자, ERROR 변수)
    sql_rc = sqlite3_exec(db, sql, callback, 0, 0);

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
    mqtt_rc = MQTTClient_create(&client, MQTT_HOST, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

    // TCP/IP 연결이 닫히지 않도록 작은 "활성 유지(keepalive)" 메시지를 20초마다 보내기
    conn_opts.keepAliveInterval = 10;
    // true로 설정되었기 때문에 이전 연결에서 남아 있는 작성 중이던 메시지가 완료되었는지 검사하지 않고 세션이 시작
    conn_opts.cleansession = 1;

    // 클라이언트를 서버에 연결하기 전에 콜백을 설정
    mqtt_rc = MQTTClient_setCallbacks(client, NULL, NULL, on_message, NULL);

    // 클라이언트 핸들 및 포인터를 연결 옵션에 인수로 전달
    if ((mqtt_rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect, return code %d\n", mqtt_rc);
        return 0;
    }

    // 선택한 토픽에 클라이언트 애플리케이션을 구독 (client, TOPIC, QOS);
    // QoS 설정은 이 구독자에게 송신된 메시지에 적용되는 최대 서비스 품질(QoS)을 판별
    // 서버는 이 설정의 낮은 값 및 원래 메시지의 QoS에서 메시지를 송신
    mqtt_rc = MQTTClient_subscribe(client, TOPIC, 0);

    // 연결 호출이 실패하는 경우 프로그램은 오류 코드 -1로 종료
    if (mqtt_rc != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to subscribe, return code %d\n", mqtt_rc);
        return 0;
    }

    printf("Subscribed to topic: %s\n", TOPIC);

    for (;;)
    {
        usleep(1000000); // Sleep for 1 second
    }

    // 클라이언트의 연결 끊기 (client, 제한시간)
    // 클라이언트는 서버에서 연결을 끊고 콜백 함수에서 작성 중인 메시지가 완료되기를 기다림
    // 제한시간을 밀리초 단위로 지정 (연결을 끊기 전에 수행해야 하는 다른 작업이 완료될 때까지 최대 10초 동안 기다림)
    MQTTClient_disconnect(client, 10000);
    // 클라이언트에 사용된 메모리를 비우고 프로그램을 종료
    MQTTClient_destroy(&client);

    // 데이터베이스 연결 닫기
    // sqlite3_close(db);
    return 0;
}