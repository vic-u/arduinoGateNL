#include "def.h"
#include <arduino.h>

#ifndef VIC_IRR
#define VIC_IRR
//#include "gsm.h"




// класс управления автополивом
class Irrigation {
private:
  unsigned long IRR_TIMER_OFF_INT = 0x5265C00;//1000*60*60*24; // запуск полива раз в сутки в миллисекундах
  unsigned long IRR_TIMER_ON_INT = 0x36EE80;//1000*60*60; // полив длится один час в миллисекундах
  boolean irr_started = false; // флаг указывает текущее состояние реле, включено или нет
  int irr_command = RC_NOTHING; // команда упарвления реле. изначально неопределена. может быть команда на включение или отключение реле
  unsigned long irr_prev_ms_off = 0;
  unsigned long irr_prev_ms_on = 0;
public:
  Irrigation(){};
  ~Irrigation(){};
  //void checkIrrigation(VGSM &vgsm);
  void checkIrrigation2();
  void setCommand(int command);
  boolean getStarted();
  void Init();
  boolean testflag = false;
  //void testTest(VGSM &vgsm);
};
#endif
