#include <arduino.h>

#ifndef VIC_DEV
//не забыть увеличить буфферв в SoftwareSerial до 256
//#define _SS_MAX_RX_BUFF 300
#define VIC_DEV

#define RC_NOTHING 0
#define RC_UNKNOWN 1
#define RC_DEVICEOFF 2
#define RC_DEVICEON 3

#define MAX_LONG 0xFFFFFFFF
#define PHONENUM   "+79160265679"
//отправка данных через смс и отправка данных через интернет
#define VER		15
#define WT5		5000 //время чтения из сериального порта по-умолчанию

#define DHTP	    4   // канал считывания данных с датчика внутри корпуса устройства
#define SSR_2   5  //будем использовать для включения реле всего нагревателя
#define SSR_1   6	// будем использовать для управления термостатом
#define VENT    7   // цифровой пин для включения вентилятора охлаждения внутри корпуса

#define SSR_W  11 // 9 цифровой пин для включения подогрева воды
#define SSR_H   10 // 10 цифровой пин для включения холодильника
#define IRR       12 // 12 цифровой пин для включения автополива
#define LED		13

#define THERM_ROOM_IN  2


#define MAXROOMTEMP  25 // максимальная температура в комнате
#define HIGHBOXTEMP  40 //максимальная темппература в корпусе, после которой включается вентилятор охлаждения
#define DELTATEMP 2 //гестерезис температуры
#define MAC_ADDRESS "26FD52AD4E94"
#define DEVICENAME	"=o2s1="
//tcp index
#define  ON 1
#define OFF 2

//#define _SS_MAX_RX_BUFF 300

#ifndef _TRACE 
#define _TRACE
#endif  
const char help_msg[]			PROGMEM = { "GSM INIT\nSTART ALL\nSTART NO HEAT\nSTOP ALL\nSTOP NO HEAT\nSTOP ONLY HEAT\nSTATUS\nSET TEMP=25 \nSET DELTA=2\nSTART IRR\nSTOP IRR" };
const char bt_msg[]				PROGMEM = { "BOX TEMP = " };
const char rt_msg[]				PROGMEM = { "ROOM TEMP = " };
const char water_msg[]			PROGMEM = { "WATER " };
const char holl_msg[]			PROGMEM = { "HOLL " };
const char htr_msg[]			PROGMEM = { "HEAT  " };
const char irr_msg[]			PROGMEM = { "IRR  " };
const char on_msg[]				PROGMEM = { "ON" };
const char off_msg[]			PROGMEM = { "OFF" };

const char sn_msg[]				PROGMEM = { "\n" };
const char srn_msg[]			PROGMEM = { "\r\n" };
const char fr_msg[]				PROGMEM = { "FR= " };
const char ver_msg[]			PROGMEM = { " V= " };
const char gsci_msg[]			PROGMEM = { "Get sms command at index: " };

const char cmd_start_all[]		PROGMEM = { "STARTALL" };
const char cmd_start_no_heat[]	PROGMEM = { "STARTNOHEAT" };
const char cmd_stop_all[]		PROGMEM = { "STOPALL" };
const char cmd_stop_no_heat[]	PROGMEM = { "STOPNOHEAT" };
const char cmd_stop_only_heat[]	PROGMEM = { "STOPONLYHEAT" };
const char cmd_status[]			PROGMEM = { "STATUS" };
const char cmd_set_temp[]		PROGMEM = { "SETTEMP" };
const char cmd_set_delta[]		PROGMEM = { "SETDELTA" };
const char cmd_start_irr[]		PROGMEM = { "STARTIRR" };
const char cmd_stop_irr[]		PROGMEM = { "STOPIRR" };
const char cmd_help[]			PROGMEM = { "HELP" };


const char ts_msg[]				PROGMEM = { "TEMP SET  " };
const char ds_msg[]				PROGMEM = { "DELTA SET " };

//команды ответа модема
const char mdm_ip_ok[]			PROGMEM = { "." };
const char mdm_error[]			PROGMEM = { "ERROR" };
const char mdm_ok[]				PROGMEM = { "OK" };
const char mdm_ready[]				PROGMEM = { "SMS Ready" };
const char mdm_initial[]		PROGMEM = { "INITIAL" };
const char mdm_empty[]		    PROGMEM = { "" };
const char mdm_start[]		    PROGMEM = { "START" };
const char mdm_ntw_reg_1[]		PROGMEM = { "+CREG:0,1" };
const char mdm_gprsact[]		PROGMEM = { "GPRSACT" };
const char mdm_ip_status[]		PROGMEM = { "IP STATUS" };
const char mdm_cnct_ok[]		PROGMEM = { "CONNECT OK" };
const char mdm_already_cnct[]   PROGMEM = { "ALREADY CONNECT" };
const char mdm_closed[]			PROGMEM = { "CLOSED" };
const char mdm_cnct_fail[]		PROGMEM = { "CONNECT FAIL" };
const char mdm_send_ok[]		PROGMEM = { "SEND OK" };
const char mdm_close_ok[]		PROGMEM = { "CLOSE OK" };
const char mdm_sms_list[]		PROGMEM = { "+CMGL:" };
const char mdm_sms_send[]		PROGMEM = { "+CMGS:" };
const char mdm_call_ready[]    PROGMEM = { "CALL READY" };
const char tmpl_sms[]			PROGMEM = { "+CMTI:" };
const char tmpl_tcp[]			PROGMEM = { "+CIPRXGET:" };

//const char rest_h1[]			PROGMEM = { "GET /entry2/" };
//const char rest_h2[]			PROGMEM = { " HTTP/1.1\r\n" };
//const char rest_h3[]			PROGMEM = { "Host:ardu.damasarent.com\r\n" };
//const char rest_h3[]			PROGMEM = { "Host:194.87.144.141:3000\r\n" };
//const char rest_h4[]			PROGMEM = { "User-Agent:ARDU\r\n" };
//const char rest_h5[]			PROGMEM = { "Accept:text/html\r\n" };
//const char rest_h6[]			PROGMEM = { "Connection:keep-alive\r\n" };
//const char rest_h7[]			PROGMEM = { "\r\n" };
const char resp_md[]			PROGMEM = { "=26FD52AD4E94=o2s1===" };
const char eqv[]					PROGMEM = { "=" };
const char comma[]			PROGMEM = { "," };
const char angbr[]				PROGMEM = { ">" };
const char quota[]				PROGMEM = { "\"" };
const char fmt_sms_del[]			PROGMEM = { "AT+CMGD=%d,0\0" };
const char fmt_sms_i_send[]		PROGMEM = { "%S%d%c" };
const char fmt_sms_phone_send[]	PROGMEM = { "AT+CMGS=\"%s\"" };
const char fmt_sms_temp_send[]	PROGMEM = { "%S%d%c" };
const char fmt_sms_sts_send[]  PROGMEM = { "%S%s%S%S%s%S%S%S%S%S%S%S%S%S%S%S%S%S%S%d%S%d%c" };
const char fmt_http_sts_send[]  PROGMEM = { "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s" };
/**
итерфейс любово обогревательного устройства
*/
class SSR
{
public:
	/**
  Выключает все датчики
  */
	void Off()
	{
		digitalWrite(SSR_1, LOW);
		delay(500);
		digitalWrite(SSR_2, LOW);
		delay(500);
		digitalWrite(SSR_W, LOW);
		delay(500);
		digitalWrite(SSR_H, LOW);
		delay(500);
		digitalWrite(IRR, LOW);
	};
	/**
   Мигает лампочками в случае нормального прохождения  в программе
   пять средних морганий
   */
	void Blink() { // когда все хорошо 5 морганий
		pinMode(LED, OUTPUT);
		for (int i = 0; i < 5; i++)
		{
			digitalWrite(LED, HIGH); // turn the LED on (HIGH is the voltage level)
			delay(1000);
			digitalWrite(LED, LOW); // turn the LED off by making the voltage LOW
			delay(1000);
		}
	}
	/**
	Мигает лампочками в слачае ошибки в программе
	три долгих моргания
	*/
	void ErrorBlink() {//когда плохо, то три долгих
		pinMode(LED, OUTPUT);
		for (int i = 0; i < 3; i++)
		{
			digitalWrite(LED, HIGH); // turn the LED on (HIGH is the voltage level)
			delay(3000);
			digitalWrite(LED, LOW); // turn the LED off by making the voltage LOW
			delay(3000);
		}
	}
	/**
	проверяет свободное место в памяти на наличие утечек
	*/
	int freeRam() {
		extern int __heap_start, * __brkval;
		int v;
		return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
	}

};
#endif
