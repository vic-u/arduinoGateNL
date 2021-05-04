#include "temp.h"

double Temp::getBoxTemp()
{
  return boxtemp;
}

double Temp::getRoomTemp() 
{
  int raw_adc = analogRead(THERM_ROOM_IN);
  double temp = log(((10240000/raw_adc) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
  #ifdef _TRACE 
    Serial.print(F("room temp ")); 
    Serial.println(temp);
  #endif
  return temp;
}
void Temp::Init()
{
  //pinMode(THERMROOMIN, INPUT);
  dht.begin();
  #ifdef _TRACE
  Serial.println(F("Set cooler output"));
  #endif
  pinMode(VENT, OUTPUT);
  delay(500);
  digitalWrite(VENT, LOW);
}

boolean Temp::CheckBoxTemp()
{
  double h = dht.readHumidity();
  double t = dht.readTemperature();
  boxtemp = t;
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) 
  {
    Serial.println(F("Failed to read from DHT"));
    return true; //если датчик перестал работать отключаем все реле
  } 
  else 
  {
    Serial.print(F("Humidity: ")); 
    Serial.print(h);
    Serial.print(F(" %\t"));
    Serial.print(F("Temperature: ")); 
    Serial.print(boxtemp);
    Serial.println(F(" *C"));
    
    if(t > HIGHBOXTEMP) 
    {
      if (!vent_started) 
      {
        digitalWrite(VENT, HIGH);
        Serial.println(F("Cooler start"));
        vent_started = true;
      };
    };
    if(t <(HIGHBOXTEMP - 2)) 
    {
      vent_started = false;
      digitalWrite(VENT, LOW);
      Serial.println(F("Cooler stop "));
    };
    return (t >(HIGHBOXTEMP + 5))? true: false; 
  };  
}
