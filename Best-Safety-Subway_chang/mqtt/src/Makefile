CC = gcc
DEBUG = -g
INCLUDE = -Iinclude

# 소스 파일들
SRCS = src/main.c

# 오브젝트 파일들
OBJS = $(SRCS:.c=.o)

# 목표 실행 파일
TARGET = mqtts

# 컴파일러 플래그
CFLAGS = $(DEBUG) $(INCLUDE)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# .c 파일을 .o 파일로 컴파일하는 암묵적 규칙
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
