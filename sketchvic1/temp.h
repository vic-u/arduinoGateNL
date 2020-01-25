#include "def.h"
#include <arduino.h>
#ifndef VIC_TEMP
#define VIC_TEMP
#include "DHT.h"

// класс управления холодильником
class Temp {
private: 
  DHT dht = DHT(DHTP, DHT22);
  // переменные для обогревателя
  double boxtemp = -100;
  boolean vent_started = false;
public:
  Temp(){};
  ~Temp(){};
  double getRoomTemp();
  double getBoxTemp();
  boolean CheckBoxTemp();
  void Init();
};
#endif
