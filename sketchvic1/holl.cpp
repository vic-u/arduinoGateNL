#include "holl.h"


void Refrigerator::setCommand(int command) 
{
  refr_command = command;
}
void Refrigerator::checkRefrigerator()
{
	if (refr_started)
	{
		digitalWrite(SSR_H, HIGH); // включаем реле
	}
	else
	{
		digitalWrite(SSR_H, LOW); // выключаем реле
	}
  if(refr_command == RC_DEVICEON) 
  {
    digitalWrite(SSR_H, HIGH); // включаем реле
    refr_started = true; // устанавливаем флаг состояния
    refr_command = RC_NOTHING; // сбрасываем команду, как обработанную
    #ifdef _TRACE 
      Serial.println(F("refrigerator start")); 
    #endif
    return;
  }
  if(refr_command == RC_DEVICEOFF)
  {
    digitalWrite(SSR_H, LOW); // выключаем реле
    refr_started = false; // сбрасываем флаг состояния
    refr_command = RC_NOTHING; // сбрасываем команду, как обработанную
    #ifdef _TRACE 
      Serial.println(F("refrigerator stop")); 
    #endif
    return;    
  }
  delay(500); // пол секунды ждем для отработки реле
}
boolean Refrigerator::getStarted()
{
  return refr_started;
}
void Refrigerator::Init()
{
  pinMode(SSR_H, OUTPUT);
  delay(500);
  digitalWrite(SSR_H, LOW);
  #ifdef _TRACE
  Serial.println(F("refrigerator init")); 
  #endif
}

