#include <Wire.h>
#include <SoftwareSerial.h>

#include "DHT.h"
#include "def.h"
#include "holl.h"
#include "irr.h"
#include "water.h"
#include "heat.h"
#include "temp.h"
#include "mylcd.h"
#include "move.h"
#include "VGSM3.h"
#include "GyverWDT.h"
/////////////////////////////////////////////////

Refrigerator holl; //объект класса управления холодидьником   
Irrigation irr; //объект класса управления автополивом
Water wtr; // объект класса управления водонагревателем
Heater htr; // объект класс управления нагревателем
SSR ssr; // объект управления твердотельными реле всеми настраивает и экстренно выключает их
Temp tmp;
MYLCD lcd(0x27, 16, 2);

VGSM3 vgsm3;
bool firststart = true; //флаг первого прохода цикла, чтобы отправить
bool gprsactive = false;//флаг включение канала интернета в дополнение к смс

void(*resetFunc) (void) = 0; //перезагрузочная функция

void setup() {
  
  Serial.begin(9600);//открываем ком порт для вывода отладки
  #ifdef _TRACE
	Watchdog.enable(INTERRUPT_MODE, WDT_PRESCALER_1024); // Режим генерации 
	Serial.println(F("first"));
	lcd.log(F("first"));
  #endif
  tmp.Init(); // включаем датчики температуры и инициализируем вентилятор
  holl.Init();// определяем реле холодильника на выход и отключаем его
  irr.Init(); // определяем реле автополива и сбрасываем счетчик 
  wtr.Init(); // определяем реле подогрева воды на выход и отключаем его
  htr.Init(); // определяем реле первого контура подогрева и отключаем его
  lcd.Init(); // включаем подсветку дисплея и ставим курсов в начало
#ifdef _TRACE
  Serial.println(F("Power on, arduino"));
  lcd.log(F("Power on, arduino"));
#endif
  vgsm3.Init(lcd); //открываем порт модема  на скорости 9600 и выводим на дисплей
#ifdef _TRACE
  Serial.println(F("After InitGSM"));
  lcd.log(F("After InitGSM"));
#endif
  Watchdog.reset();
  if (!vgsm3.InitGSM()) resetFunc(); // проверяем инициализацию ГСМ и ГПРС, не смогли перегружаем устройство и модем
#ifdef _TRACE
  Serial.println(F("After reset"));
  lcd.log(F("After reset"));
#endif
  // модем нам нужен обязательно а gprs доп функция
  Watchdog.reset();
  gprsactive = vgsm3.InitGPRS(); //дополнительно включаем инициализацию для интернета
  Watchdog.reset();
  htr.max_room_temp = MAXROOMTEMP; // выставляем начальную температуру отключения
  for (int i = 0; i < 20; i++)
  {
	  htr.setRoomTemp(htr.getRoomTemp()); //заполняем температурный массив данными
  }
#ifdef _TRACE
  Serial.println(F("Done Init"));
  lcd.log(F("Done Init"));
#endif  
}

void loop() {
	Watchdog.reset();
#pragma region WEEKLY REBOOT
	//добавим общую регулярную  перезагрузку  - раз в неделю
	if (604800000 - long(millis()) <= 0) {
		resetFunc();
	}
	htr.setRoomTemp(htr.getRoomTemp()); //передаем обогревателю данные с датчика
#pragma endregion
#pragma boxtemp
	if (tmp.CheckBoxTemp()) ssr.Off(); // проверяем температуру внутри блока, если
#pragma endregion
#pragma region CHECK MODEM ABILITY
	Watchdog.reset();
	if (vgsm3.SendATcommand4(F("AT"), mdm_ok, mdm_ok, WT5) != 1) {//отправляем в модем АТ команду и читаем текущий буфер. В этот момент может прийти команда смс
#ifdef _TRACE
		Serial.println(F("No AT. Reset"));
		lcd.log(F("No AT. Reset"));
#endif
		ssr.ErrorBlink();
		resetFunc(); //пока модем не откликнется будем перезапускать ардуино
	}
#ifdef _TRACE
	Watchdog.reset();
	Serial.println(F("MODEM OK"));
	lcd.log(F("MODEM OK"));
#endif
	lcd.Status(tmp.getBoxTemp(), htr.getTempArr(), holl.getStarted(), wtr.getStarted(), irr.getStarted(), htr.getStarted(), true, htr.max_room_temp, htr.delta_temp); // на LCD экран выводим информацию
#pragma endregion
#pragma region FIRST START

	if (firststart) {// на первом запуске шлем SMS команду пользователю, что живы
		Watchdog.reset();
		vgsm3.SendInitSMSChr();
		firststart = false; // чтобы не слать повторно при следующем цикле
		Watchdog.reset();
	}
#pragma endregion
	if (!gprsactive) {
		Watchdog.reset();
		lcd.log(F("InitGPRS"));
		gprsactive = vgsm3.InitGPRS(); //если в предыдущий заход отправка по tcp не удалась, попробуем запустить gprs
		Watchdog.reset();
	}
#pragma region SMS CHECK
	if (vgsm3.SMSCheckNewMsg()) {// проверяем наличие UNREAD сообщений;
#ifdef _TRACE
		Serial.println(F("NewSMS")); //Done1
		lcd.log(F("NewSMS"));
#endif
		Watchdog.reset();
		if (vgsm3.CheckSMSCommand(htr, holl, wtr, irr, vgsm3.chf, vgsm3.crf, vgsm3.cwf, vgsm3.cirrf, vgsm3.chtf, vgsm3.chdf)) { // проверяем приход sms комманды, в случае прихода выставляем флаги для дальнейшей обработки действий
#ifdef _TRACE
			Serial.println(F("NewCom")); //Done3
			lcd.log(F("NewCom"));
#endif
			Watchdog.reset();
			htr.checkHeat();// отрабатываем флаг включения обогревателя
			holl.checkRefrigerator(); // отрабатываем флаг команды включения холодильника
			wtr.checkWater();// отрабатываем флаг команды включения подогрева воды
			irr.checkIrrigation2();// отрабатываем автополив
			vgsm3.StatusChr(tmp.getBoxTemp(), htr.getTempArr(), holl.getStarted(), wtr.getStarted(), irr.getStarted(), htr.getStarted(1), ssr.freeRam()); // если пришла удаленная команда, формируем ответ
		}
	}
#pragma endregion
#pragma region TCP SEND
	Watchdog.reset();
	if (gprsactive) gprsactive = vgsm3.TCPSendData2(tmp.getBoxTemp(), htr.getTempArr(), htr.getStarted(1), holl.getStarted(), wtr.getStarted(), irr.getStarted(), htr, holl, wtr, irr, 
		vgsm3.chf, vgsm3.crf, vgsm3.cwf, vgsm3.cirrf, vgsm3.chtf, vgsm3.chdf);
#pragma endregion
	htr.checkHeat(); //проверяем только обогрев, так как он может быть уже включен
	holl.checkRefrigerator(); // отрабатываем флаг команды включения холодильника
	wtr.checkWater();// отрабатываем флаг команды включения подогрева воды
	irr.checkIrrigation2();// отрабатываем автополив
	ssr.Blink();
#ifdef _TRACE
	Serial.print(F("free memory - "));
	Serial.println(ssr.freeRam());
	lcd.FreeRam(ssr.freeRam(), htr.max_room_temp, htr.delta_temp);
#endif
	Watchdog.reset();
	Watchdog.disable();
	delay(30000); //ждем 30 секунд и делаем новый опрос 
	Watchdog.enable(INTERRUPT_MODE, WDT_PRESCALER_1024); // Режим генерации 
}
/* Прерывание watchdog */
ISR(WATCHDOG) {
	resetFunc();
}