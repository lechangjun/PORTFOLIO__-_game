#include "main.h"

// handle mqtt 통신 연결 (역할)
void on_connect(struct mosquitto *mosq, void *obj, int rc) 
{
    printf("ID: %d\n", *(int *) obj);
    if (rc) 
    {
        printf("Error with result code: %d\n", rc);
        exit(-1);
    }
    mosquitto_subscribe(mosq, NULL, "test/t1", 0);
}

// handle mqtt 통신 메세지를 게임 오브젝트에 전달하는 (역할)
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) 
{
    char* buffer2_address = buffer2;
    buffer2_address = (char *) msg->payload;

    for (int i = 0; i < 15; i++) 
    {
        buffer2[i] = *(buffer2_address+i);
    }
    printf("%s", buffer2_address);
}