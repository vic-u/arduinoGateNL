#include <Wire.h>
#include <SoftwareSerial.h>

//#include "HWSerial.h"
//#include <HardwareSerial.h>
//#include "LOG.h"

//#include "Streaming.h"
//#include "WideTextFinder.h"

//#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "def.h"
#include "holl.h"
#include "irr.h"
#include "water.h"
#include "heat.h"
#include "temp.h"
#include "mylcd.h"
#include "move.h"
//#include "gsm.h"
#include "VGSM2.h"
/////////////////////////////////////////////////

Refrigerator holl; //объект класса управления холодидьником   
Irrigation irr; //объект класса управления автополивом
Water wtr; // объект класса управления водонагревателем
Heater htr; // объект класс управления нагревателем
SSR ssr; // объект управления твердотельными реле всеми настраивает и экстренно выключает их
Temp tmp;
MYLCD lcd(0x27, 16, 2);
//Move msr;
//VGSM vgsm;
VGSM2 vgsm2;

unsigned long tim;
bool firststart = true;
int led = 13;


void setup() {
  
  Serial.begin(9600);//открываем ком порт для вывода отладки
  #ifdef _TRACE
    Serial.println(F("Power on, arduino"));
  #endif
  tmp.Init(); // включаем датчики температуры и инициализируем вентилятор
  holl.Init();// определяем реле холодильника на выход и отключаем его
  irr.Init(); // определяем реле автополива и сбрасываем счетчик 
  wtr.Init(); // определяем реле подогрева воды на выход и отключаем его
  htr.Init(); // определяем реле первого контура подогрева и отключаем его
  
  ssr.Init(); // включаем аварийный контроллер твердотельных реле
  lcd.Init(); // включаем подсветку дисплея и ставим курсов в начало
  //vgsm.Init(lcd);
  vgsm2.InitGSM(lcd);
  vgsm2.Reset(); // вызываем команду перезагрузки устройства при первом запуске
  Serial.println(F("\nDone Init"));
  //msr.Init(); // инициализация датчика движения
  
  htr.setMaxRoomTemp(MAXROOMTEMP); // выставляем начальную температуру отключения подогрева в 22 градуса
  
}
void(*resetFunc) (void) = 0;

//resetFunc();
void loop() {
	//добавим общую регулярную  перезагрузку  - раз в неделю
	//tim = millis();
	//if (604800000 - long(tim) <= 0) resetFunc(); //asm volatile ("jmp 0x0000");
	if (604800000 - long(millis()) <= 0) resetFunc();

	Serial.println(F("Done0"));
	if (!vgsm2.SendATCommand()) {
		resetFunc(); //пока модем не откликнется будем перезапускать ардуино
	}
	else {
		Serial.println(F("MODEM OK"));
		lcd.Status(tmp.getBoxTemp(), htr.getTempArr(), holl.getStarted(), wtr.getStarted(), irr.getStarted(), htr.getStarted(), true);
	}

	if (firststart) {// на первом запуске шлем команду, что живы
		vgsm2.SendInitSMS(vgsm2.phone);
		firststart = false;
	}
	if (vgsm2.index > 0) { //пришло смс
		String msg = vgsm2.ReadSMS(); //прочтем очищенную смс в строку

		//if((!vgsm.ModemIsAlive()) || (!vgsm.getStarted()))
		//{  
		//	vgsm.Init(lcd);
		//} // повторно пробуем стартовать gsm модуль, если при инициализации не получилось

		Serial.println(F("Done1"));
		if (tmp.CheckBoxTemp()) ssr.Off(); // проверяем температуру внутри блока, если датчик температуры сломался или температура выше максимальной на 5 градусов (не уменьшается), то выключаем все реле. внутри функции включаем вентилятор
		Serial.println(F("Done2"));
		if (vgsm2.CheckSMSCommand(msg, htr, holl, wtr, irr)) { // проверяем приход sms комманды, в случае прихода выставляем флаги для дальнейшей обработки действий
			Serial.println(F("Done3"));
			holl.checkRefrigerator(); // отрабатываем флаг команды включения холодильника
			wtr.checkWater();// отрабатываем флаг команды включения подогрева воды
			htr.setRoomTemp(tmp.getRoomTemp()); //передаем обогревателю данные с датчика температуры в комнате
			htr.checkHeat();// отрабатываем флаг включения обогревателя
			irr.checkIrrigation2();// отрабатываем автополив
			//irr.testTest(vgsm);
			vgsm2.Status(tmp.getBoxTemp(), htr.getTempArr(), holl.getStarted(), wtr.getStarted(), irr.getStarted(), htr.getStarted(1), htr.getStarted(2),
				htr.getStarted(3)); // если пришла удаленная команда, формируем ответ
		}
	}
	ssr.Blink();
	delay(5000); //ждем 5 секунд и делаем новый опрос
}




