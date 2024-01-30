#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <mosquitto.h>
#include <pthread.h>
#include <semaphore.h>

// MQTT 통신에 사용되는 토큰 정의
#define mysub_token "test1/t1"
#define mypub_token "test2/t2"

// 이동 데이터 큐의 최대 크기
#define QUEUE_SIZE 10

// 플레이어 각각의 움직임을 동기화 및 공유하기 위한 변수
char buffer2[256];
pthread_mutex_t ballMutex; // 공의 위치 동기화를 위한 뮤텍스
sem_t bufferSemaphore;     // 버퍼 세마포어

// 시리얼 통신을 위한 구조체
typedef struct
{
    int fd;
} SerialPort;

// MQTT 통신을 위한 구조체
struct MosquittoData
{
    struct mosquitto *mosq;
};

// 데이터 버퍼를 처리하기 위한 구조체
struct BufferData
{
    char buffer[256];
    pthread_mutex_t bufferMutex;
};

// 물체의 움직임을 따로 처리하기 위한 변수 선언
Vector2 ballPosition, ballPosition2; // 공의 움직임에 대한 전역 변수
Color ballColor, ballColor2, Save_ballcolor, Save2_ballcolor; // 공의 색상에 대한 전역 변수
char xmove, ymove, x_2move, y_2move; // 공의 움직임에 대한 변수

// 이동 데이터를 정의하는 구조체
typedef struct
{
    char xmove;
    char ymove;
} MoveData;

MoveData dataQueue[QUEUE_SIZE]; // 이동 데이터를 저장하는 큐
int front = -1;
int rear  = -1;

// 이동 데이터를 큐에 추가하는 함수
void Enqueue(MoveData move)
{
    pthread_mutex_lock(&ballMutex);

    if ((front == 0 && rear == QUEUE_SIZE - 1) || (rear == (front - 1) % (QUEUE_SIZE - 1)))
    {
        pthread_mutex_unlock(&ballMutex);
        return; // 큐가 꽉 차면 추가하지 않음
    }

    if (front == -1)
        front = rear = 0;
    else if (rear == QUEUE_SIZE - 1 && front != 0)
        rear = 0;
    else
        rear = (rear + 1) % QUEUE_SIZE;

    dataQueue[rear] = move;

    pthread_mutex_unlock(&ballMutex);
}

// 이동 데이터를 큐에서 꺼내는 함수
MoveData Dequeue()
{
    pthread_mutex_lock(&ballMutex);

    if (front == -1)
    {
        pthread_mutex_unlock(&ballMutex);
        MoveData emptyMove = {0, 0};
        return emptyMove; // 큐가 비어있으면 빈 데이터 반환
    }

    MoveData move = dataQueue[front];

    if (front == rear)
        front = rear = -1;
    else if (front == QUEUE_SIZE - 1)
        front = 0;
    else
        front = (front + 1) % QUEUE_SIZE;

    pthread_mutex_unlock(&ballMutex);

    return move;
}

// 시리얼 포트를 열고 초기화하는 함수
int serial_open(SerialPort *port, const char *device)
{
    port->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (port->fd == -1)
    {
        perror("Unable to open port");
        return -1;
    }

    struct termios options;
    tcgetattr(port->fd, &options);
    cfsetispeed(&options, B9600); // 9600보의 속도로 설정 (필요에 따라 조절)
    cfsetospeed(&options, B9600);

    options.c_cflag &= ~PARENB; // 패리티 없음
    options.c_cflag &= ~CSTOPB; // 스탑 비트 1개
    options.c_cflag &= ~CSIZE;  // 문자 크기 비트 마스킹
    options.c_cflag |= CS8;     // 8비트 데이터

    tcsetattr(port->fd, TCSANOW, &options);

    return 0;
}

// 시리얼 포트를 닫는 함수
void serial_close(SerialPort *port)
{
    close(port->fd);
    port->fd = -1;
}

// 시리얼 포트에서 데이터를 읽는 함수
int serial_read(SerialPort *port, char *buffer, size_t size)
{
    return read(port->fd, buffer, size);
}

// MQTT 통신 연결 시 호출되는 콜백 함수
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if (rc)
    {
        exit(-1); // 연결에 실패하면 프로그램 종료
    }
    mosquitto_subscribe(mosq, NULL, mysub_token, 0);
}

// MQTT 메시지 수신 시 호출되는 콜백 함수
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    char *buffer2_address = (char *)msg->payload;

    for (int i = 0; i < 15; i++)
    {
        buffer2[i] = *(buffer2_address + i);
    }
}

// MQTT 쓰레드
void *mosquittoThread(void *data)
{
    struct MosquittoData *mosqData = (struct MosquittoData *)data;
    struct mosquitto *mosq = mosqData->mosq;

    mosquitto_lib_init();
    mosquitto_loop_start(mosq);

    pthread_exit(NULL);
}

// 데이터 버퍼 처리 쓰레드
void *processBufferThread(void *data)
{
    struct BufferData *bufferData = (struct BufferData *)data;
    char *buffer = bufferData->buffer;

    pthread_exit(NULL);
}

// 공의 움직임을 처리하는 함수
void MoveBall(Vector2 *position, char xmove, char ymove)
{
    pthread_mutex_lock(&ballMutex);

    if (xmove == '8' || xmove == '9')
    {
        position->x += 5.0f;
    }
    else if (xmove == '1')
    {
        position->x -= 5.0f;
    }

    if (ymove == '8' || ymove == '9')
    {
        position->y -= 5.0f;
    }
    else if (ymove == '1')
    {
        position->y += 5.0f;
    }

    pthread_mutex_unlock(&ballMutex);
}

// 데이터를 처리하는 쓰레드
void *ProcessData(void *data)
{
    while (1)
    {
        sem_wait(&bufferSemaphore);
        MoveData move = Dequeue();
        sem_post(&bufferSemaphore);

        if (move.xmove != 0 || move.ymove != 0)
        {
            MoveBall(&ballPosition, move.xmove, move.ymove);
        }
    }

    return NULL;
}

// 데이터를 생성하는 쓰레드
void *producer(void *arg) 
{
    for (int i = 0; i < 10; ++i) 
    {
        MoveData move;
        move.xmove = i;
        move.ymove = i;

        Enqueue(move);
        usleep(100000); // 100ms 대기
    }

    pthread_exit(NULL);
}

// 데이터를 소비하는 쓰레드
void *consumer(void *arg) 
{
    for (int i = 0; i < 10; ++i) 
    {
        MoveData move = Dequeue();

        printf("Consumer: xmove=%d, ymove=%d\n", move.xmove, move.ymove);

        usleep(100000); // 100ms 대기
    }

    pthread_exit(NULL);
}


int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth  = 800;
    const int screenHeight = 450;

    char xmove;
    char ymove;

    char x_2move;
    char y_2move;
    InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");

    // 세마포어 및 뮤텍스 초기화
    pthread_mutex_init(&ballMutex, NULL);
    sem_init(&bufferSemaphore, 0, 1);

    Vector2 ballPosition  = {(float)screenWidth / 3, (float)screenHeight / 3}; // 초기 공이 나올 위치
    Vector2 ballPosition2 = {(float)screenWidth / 2, (float)screenHeight / 2};
    Color ballColor       = DARKBLUE;
    Color ballColor2      = DARKBLUE;
    Color Save_ballcolor;
    Color Save2_ballcolor;

    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    // 시리얼 통신 준비 ACM 0인지 1인지-----------------------------------------------------------------------------------
    SerialPort port;

    if (serial_open(&port, "/dev/ttyACM0") == -1)
    {
        return -1;
    }

    char buffer[256];
    int bytesRead;
    int bytestermiRead;

    int rc, id = 12;

    struct mosquitto *mosq; // 받는애
    mosquitto_lib_init();
    mosq = mosquitto_new("subscribe-test1", true, &id);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    rc = mosquitto_connect(mosq, "192.168.0.51", 1883, 10);
    if (rc)
    {
        printf("Could not connect to Broker with return code %d\n", rc);
        return -1;
    }

    struct mosquitto *mosq_pub; // 보내는애
    int rc_pub;
    mosq_pub = mosquitto_new("publisher-test1", true, NULL);
    rc_pub = mosquitto_connect(mosq_pub, "192.168.0.51", 1883, 60);
    if (rc_pub != 0)
    {
        printf("Client could not connect to broker! Error Code: %d\n", rc);
        mosquitto_destroy(mosq);
        return -1;
    }

    mosquitto_loop_start(mosq);

    // 쓰레드 생성
    pthread_t processDataThread;
    pthread_create(&processDataThread, NULL, ProcessData, NULL);

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {

        // Draw

        //----------------------------------------------------------------------------------

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCircleV(ballPosition, 40, ballColor);
        DrawCircleV(ballPosition2, 40, ballColor2); // 위에서 잡아준 위치에 볼을 40반지름 크기로 지정된색으로 만들기로 보임
        DrawText("move the ball with arrow keys", 10, 10, 20, DARKGRAY);
        EndDrawing();

        /// serial start
        // ------------------------------------------------------

        bytesRead = serial_read(&port, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {

            ////////////////////////////////////////move one ball ///////////////////////////
            if (buffer[bytesRead - 1] == '\n')

            {
                // buffer[bytesRead] = '\0';
                mosquitto_publish(mosq_pub, NULL, mypub_token, 15, buffer, 0, false);

                xmove = buffer[9];
                ymove = buffer[12];
                MoveBall(&ballPosition, xmove, ymove);

                if (xmove == '8' || xmove == '9')
                {
                    ballPosition.x += 5.0f;
                }
                else if (xmove == '1')
                {
                    ballPosition.x -= 5.0f;
                }

                if (ymove == '8' || ymove == '9')
                {
                    ballPosition.y -= 5.0f;
                }
                else if (ymove == '1')
                {
                    ballPosition.y += 5.0f;
                }

                /////////////////////////////////change color + 충돌 시 대비 /////////////////////

                if (buffer[7] == '0')
                {
                    ballColor = MAROON;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[6] == '0')
                {
                    ballColor = LIME;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[5] == '0')
                {
                    ballColor = DARKBLUE;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[4] == '0')
                {
                    ballColor = PURPLE;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[3] == '0')
                {
                    ballColor = YELLOW;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[2] == '0')
                {
                    ballColor = ORANGE;
                    Save_ballcolor = ballColor;
                }
                else if (buffer[1] == '0')
                {
                    ballColor = BEIGE;
                    Save_ballcolor = ballColor;
                }

                //////////////////////////////충돌 시 색깔 바꾸기 ////////////////////////////////////////////////

                if (CheckCollisionCircles(ballPosition, 40.0, ballPosition2, 40.0))
                {
                    ballColor = RED;
                }
                else
                    ballColor = Save_ballcolor;
            }
        }

        ////////////////////////////////////남의 공 move one ball ///////////////////////////
        if (buffer2[15 - 1] == '\n')
        {

            x_2move = buffer2[9];
            y_2move = buffer2[12];
            MoveBall(&ballPosition2, x_2move, y_2move);

            if (x_2move == '8' || x_2move == '9')
            {
                ballPosition2.x += 5.0f;
            }
            else if (x_2move == '1')
            {
                ballPosition2.x -= 5.0f;
            }

            if (y_2move == '8' || y_2move == '9')
            {
                ballPosition2.y -= 5.0f;
            }
            else if (y_2move == '1')
            {
                ballPosition2.y += 5.0f;
            }

            ////////////////////////////////남의 공 change color + 충돌 시 대비 /////////////////////

            if (buffer2[7] == '0')
            {
                ballColor2 = MAROON;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[6] == '0')
            {
                ballColor2 = LIME;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[5] == '0')
            {
                ballColor2 = DARKBLUE;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[4] == '0')
            {
                ballColor2 = PURPLE;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[3] == '0')
            {
                ballColor2 = YELLOW;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[2] == '0')
            {
                ballColor2 = ORANGE;
                Save2_ballcolor = ballColor2;
            }
            else if (buffer2[1] == '0')
            {
                ballColor2 = BEIGE;
                Save2_ballcolor = ballColor2;
            }

            //////////////////////////////충돌 시 색깔 바꾸기 ////////////////////////////////////////////////

            if (CheckCollisionCircles(ballPosition, 40.0, ballPosition2, 40.0))
            {
                ballColor2 = RED;
            }
            else
                ballColor2 = Save2_ballcolor;
            //   }

        } //
    }

    // 마무리
    pthread_join(processDataThread, NULL);
    sem_destroy(&bufferSemaphore);
    pthread_mutex_destroy(&ballMutex);

    serial_close(&port);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);

    mosquitto_lib_cleanup();

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}