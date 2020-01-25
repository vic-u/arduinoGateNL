#include <arduino.h>

#ifndef VIC_DEV
  #define VIC_DEV
  
  #define RC_NOTHING 0
  #define RC_UNKNOWN 1
  #define RC_DEVICEOFF 2
  #define RC_DEVICEON 3
  
  #define MAX_LONG 0xFFFFFFFF
  #define PHONENUM   "+79160265679"
  #define PHONENUM2 "+79850455595"

  #define DHTP	 4   // канал считывания данных с датчика внутри корпуса устройства
  #define VENT   7   // цифровой пин для включения вентилятора охлаждения внутри корпуса
  #define SSR_1  6	// будем использовать для управления термостатом
  #define SSR_2  5  //будем использовать для включения реле всего нагревателя
  #define SSR_3  6  // 4 5 6 цифровой пин для включения трех ступе обогрева
  #define SSR_W  10 // 9 цифровой пин для включения подогрева воды
  #define SSR_H  11 // 10 цифровой пин для включения холодильника
  #define IRR    12 // 12 цифровой пин для включения автополива
  #define MV     13 // цифровой пин для сигнала с инфракрасного датчика движения

  #define THERMROOMIN  2
  #define THERMROOMOUT 4
  #define THERMFLOWOUT 5
  #define THERMFLOWIN  6

  
  #define MAXROOMTEMP  22 // максимальная температура в комнате
  #define HIGHBOXTEMP  40 //максимальная темппература в корпусе, после которой включается вентилятор охлаждения
  #define DELTATEMP 2 //гестерезис температуры

  #ifndef _TRACE 
   #define _TRACE
  #endif  

class SSR
{
public:
  SSR(){};
  ~SSR(){};
  void Init(){};
  void On()
  {
    
  };
  void Off()
  {
    digitalWrite(SSR_1, LOW);
    delay(500);
    digitalWrite(SSR_2, LOW);
    delay(500);
    digitalWrite(SSR_3, LOW);
    delay(500);
    digitalWrite(SSR_W, LOW);
    delay(500);
    digitalWrite(SSR_H, LOW);
    delay(500);
    digitalWrite(IRR, LOW);
  };
  void Blink() {
	  int led = 13;
	  pinMode(led, OUTPUT);
	  digitalWrite(led, HIGH); // turn the LED on (HIGH is the voltage level)
	  delay(500);
	  digitalWrite(led, LOW); // turn the LED off by making the voltage LOW
	  digitalWrite(led, HIGH); // turn the LED on (HIGH is the voltage level)
	  delay(500);
	  digitalWrite(led, LOW); // turn the LED off by making the voltage LOW
  };

};
#endif
