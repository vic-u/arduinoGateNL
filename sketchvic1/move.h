#include "def.h"
#include <arduino.h>
#ifndef VIC_MOVE
#define VIC_MOVE

// класс управления холодильником
class Move {
private:
  boolean move_started = false; // флаг указывает текущее состояние реле, включено или нет
  boolean alarm_send = false; // промежуточный флаг оповещения, первый раз оповестим, потом будем ждать, пока не успокоится и не сработает снова
public:
  Move(){};
  ~Move(){};
  boolean checkMove();
  void Init();
  inline void setCommand(int command) {move_started = command;};
  inline boolean getStarted() {return move_started;};
};
#endif
