#include "def.h"
#include <arduino.h>
#ifndef VIC_WTR
#define VIC_WTR



// класс управления подогревом воды
class Water {
private:
  boolean water_started = false; // флаг указывает текущее состояние реле, включено или нет
  int water_command = RC_NOTHING; // команда упарвления реле. изначально неопределена. может быть команда на включение или отключение реле
public:
  Water(){};
  ~Water(){};
  void checkWater();
  void setCommand(int command);
  boolean getStarted();
  void Init();
};
#endif
