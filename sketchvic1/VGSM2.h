#pragma once
#include <arduino.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>
#include<HardwareSerial.h>
#include "heat.h"
#include "holl.h"
#include "water.h"
#include "irr.h"
#include "mylcd.h"

const char cmd_start_all[]		PROGMEM = "STARTALL";
const char cmd_start_no_heat[]	PROGMEM = "STARTNOHEAT";
const char cmd_stop_all[]		PROGMEM = "STOPALL";
const char cmd_stop_no_heat[]	PROGMEM = "STOPNOHEAT";
const char cmd_stop_only_heat[]	PROGMEM = "STOPONLYHEAT";
const char cmd_status[]			PROGMEM = "STATUS";
const char cmd_set_temp[]		PROGMEM = "SETTEMP";
const char cmd_set_delta[]		PROGMEM = "SETDELTA";
const char cmd_help[]			PROGMEM = "HELP";
const char cmd_start_irr[]		PROGMEM = { "STARTIRR" };
const char cmd_stop_irr[]		PROGMEM = { "STOPIRR" };


const char* const cmd[]		PROGMEM = { cmd_start_all, cmd_start_no_heat, cmd_stop_all,
cmd_stop_no_heat, cmd_stop_only_heat, cmd_status, cmd_set_temp, cmd_set_delta, cmd_help, cmd_start_irr, cmd_stop_irr};

class VGSM2
{
private:
	SoftwareSerial GSMport = SoftwareSerial(2, 3); // открываем работу с gsm модулем через программный эмулятор сериал через пин 2 и 3
public:
	String phone = "+79160265679";
	String phone2 = "";
	String buffer = "";
	int index = 0;
	VGSM2();
	~VGSM2();
	void InitGSM(MYLCD& lcd);
	boolean SendATCommand();
	void SendInitSMS(String phone);
	void SendHelp(String phone);
	void SendSMS(String text, String phone);
	String ReadGSM();
	int NewSMS();
	String ReadSMS();
	//void ParseCmd(String cmd);
	void Reset();
	boolean CheckSMSCommand(String msg, Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr);
	boolean ConvertTemp(String command, int &t);
	void Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htr1flag, boolean htr2flag, boolean htr3flag);
};

