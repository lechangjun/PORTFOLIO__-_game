# 🚇 BSS (Best Safety Subway) 🚇
- 안전하고 깨끗한 지하철 탑승 및 운행 GABOZAGO ~

### 🛎️ 2조의 규칙
**1️⃣ commit message** 

```c
➕[Feat] : 기능 추가
🚀[Chore] : 실행 파일 및 수정 시 
🚧[Refactor] : 코드 수정 (리팩토링)
📃[Memo] : 주석 수정
🚨[Alert] : 기능 오류 시
```

**2️⃣ 깃허브 push 시 🚨 알리기 (충돌 발생!! Pull 하고 수정하세요!!)**

**3️⃣ 1일 1 issue 작성**

**4️⃣ dev branch 에 1차 Merge, main branch 에 최종 Merge**

# 🚇 프로젝트 🚇

**🚩 BSS (Best Safety Subway)**

- **비즈니스 지원 시스템 BSS (Business Support System) :** 통신 조직이 모든 고객 대면 활동을 관리하고 간소화하는 데 도움이 되는 다양한 소프트웨어 프로그램
- 안전하고 깨끗한 지하철 탑승 및 운행 ~ !

**🚩 추진 배경 및 필요성**

- 자살 및 사고 빈번하게 일어남
- 스크린 도어 고장 문제
- 출퇴근 압사 사고 위험 높음
    - 간접적으로 이산화탄소 과포화 시각화로 “위험” 의식 전달

**🚩 서비스 내용**

1. 지하철 역
  - 지하철 기관사에게 위급 상황 및 방해물 감지 알리기
      - MQTT 통신으로 알림
      - 몇 번 출입문인지 알림
      - 초록색 노란색 빨간색으로 꽉 차있으면 이산화탄소 과포화로, 사람들이 무서워서 안 탈 것임
  - 정차역에 초음파 라이더 센서로 방해물 감지
  - 정차역에 스크린 도어 열릴 시 위급 상황 알림

2. 지하철 내
    - 산소 포화 농도를 지하철 내부 역 판넬에 표시

**🚩 개발 시스템 환경**

**☑️ 개발 환경**

- Linux

**☑️ 소요 자원**

- Arduino UNO R4 WIFI
- MQ-135 아두이노 유해가스/공기질 센서 모듈 x 3
    - NH3, NOx, 알코올, 벤젠, 연기 및 CO2를 포함한 광범위한 가스를 감지
- 초음파 센서 x 2
    - 스크린 도어 사이에 하나씩 놓고 왼쪽 or 오른쪽 감지
- RGB LED
- LCD x 2
- BUTTON x 2
- 프로토타입
    - 스크린 도어 x 2
    - ( 각 스크린 도어 사이에 **초음파 센서** ) X 2
    - ( 각 스크린 고장 유무를 알려주는 임시 **버튼 센서** ) X 2
    - ( 각 지하철 칸마다 유해가스/공기질 센서 ) X 3
    - 각 스크린 도어의 **LCD 판넬** & **LED**

**☑️ 유스페이스** 

**☑️ 기술**

- **MQTT 통신**
    - 장애물 감지 & 스크린 도어 고장 → 기관사에 알림
- **FreeRTOS**
    - **⭐ RTOS 실시간(Real Time)**
        - 운영체제(Operating System)의 약자
        - 제한된 시간내에 원하는 작업을 모두 처리하는것을 보장하는 운영체제
        - 멀티태스킹 환경에서 Task 처리시간을 일관되게 유지하기 위한 용도로 사용
        - 시분할 시스템 하에서 우선순위 기반 스케줄링을 통해 우선순위가 높은 task가 먼저 작업을 처리할 수 있게 함
    - **🌳 태스크 (Task)**
        - 1️⃣ 각 task에는 우선순위를 할당하며 숫자가 높을수록 큰 우선순위를 의미한다. 우선순위는 0부터 configMAX_PRIORITIES까지 할당이 가능하다. 우선순위가 높은 task는 낮은 task를 선점(preemption)할 수 있고, 이때 context switching이 발생 동일한 우선순위 사이에서는 round robin을 사용
        - 2️⃣ Task는 return value가 없으며 (void*) 타입으로 여러 자료형을 매개변수로 받을 수 있다
        - 3️⃣ Task는 일회용 task와 주기적 task 2가지 종류

**🚩 사용자 인터페이스**
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2Fcf2rad%2FbtsCwhQs9Uz%2FQmtKogezVezssZPKNdfyB1%2Fimg.png)

**🚩 개발 일정**
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FW1lsm%2FbtsCtmyabfL%2FQnsuZoKGBpFjqX1IvDmhg1%2Fimg.png)


**🚩 관련 링크**

- [GitHub - Johannes4Linux/libmosquitto_examples: Some examples about how to use libmosquitto](https://github.com/Johannes4Linux/libmosquitto_examples)
- [GitHub - knolleary/pubsubclient: A client library for the Arduino Ethernet Shield that provides support for MQTT.](https://github.com/knolleary/pubsubclient/tree/master)
- [ESP8266과 MQTT 통신 사용해보기](https://blog.naver.com/roboholic84/221232207387)
- [MQ-135 아두이노 유해가스/공기질 센서 모듈 [SZH-SSBH-038]](https://www.devicemart.co.kr/goods/view?no=1327411)
- [FreeRTOS 사용하기](https://blog.naver.com/005334337/220725214276)
- [[FreeRTOS] 7. 태스크 관련 API 커널](https://velog.io/@psh4204/FreeRTOS-7.-%ED%83%9C%EC%8A%A4%ED%81%AC-%EA%B4%80%EB%A0%A8-API-%EC%BB%A4%EB%84%90)


**🚩 실행방법**
- Arduino/subway_inside/subway_inside.ino
- Arduino/subway_outside/subway_outside.ino
- DB/main.c
    - gcc -g -o main main.c -lpaho-mqtt3c -lsqlite3

**🚩 DEMO**
- YOUTUBE -> https://youtu.be/z0vN7ogobQA
- 🚉 Station
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FWEstZ%2FbtsCoIbKPuj%2FYNKrCbCrDdHIDXhOPki8C0%2Fimg.png)
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2F4wv8D%2FbtsCt4c1cJe%2FUJdUhDEduKjVeOPqcjt9gk%2Fimg.png)

- 🚋 Subway
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FbikoRz%2FbtsCtpPeLT9%2FsJdEmwUvsNrZRyn6HBeGvK%2Fimg.png)
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FblzNX6%2FbtsCtbpU01P%2FOxKSA60yN2RHz8UndlBqSk%2Fimg.png)

- 📔 DB
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FD4IXQ%2FbtsCoAEJgOT%2FBqNRvvGT4ozAjtLnlYnQmk%2Fimg.png)
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FmaNW9%2FbtsCtov3Bf1%2F4HYkdQckRn0ajU0ZkRmxz1%2Fimg.png)

- 🧑‍🎄 완성본
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FcAVvEh%2FbtsCrayAzrj%2FFQ6leY6DORuhrOYb8xQWS0%2Fimg.png)
![image](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fblog.kakaocdn.net%2Fdn%2FQVAJO%2FbtsCt5Xh8DY%2FekvxcX8e1jIvyr9dxcdlT0%2Fimg.png)


