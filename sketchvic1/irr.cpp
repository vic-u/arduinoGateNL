#include "irr.h"


void Irrigation::setCommand(int command) 
{
  irr_command = command;
}
//команда обработки автополива по смс сообщению включение и выключение
void Irrigation::checkIrrigation2()
{
	if (irr_started)
	{
		digitalWrite(IRR, HIGH); // включаем реле
	}
	else
	{
		digitalWrite(IRR, LOW); // выключаем реле
	}
	if (irr_command == RC_DEVICEON)
	{
		digitalWrite(IRR, HIGH); // включаем реле
		irr_started = true; // устанавливаем флаг состояния
		irr_command = RC_NOTHING; // сбрасываем команду, как обработанную
    #ifdef _TRACE 
		  Serial.println(F("irrigator start"));
    #endif
		return;
	}
	if (irr_command == RC_DEVICEOFF)
	{
		digitalWrite(IRR, LOW); // выключаем реле
		irr_started = false; // сбрасываем флаг состояния
		irr_command = RC_NOTHING; // сбрасываем команду, как обработанную
    #ifdef _TRACE 
		  Serial.println(F("irrigator stop"));
    #endif
		return;
	}
	delay(500); // пол секунды ждем для отработки реле
}
//void Irrigation::checkIrrigation(VGSM &vgsm) // включение и отключение автополива
//{
//  unsigned long time = millis(); // выдает количество тиков внутреннего счетчика ардуино
//  if (!irr_started) //если полив не был включен. после включения платы или после очередного отключения по таймеру
//  {
//    if(time > (irr_prev_ms_off + IRR_TIMER_OFF_INT)) // автополив не включен и прошли сутки или счетчик переполнился и пошел с нуля и прошли сутки  
//    { 
//      irr_prev_ms_off = time; // сохраняем текущее значение таймера ардуино и начинаем его проверять
//      if (IRR_TIMER_OFF_INT > (MAX_LONG - irr_prev_ms_off))  // проверяем что при следующем срабатывании таймера произойдет переполнение счетчика ардуино и надо к этому подготовиться
//      //следующее срабатыввание таймера проходит через переполнение 50 дней пройдет и надо считать с нуля
//      {
//        unsigned long x = (MAX_LONG - irr_prev_ms_off); // чтобы не переполнить лонг
//        irr_prev_ms_off = IRR_TIMER_OFF_INT - x; //считаем сдвиг, который будет после переполнения с учетом пройденного времени до переполнения
//      } 
//      
//      irr_prev_ms_on = time; // запоминаем время включени
//      if (IRR_TIMER_ON_INT > (MAX_LONG - irr_prev_ms_on))  // следующее срабатываение таймера отключения проидет через переполнение 50 дней и один час полива  и надо считать с нуля
//      {
//        unsigned long x = (MAX_LONG - irr_prev_ms_on);
//        irr_prev_ms_on = IRR_TIMER_OFF_INT - x; //считаем сдвиг после переполнения с учетом пройденного времени до переполнения. переполнение может произойти во время полива
//      }
//      digitalWrite(IRR, HIGH); // запускаем полив
//      irr_started = true;
//	  vgsm.SendSMS(PHONENUM, "IRRIGATOR START!");
//      #ifdef _TRACE 
//      Serial.println(F("irrigation start")); 
//      #endif
//    }
//  }
//  else // команда на включения полива была дана и надо отсчитать час
//  {
//    if(time > (irr_prev_ms_on + IRR_TIMER_ON_INT)) // автополив включен и прошел час
//    {
//      digitalWrite(IRR, LOW); // выключаем полив
//      irr_started = false;
//	  vgsm.SendSMS(PHONENUM, "IRRIGATOR STOP!");
//      #ifdef _TRACE 
//      Serial.println(F("irrigation stop")); 
//      #endif
//    }
//  };
//
//}
boolean Irrigation::getStarted()
{
  return irr_started;
}
void Irrigation::Init()
{  
  irr_prev_ms_off = 0; // включили ардуино и пошел отсчет дней. через определенный интервал - по умолчанию день, мы даем команду на включение
  pinMode(IRR, OUTPUT);
  delay(500);
  digitalWrite(IRR, LOW);
  #ifdef _TRACE
  Serial.println(F("irregation init")); 
  #endif
}
//void Irrigation::testTest(VGSM &vgsm)
//{
//	if (testflag) return;
//	testflag = true;
//
//	digitalWrite(IRR, HIGH); // запускаем полив
//	irr_started = true;
//	vgsm.SendSMS(PHONENUM, "IRRIGATOR START!");
//	#ifdef _TRACE 
//		Serial.println(F("irrigation start"));
//	#endif
//}
