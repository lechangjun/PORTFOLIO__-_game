-------------------플레이어 1 code ---------------------
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <mosquitto.h>

#define mysub_token "test2/t2"
#define mypub_token "test1/t1"

char buffer2[256];

typedef struct
{
    int fd;
} SerialPort;

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
    cfsetispeed(&options, B9600); // Set baud rate to 9600 (adjust as needed)
    cfsetospeed(&options, B9600);

    options.c_cflag &= ~PARENB; // No parity
    options.c_cflag &= ~CSTOPB; // One stop bit
    options.c_cflag &= ~CSIZE;  // Mask the character size bits
    options.c_cflag |= CS8;     // 8 data bits

    tcsetattr(port->fd, TCSANOW, &options);

    return 0;
}

void serial_close(SerialPort *port)
{
    close(port->fd);
    port->fd = -1;
}

int serial_read(SerialPort *port, char *buffer, size_t size)
{
    return read(port->fd, buffer, size);
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
//	printf("ID: %d\n", * (int *) obj);
	if(rc) {
		printf("Error with result code: %d\n", rc);
		exit(-1);
	}
	mosquitto_subscribe(mosq, NULL, mysub_token, 0);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) 
{   char* buffer2_address;
    buffer2_address = (char *) msg->payload;
    
    for (int i =0; i<15; i++)

    {
        buffer2[i] = *(buffer2_address+i);
        
    }
	//printf("%s", buffer2_address);
}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{    
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    char xmove;
    char ymove;
    char x_2move;
    char y_2move;
    InitWindow(screenWidth, screenHeight, "raylib [core] example - keyboard input");

    Vector2 ballPosition = {(float)screenWidth / 3, (float)screenHeight / 3}; // 초기 공이 나올 위치
    Vector2 ballPosition2 = {(float)screenWidth / 2, (float)screenHeight / 2};
    Color ballColor = DARKBLUE;
    Color ballColor2 = DARKBLUE;
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

   int rc, id=12;
  
	
   struct mosquitto *mosq; // 받는애
    mosquitto_lib_init();
	mosq = mosquitto_new("subscribe-test1", true, &id);
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_message_callback_set(mosq, on_message);
	rc = mosquitto_connect(mosq, "192.168.0.51", 1883, 10);
	if(rc) {
		printf("Could not connect to Broker with return code %d\n", rc);
		return -1;
	}

    struct mosquitto *mosq_pub; //보내는애
    int rc_pub;
    mosq_pub = mosquitto_new("publisher-test1", true, NULL);
    rc_pub = mosquitto_connect(mosq_pub, "192.168.0.51", 1883, 60);
    if(rc_pub != 0){
		printf("Client could not connect to broker! Error Code: %d\n", rc);
		mosquitto_destroy(mosq);
		return -1;
	}

	mosquitto_loop_start(mosq);
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
               //buffer[bytesRead] = '\0';
               mosquitto_publish(mosq_pub, NULL, mypub_token, 15, buffer, 0, false);


                xmove = buffer[9];
                ymove = buffer[12];

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


        }//




    }

    serial_close(&port);
    mosquitto_disconnect(mosq);
		mosquitto_destroy(mosq);

		mosquitto_lib_cleanup();
    
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}