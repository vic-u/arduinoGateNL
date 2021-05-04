#include "heat.h"

//void Heater::setCommand(int command)
//{
//	heat_command = command;
//}
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
#ifdef _TRACE 
	Serial.println(F("heater start"));
#endif 
	heat_started = true;
}
/*
функция выставляет текущую температуру
и пополняет массив данных общей средней температуры
*/
void Heater::setRoomTemp(double temp)
{
	room_temp = temp;
	setTempArr(room_temp);
}
//void Heater::setDeltaRoomTemp(double temp)
//{
//	delta_temp = temp;
//}

void Heater::checkHeat() // включение и отключение обогревателя
{
	if (heat_command == RC_DEVICEON)
	{
		Serial.println(F("heater device on command"));
		Serial.println(room_temp);
		digitalWrite(SSR_2, HIGH); //включаем компрессор
		heat_started2 = true;
		heat_command = RC_NOTHING;
		delay(2000); //пропускаем скачек по включению компрессора
		if (getTempArr() < max_room_temp) // если больше 16 но меньше выставленной температуры, то включаем подогрев одного тена
		{
			//текущая температура меньше максимальной, включаем подогрев
			digitalWrite(SSR_1, HIGH); //включаем тен нагрева
			heat_started = true;
			heat_started1 = true;
			heat_command = RC_NOTHING;
			Serial.println(F("on heater ten one"));
			return;
		} else  {
			delta_heat = true;
			return;
		}
	}
	if (heat_command == RC_DEVICEOFF)
	{
		digitalWrite(SSR_1, LOW); // отключаем нагрев
		delay(2000);
		digitalWrite(SSR_2, LOW); // отключаем насос
		heat_started = false;
		heat_started1 = false;
		heat_started2 = false;
		delta_heat = false;//добавлено после ошибки 19.05 после выключения, через какое то время был включен
		heat_command = RC_NOTHING;
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
}

void Heater::switchAnalogSensor(int pin, int signal) {
	pinMode(pin, OUTPUT);
	delay(500);
	digitalWrite(pin, signal);
}
void Heater::Init()
{
	switchAnalogSensor(SSR_1, LOW);
	switchAnalogSensor(SSR_2, LOW);

#ifdef _TRACE
	Serial.println(F("heater init"));
#endif
}
void Heater::StopHeat() //TODO  надо только тен отключить, а компрессор не дергать 
{
	digitalWrite(SSR_1, LOW); // отключаем тены
	heat_started1 = false;
	digitalWrite(SSR_2, LOW);
	heat_started2 = false;
	heat_started = false; // выключаем признак работы обогревателя
}
void Heater::setTempArr(double rt)
{ // заносим очередное значение в массив для вычисления среднего значения температуры против шума
	room_temp_arr[room_temp_arr_index] = rt;
	room_temp_arr_index++;
	if (room_temp_arr_index > 19) room_temp_arr_index = 0;
}
/**
вычисляем среднюю температуру из 20 последних показателей датчика
*/
double Heater::getTempArr()
{
	double sum = 0;
	for (int i = 0; i < 20; i++)
	{
		sum = sum + room_temp_arr[i];
	}
	return sum / 20;
}
/**
функция собирает с аналогового датчика показатель температуры
*/
double Heater::getRoomTemp()
{
	analogReference(DEFAULT);
	int raw_adc = analogRead(THERM_ROOM_IN);
	double temp = log(((10240000 / raw_adc) - 10000));
	temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
	temp = temp - 273.15;
	return temp;
}