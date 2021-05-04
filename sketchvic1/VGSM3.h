#pragma once
#include <SoftwareSerial.h>
#include "def.h"
#include "water.h"
#include "heat.h"
#include "mylcd.h"
#include "holl.h"
#include "irr.h"


//команда - конфигурации модема под оператора MTS
//#define command_APN "AT+CSTT=\"internet.mts.ru\",\"mts\",\"mts\""
//команда - конфигурации модема под оператора beeline
#define command_APN "AT+CSTT=\"\",\"beeline\",\"beeline\""
//команда - конфигурации модема под оператора МегаФон
//#define command_APN "AT+CSTT=\"internet\",\"gdata\",\"gdata\""
//команда - конфигурации модема под оператора Теле2
//#define command_APN "AT+CSTT=\"internet.tele2.ru\",\"\",\"\""
#define data_ip_protocol "AT+CIPSTART=\"TCP\",\"194.87.144.141\",\"3000\""
//#define data_ip_protocol "AT+CIPSTART=\"TCP\",\"ardu.damasarent.com\",\"80\""
#define SMS_COUNT 25

class VGSM3
{
private:
	SoftwareSerial GSMport = SoftwareSerial(2, 3); // открываем работу с gsm модулем через программный эмулятор сериал через пин 2 и 3
	MYLCD *_lcd;
public:
	char serial_buff[302];
	char aux_str[30];
	char out_msg_buff[302];
	char out_phn_buff[13];
	char in_msg_buff[15];
	char last_tcp_command[4];
	boolean chf = false; //флаг прихода команды по смс с управлением обогревателем
	boolean crf = false; //флаг прихода команды по смс с управлением холодильником
	boolean cwf = false; //флаг прихода команды по смс с управлением водой
	boolean cirrf = false; //флаг прихода команды по смс с управлением авто поливом
	
	void Init(MYLCD &lcd); // открываем порт и отображаем инфу на дисплее
	int8_t SendATcommand4(const __FlashStringHelper * commandAT, const char* expected_answer1, const char* expected_answer2, unsigned int timeout = 10000, unsigned int adelay = 1000);
	int8_t SendATcommand4Str(const char* commandAT, const char* expected_answer1, const char* expected_answer2, unsigned int timeout = 10000, unsigned int adelay = 1000);
	int8_t ReadBuffer(const char* expected_answer1, const char* expected_answer2, unsigned int timeout, unsigned int adelay);
	boolean WaitResponse_P(const __FlashStringHelper * commandAT, const char* expected_answer1, const char* expected_answer2);
	boolean InitGSM();
	boolean InitGPRS();

	void DeleteSpaceAndUpper(char *buff);
	void SendInitSMSChr();
	void SendSMSChr(char text[], char phone[]);

	void ConvertTempToStr(double temp, char str_temp[], int str_temp_size);
	
	boolean SMSCheckNewMsg();
	void SMSDelete(int index);
	boolean CheckSMSCommand(Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr, boolean& hf, boolean& rf, boolean& wf, boolean& irrf);
	void StatusChr(double boxtemp, double roomtemp, boolean wtrflag, boolean hollflag, boolean irrflag, boolean htrflag, int free_ram);
	boolean ConvertTempChr(char * command, int &t);
	boolean TCPSendData2(double boxtemp, double roomtemp, boolean htrflag, boolean hollflag, boolean wtrflag, boolean irrflag, Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr, boolean hf, boolean rf, boolean wf, boolean irrf);
	boolean HardSocketReset();
	int TCPSocketResponse(Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr);
	boolean ParseTemplateChr(int &from_last, const char *tmpl, const char *delim, char *s, int size_s, const char *empty = NULL);
	void DeleteAllSMS();
	boolean SetDeltaTemp(Heater& htr, boolean delta, boolean& hf);
};
