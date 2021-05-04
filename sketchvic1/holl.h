#include "def.h"
#include <arduino.h>
#ifndef VIC_REFR
#define VIC_REFR


// класс управления холодильником
class Refrigerator {
public:
  boolean refr_started = false; // флаг указывает текущее состояние реле, включено или нет
  int refr_command = RC_NOTHING; // команда упарвления реле. изначально неопределена. может быть команда на включение или отключение реле
public:
  Refrigerator(){};
  ~Refrigerator(){};
  void checkRefrigerator();
  boolean getStarted();
  void Init();
};
#endif
