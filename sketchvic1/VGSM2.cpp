#include "VGSM2.h"

VGSM2::VGSM2(){}
VGSM2::~VGSM2(){}

void VGSM2::InitGSM(MYLCD& lcd) {
	Serial.println(F("InitGSM"));
	GSMport.begin(9600); //открываем порт модема на скорости 9600
	lcd.gsmInit();

}
//перезагружаем модем при запуске ардуины, так как при перезапуске ардуины модем не перезапускается делаем это отдельно
void VGSM2::Reset() {
	Serial.println(F("Send Reset"));
	String AT = "AT+CFUN=1,1"; //команда перезагрузки модема
	GSMport.println(AT);  // отправляем в порт модема
	delay(10000); // ждем 10 секунд
	AT = "AT+CMGDA=\"DEL ALL\""; //очищаем смс в модеме
	GSMport.println(AT);  //
	delay(10000);
	String R = ReadGSM(); //смотрим ответ модема и выводим в консоль
	Serial.println(R);
	//Serial.print("AT+CNMI=1,2,2,1,0\r"); вывод принятого смс в терминал
	/*AT + CNMI = 2, 1, 0, 0, 0                   OK
		Note : <mt >= 1
		---------------------------------------------------- -
		AT + CMTI : "SM", 1
		Note : message received*/

}
//отправляем в модем команду АТ для проверки жив или нет, заодно читаем остатки входного буфера, вдруг там пришло новое сообщение
boolean VGSM2::SendATCommand() {
	Serial.println(F("Send AT"));
	String cmd = "AT";
	GSMport.println(cmd);  //отправляем в модем команду проверки 
	delay(500);
	buffer = ""; //очищаем буфер от предыдущих данных
	buffer = ReadGSM(); // читаем ответ в общий буфер
	//Serial.println(buffer);
	index = NewSMS(); //разбираем данные в буффере на наличие команды о приходе новой смс, если пришла, заберем ее индекс для чтения данных
	//Serial.println(index);
	return (buffer.indexOf(F("OK")) > -1) ? true : false; //возвращаем ответ о том, что модем откликнулся, иначе перегрузим ардуино с модемом
}
//отправляем смс с текстом на определенный номер
void VGSM2::SendSMS(String text, String phone) {
	Serial.println(F("Send sms"));
	GSMport.println("AT+CMGF=1"); //переводим модем в текстовый режим
	delay(500);
	String val = ReadGSM();
	//Serial.println(val);

	GSMport.print("AT+CMGS=\"");//отправляем команду на отправку сообщения
	GSMport.print(phone); //на номер
	GSMport.println("\"");
	delay(500);
	GSMport.print(text); //с текстом
	delay(500);
	GSMport.write(0x1A); // окончание строки и перевод каретки
	GSMport.write(0x0D);
	GSMport.write(0x0A);
	delay(1500);
	val = ReadGSM();//получим ответ модема
	Serial.println(val);
}
//отправляем начальное сообщение при загрузке модема с перечнем возможных команд
void VGSM2::SendInitSMS(String phone) {
	SendSMS(F("GSM INIT\nSTART ALL\nSTART NO HEAT\nSTOP ALL\nSTOP NO HEAT\nSTOP ONLY HEAT\nSTATUS\nSET TEMP=25\nSET DELTA=2\nHELP\nSTART IRR\nSTOP IRR"), phone);
}
void VGSM2::SendHelp(String phone) {
	SendSMS(F("START ALL\nSTART NO HEAT\nSTOP ALL\nSTOP NO HEAT\nSTOP ONLY HEAT\nSTATUS\nSET TEMP=25 \nSET DELTA=2\nHELP\nSTART IRR\nSTOP IRR"), phone);
}
//читаем данные из портма модема, порцию очередных доступных данных
String VGSM2::ReadGSM() {
	int c;
	//char x[200];
	String v = "";// String(x);
	//GSMport.listen();
	while (GSMport.available()) {  //пока не кончатся данные в порте читаем их в строку v
		c = GSMport.read();
		v += char(c);
		delay(10);
		//Serial.println(v);
	}
	return v;// +"\0";
}
//проверяем в буфере отправленном командой АТ появление данных о приходе новой смс, которую надо обработать
int VGSM2::NewSMS() {
	Serial.println("NewSMS");
	//Serial.println(buffer);
	String cmd = "+CMTI:\"SM\","; //шаблон наличия в буффере данных новых сообщений
	buffer.trim();
	buffer.replace(" ", "");
	if (buffer.indexOf(cmd) > -1) { //проверяем наличие в буфере шаблона нового сообщения
		Serial.println("get new sms");
		//Serial.println(cmd.length());
		//Serial.println(buffer.length());
		//Serial.println(buffer);
		int first = buffer.indexOf(cmd) + cmd.length(); //11;// находим конец шалона нового сообщения, за ним будем номер на симке, откуда прочитать сообщение
		//Serial.println(first);
		int last = buffer.indexOf("\n", first); //находим конец строки
		if (last == -1) last = buffer.length(); //бывает, что строка заканчивается без ввода, тогда ставим конец выделения на конец строки
		//Serial.println(last);
		String reply = "";
		reply = buffer.substring(first, last); //получаем номер нового сообщения в памяти сим карты модема
		Serial.println(reply);
		return reply.toInt(); //переводим его в цифру и кладем в индекс, откуда его потом прочитаем в очередном цикле
	}
	else { return 0; } //шаблона в тексте не было, новых сообщений не пришло. первое сообщение кладется под номером 1
}
//разбор пришедшего сообщения
String VGSM2::ReadSMS() {
	String cmd = "AT+CMGR=" + String(index); // формируем команду на прочтение из модема сообщения под номером index
	GSMport.println(cmd);
	delay(1000);
	String response = ReadGSM();
	//	сообщение в модем хранится в следующем виде
	// "+CMGR: \"REC READ\",\" + 790XXXXXXXX\",\"\",\"13 / 09 / 21, 11:57 : 46 + 24\"\nTest sms text\nOK\0";
	Serial.println("ReadSMS");
	Serial.println(response);
	/*Serial.println("Length");
	Serial.println(response.length());*/
//	Serial.println(response);
	response.trim();
	//Serial.println("trim");
	//Serial.println(response);
	response.replace(" ", "");
	//Serial.println("space");
	//Serial.println(response);
	if (response.indexOf("OK") > -1) {//сообщение прочли без ошибок
		Serial.println("OK");
		int first = response.indexOf(","); //находим первую запятую для выделения номера телефона
		int second = response.indexOf(",", first + 1); //находим следующую запятую, которая будет после номера телефона
		String val = response.substring(first + 2, second - 1); //phone without " выделяем номер телефона без кавычек
		Serial.println(val);//печатаем в консоль номер, пока мы его не используем
		first = response.indexOf(",", second); //пропускам пустые кавычки перед датой
		second = response.indexOf(",", first + 1);
		first = response.indexOf(",", second); //date находим запятую перед датой
		second = response.indexOf("\n", first);//находим возврат строки перед текстом сообщения
		first = second + 1; //begin message //это будет начало сообщения
		second = response.indexOf("\n", first);//находим второй возврат строки перед ОК - это бует конец сообщения
		val = response.substring(first, second); //message выделяем сообщение из всего текста
		val.toUpperCase(); // поднимаем регистр для обработки
		Serial.println(val); //печатаем сообщение для проверки в консоль
		cmd = "AT+CMGD=" + String(index) + ",0"; //выполняем команду удаления сообщения из памяти модема
		GSMport.println(cmd);  //отправляем в модем
		delay(500);
		response = ReadGSM();//читаем данные об удаении сообщения и выводим на консоль
		Serial.println(response);
		return val;		//возвращаем текст сообщения
	}
	else { return ""; }
}

boolean VGSM2::CheckSMSCommand(String msg, Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr)
{
	if (msg = "") return false;
#ifdef _TRACE
	Serial.println(F("Check SMS Command"));
	Serial.println(msg);
#endif
	//start all
	if (msg.indexOf(F("STARTALL")) > -1) //start all
	{
		htr.setCommand(RC_DEVICEON);
		holl.setCommand(RC_DEVICEON);
		wtr.setCommand(RC_DEVICEON);
		return true;
	}
	//start no heat
	if (msg.indexOf(F("STARTNOHEAT")) > -1) //start no heat
	{
		holl.setCommand(RC_DEVICEON);
		wtr.setCommand(RC_DEVICEON);
		return true;
	}
	//stop all
	if (msg.indexOf(F("STOPALL")) > -1) // stop all
	{
		htr.setCommand(RC_DEVICEOFF);
		holl.setCommand(RC_DEVICEOFF);
		wtr.setCommand(RC_DEVICEOFF);
		return true;
	}
	// stop no heat
	if (msg.indexOf(F("STOPNOHEAT")) > -1)
	{
		holl.setCommand(RC_DEVICEOFF);
		wtr.setCommand(RC_DEVICEOFF);
		return true;
	}
	// stop only heat
	if (msg.indexOf(F("STOPONLYHEAT")) > -1)
	{
		htr.setCommand(RC_DEVICEOFF);
		return true;
	}
	//status
	if (msg.indexOf(F("STATUS")) > -1)
	{
#ifdef _TRACE
		Serial.println(F("sms status"));
#endif
		return true;
	}
	//set temp
	if (msg.indexOf(F("SETTEMP")) > -1)
	{
		int t = 0;
		if (ConvertTemp(msg, t))
		{
			htr.setMaxRoomTemp(t);
			SendSMS(String(F("TEMP SET ")) + String(t), phone);
		}
#ifdef _TRACE
		Serial.println(F("set temp"));
#endif
		return true;
	}
	//set delta
	if (msg.indexOf(F("SETDELTA")) > -1)
	{
		int t = 0;
		if (ConvertTemp(msg, t))
		{
			htr.setDeltaRoomTemp(t);
			String txt = "DELTA SET " + String(t);
			SendSMS(String(F("DELTA SET ")) + String(t), phone);
		}
#ifdef _TRACE
		Serial.println(F("set delta temp"));
#endif
		return true;
	}
	//set help
	if (msg.indexOf("HELP") > -1)
	{ 
		 SendHelp(phone);
#ifdef _TRACE
		Serial.println(F("sms help"));
#endif
		return true;
	}
	//set start irr
	if (msg.indexOf(F("STARTIRR")) > -1)
	{
		irr.setCommand(RC_DEVICEON);
		return true;
	}
	//set stop irr
	if (msg.indexOf(F("STOPIRR")) > -1)
	{
		irr.setCommand(RC_DEVICEOFF);
		return true;
	}
	return false; //unknown command
}
boolean VGSM2::ConvertTemp(String command, int &t)
{
	int first = command.indexOf("="); //находим знак равно в тексте
	int last = command.length();// получаем длинну строки
	if (first > -1) //символ равно в тексте есть
	{
		String reply = ""; //выделяем значение после равно
		reply = command.substring(first + 1, last); //выделяем кусок строки после равно, до конца строки
#ifdef _TRACE
		Serial.println(reply);
#endif
		t = reply.toInt();//преобразовываем строку в число
		return isnan(t) ? false : true;//возвращаем результат функции, число или не число
	}
	return false; // не нашли равно в строке, тоже возвращаем неудачную попытку
}
void VGSM2::Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htr1flag, boolean htr2flag, boolean htr3flag)
{
	String sms = String(F("STATUS\nBOX TEMP  = ")) + String(boxtemp) + String(F("\nROOM TEMP = ")) + String(roomtemp) + "\n" +
		String(F("HOLL  "))  + String(hollflag? F("ON") : F("OFF")) + "\n" +
		String(F("WATER "))  + String(wtrflag? F("ON") : F("OFF")) + "\n" +
		String(F("IRR "))    + String(irrflag? F("ON") : F("OFF")) + "\n" +
		String(F("HEAT 1 ")) + String(htr1flag? F("ON") : F("OFF")) + "\n" +
		String(F("HEAT 2 ")) + String(htr2flag? F("ON") : F("OFF")) + "\n" +
		String(F("HEAR 3 ")) + String(htr3flag? F("ON") : F("OFF"));
	SendSMS(sms, phone);
}