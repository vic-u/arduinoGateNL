#include "def.h"
#include <arduino.h>
#ifndef VIC_HEAT
#define VIC_HEAT



// класс управления подогревом воды
class Heater {
private:
  boolean heat_started = false; // флаг указывает текущее состояние реле, включено или нет
  
  boolean heat_started1 = false;
  boolean heat_started2 = false;
  boolean heat_started3 = false;
  
  boolean delta_heat = false;
  double room_temp = -100;
  double max_room_temp = MAXROOMTEMP;
  double delta_temp = DELTATEMP;
  double room_temp_arr[20] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  int room_temp_arr_index = 0;
  int heat_command = RC_NOTHING; // команда упарвления реле. изначально неопределена. может быть команда на включение или отключение реле
  void OnTen(int tens); // включаем тены
  void StopHeat(); // выключаем все тены обогревателя
public:
  Heater(){};
  ~Heater(){};
  void checkHeat();
  void setCommand(int command);
  boolean getStarted();
  boolean getStarted(int tens);
  void setMaxRoomTemp(double temp);
  void setRoomTemp(double temp);
  void setDeltaRoomTemp(double temp);

  void setTempArr(double rt); //вычисляем среднее значение  температуры 20 последниъ значений
  double getTempArr(); // выдаем среднее значение температуры
  void Init();
};
#endif
