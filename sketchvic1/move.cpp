#include "move.h"


boolean Move::checkMove()
{
  if (!move_started) return false; // информация с датчика отключена командой, ничего не сообщаем
  int val = analogRead(1);//digitalRead(MV); // цепляем на аналоговый датчик, так как цифровые закончились
  Serial.print(F("move "));
  Serial.println(val);
  if(val > 0)
  {
    if (!alarm_send)
    {
      Serial.println(F("move sensor alarm"));
      alarm_send = true; // устанавливаем промежуточный флаг, чтобы послать сигнал, только один раз, а потом послать только после успокоения датчика
      return true;
    } else return false; // датчик сработал и мы уже оповестили, ждем пока не успокоится
  }
  else
  {
    alarm_send = false; // датчик успокоился, сигнализировать не надо
    return false;
  }
  
}
void Move::Init()
{
  Serial.println(F("move init start"));
  delay(30000);
  Serial.println(F("move sensor stop"));
}

