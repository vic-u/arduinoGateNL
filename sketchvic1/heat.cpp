#include "heat.h"

void Heater::setCommand(int command) 
{
  heat_command = command;
}
void Heater::OnTen(int tens) // включаем тены обогрева один, два или три
{
  if (tens >= 1) // включаем первый тен
  {
    digitalWrite(SSR_1, HIGH);
    heat_started1 = true;
  }
  if (tens >= 2) //первый тен запустили и еще включаем второй
  {
    digitalWrite(SSR_2, HIGH);
    heat_started2 = true;
  }
  if (tens == 3) //включили первый, второй и еще и третий тены
  {
    digitalWrite(SSR_3, HIGH);
    heat_started3 = true;
  }
  #ifdef _TRACE 
    Serial.println(F("heater start")); 
  #endif 
  heat_started = true;
}
void Heater::setRoomTemp(double temp) 
{
  room_temp = temp;
  setTempArr(room_temp);
}
void Heater::setMaxRoomTemp(double temp) 
{
  max_room_temp = temp;
}
void Heater::setDeltaRoomTemp(double temp)
{
	delta_temp = temp;
}

void Heater::checkHeat() // включение и отключение обогревателя
{ 
  if(heat_command == RC_DEVICEON) 
  {
    Serial.println(F("heater device on command"));
    Serial.println(room_temp);
    //if (room_temp < 4) // меньше 4 градусов, холодно. включаем три тена
    //{
    //  OnTen(3);
    //  heat_command = RC_NOTHING;
    //  return;
    //}
    //if (room_temp < 16) // от 4 до 16. включаем два тена
    //{
    //  OnTen(2);
    //  heat_command = RC_NOTHING;
    //  return;
    //}
	digitalWrite(SSR_2, HIGH); //включаем компрессор
	heat_started2 = true;
	delay(2000); //пропускаем скачек по включению компрессора
    if (room_temp < max_room_temp) // если больше 16 но меньше выставленной температуры, то включаем подогрев одного тена
    {
		digitalWrite(SSR_1, HIGH); //включаем тен нагрева
		heat_started = true;
		heat_started1 = true;
		heat_command = RC_NOTHING;
		Serial.println(F("on heater ten one"));
		return;
    }
  }
  if(heat_command == RC_DEVICEOFF)
  {
	digitalWrite(SSR_1, LOW); // отключаем нагрев
	delay(2000);
	digitalWrite(SSR_2, LOW); // отключаем насос
	heat_started = false;
	heat_started1 = false;
	heat_started2 = false;
    return;
  }
  // проверка без команды на остановку нагрева
  if (heat_started) {
    if (getTempArr() >= max_room_temp) // прогрели больше чем задана максимальная температура
    {
		digitalWrite(SSR_1, LOW); //отключаем только нагрев
		delta_heat = true; // включаем временный флаг проверки поддержания температуры
		heat_started1 = false;
		heat_started = false;
		return;
    }
  }
  if (delta_heat) // включен временный флаг поддержания температуры 
  { 
    if (getTempArr() < (max_room_temp - delta_temp)) //температура остыла на два градуса
    {
      digitalWrite(SSR_1, HIGH); // запускаем один тен
      heat_started1 = true;
      heat_started = true;
      delta_heat = false;
      return;
    }
  }   
}
 
boolean Heater::getStarted()
{
  return heat_started;
}

boolean Heater::getStarted(int tens)
{
  if (tens == 1) return heat_started1;
  if (tens == 2) return heat_started2;
  if (tens == 3) return heat_started3;
}

void Heater::Init()
{
  pinMode(SSR_1, OUTPUT);
  delay(500);
  digitalWrite(SSR_1, LOW);
  pinMode(SSR_2, OUTPUT);
  delay(500);
  digitalWrite(SSR_2, LOW);
  pinMode(SSR_3, OUTPUT);
  delay(500);
  digitalWrite(SSR_3, LOW);
  #ifdef _TRACE
  Serial.println(F("heater init")); 
  #endif
}
void Heater::StopHeat()
{
      digitalWrite(SSR_1, LOW); // отключаем тены
      heat_started1 = false;
      digitalWrite(SSR_2, LOW);
      heat_started2 = false;
      digitalWrite(SSR_3, LOW);
      heat_started3 = false;
      heat_started = false; // выключаем признак работы обогревателя
}
void Heater::setTempArr(double rt)
{ // заносим очередное значение в массив для вычисления среднего значения температуры против шума
	room_temp_arr[room_temp_arr_index] = rt;
	room_temp_arr_index++;
	if (room_temp_arr_index > 19) room_temp_arr_index = 0;
}
double Heater::getTempArr()
{
	double sum = 0;
	for (int i = 0; i < 20; i++)
	{
		sum = sum + room_temp_arr[i];
	}
	return sum / 20;
}