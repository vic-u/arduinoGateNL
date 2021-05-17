#include "VGSM3.h"

/**
   Функция открывает порт модема на скорости 9600
   и вызывает функцию отрисовки на LCD экране
*/
void VGSM3::Init(MYLCD &lcd) {
	GSMport.begin(9600); //открываем порт модема на скорости 9600
	lcd.gsmInit();
	_lcd = &lcd;
}
/**
   Отправляет в модем команду из commandAT, которая надодится в памяти ардуино
   ожидая хорошего(expected_answer1) или плохого (expected_answer2) ответа, возвращая 1 или 2 соответственно
   вычитывает результат ответа модема в глобальный буфер serial_buff.
   может вернуть и 0, если ничего не нашли
*/
int8_t VGSM3::SendATcommand4(const __FlashStringHelper *commandAT, const char* expected_answer1, const char* expected_answer2, unsigned int timeout, unsigned int adelay)
{
	if (commandAT != NULL) GSMport.println(commandAT);  // если команда не NULL, т.е. она есть, то отправляем в порт модема ее.
	//читаем буфер или после отправки этой команды или текущий буфер от предыдущей команды
	return ReadBuffer(expected_answer1, expected_answer2, timeout, adelay); //смотрим ответ модема и выводим в консоль
}
/**
   Отправляет в модем команду из commandAT
   ожидая хорошего(expected_answer1) или плохого (expected_answer2) ответа, возвращая 1 или 2 соответственно
   вычитывает результат ответа модема в глобальный буфер serial_buff
*/
int8_t VGSM3::SendATcommand4Str(const char* commandAT, const char* expected_answer1, const char* expected_answer2, unsigned int timeout, unsigned int adelay) {
	if (commandAT != NULL) GSMport.println(commandAT);  // отправляем команду в порт модема
	return  ReadBuffer(expected_answer1, expected_answer2, timeout, adelay); //смотрим ответ модема и выводим в консоль
}
/**
   Функция читает данные из буфера модема в глобальную переменную
   serial_buff. и ждет timeout времени пока из сомпорта появится след символ
*/
int8_t VGSM3::ReadBuffer(const char* expected_answer1, const char* expected_answer2, unsigned int timeout, unsigned int adelay) {
	delay(adelay); // подождем перед чтением из буфера
	unsigned long previous = millis(); //начальное значение времени
	memset(serial_buff, '\0', sizeof(serial_buff)); //очистим буфер чтения
	
	int count = 0; //счетчик прочитанных из буффера данных, чтобы не уйти за границу массива
	char empty; //вводим переменную, в которую будем писать, если буфер закончится, а порт еще будет слать данные

	while (GSMport.available() && ((millis() - previous) < timeout)) {  //пока не кончатся данные в порте или не истечет время ожидания читаем их в строку 	
		if (count < (sizeof(serial_buff)-1)) { //пока мы внутри буфера, читаем в него и оставляем последний символ на \0
			serial_buff[count] = GSMport.read();
			//Serial.println(serial_buff);
			count++; // увеличиваем счетчик длины буфер, чтобы не проспать его окончание
		}
		else {
			empty = GSMport.read(); //читаем данные в пустую переменную, надо же их дочитать
		}
		delay(10);
	}
#ifdef _TRACE
	Serial.println(F("-sendAT->"));
	Serial.println(serial_buff);
	_lcd->log(serial_buff);
#endif
	if (strstr_P(serial_buff, expected_answer1) != NULL) return 1;
	if (strstr_P(serial_buff, expected_answer2) != NULL) return 2;
	return 0;
}
/**
Функция удаляет в с строке пробелы и поднимает регистр символов
*/
void VGSM3::DeleteSpaceAndUpper(char *buff)
{
	int i = 0 , j = 0;
	while (buff[i] != '\0') //идем по заполненному буферу пока не встретим конец
	{
		if (buff[i] != ' ') //если текущий символ в буфере не пробел
		{
			buff[j] = toupper(buff[i]);//то его оставляем в буффере на новом месте и поднимаем регистр
			++j;
		}
		++i;
	}
	buff[j] = '\0';
}
/**
   Функция удаляет sms сообщение из памяти модема
   по номеру сообщения int index
*/
void VGSM3::SMSDelete(int index) {
	memset(aux_str, '\0', sizeof(aux_str));
	sprintf_P(aux_str, fmt_sms_del, index);
	SendATcommand4Str(aux_str, mdm_ok, mdm_error, WT5); //посылаем команду на удаление конкретного сообщения
}
/**
удаляем из памяти модем сообщения, чтобы не парсить всякое барахло
*/
void VGSM3::DeleteAllSMS() {
	SendATcommand4(F("AT+CPMS= \"SM\""), mdm_ok, mdm_error, WT5); //выбираем хранилище
	for (int i = 1; i <= SMS_COUNT; i++) SMSDelete(i); // удаляем сообщения 
}
/**
Функция будет ожидать нужного ответа от модема
Посылая в него command
и высматривая в отвте нужный шаблон templ
но не более 3 мин
commandAT может быть NULL, тогда команда не отправляется, а просто читается буфер на наличие шаблона
*/
boolean VGSM3::WaitResponse_P(const __FlashStringHelper * commandAT, const char* expected_answer1, const char* expected_answer2) {
	// сначала поищем шаблон в уже  пришедшем и вычатанном буффере из модема
	if ((strstr_P(serial_buff, expected_answer1) != NULL) || (strstr_P(serial_buff, expected_answer2) != NULL)) return true;
	// в буфере нет, значит шлем формальную команду, чтобы среди ответа увидеть и нужный нам шаблон, ответа. Например, подтверждение отправки смс
	int i = 0;
	//command может скосячить, так как он стат памяти
	while ((SendATcommand4(commandAT, expected_answer1, expected_answer2, WT5) == 0)) {
		if (i >= (12 * 3)) {
			return false; // но не более трех минут 5 секунд ждем в цикле чтения 12 раз в минуту, 
		//иначе принудительно на выход
		}
		i = i + 1;
	}
	return true;
}
/**
функция выделяет кусочек строки от конца встретившегося шаблона templ, до первого разделителя delim или конца строки,
пропуская или from_last символов или до вхождения шаблона empty
from_last - номер символа, с которого надо искать, в него же и записывается значение найденного шаблона
tmpl - шаблон поискового текста
delim - разделитель, до которого выделяется кусок текста
s буфер для возврата результата size_s размер буфера
empty - с какой позиции надо искать
*/
boolean VGSM3::ParseTemplateChr(int &from_last, const char *tmpl, const char *delim, char *s, int size_s, const char *empty) {
	//ищем первое вхождение шаблона в строку
	//если размер буфера возврата =0 или в строке нет шаблона или в строке нет разделителя, то разбирать нечего
	//размер буффера для возрата результата 0 или в строке нет шаблона или в строке нет нужного разделителя, выходим 
	if ((size_s == 0) || (strstr_P(serial_buff, tmpl) == NULL) || (strstr_P(serial_buff, delim) == NULL)) return false;
	if ((empty != NULL) && (strstr_P(serial_buff, empty) == NULL)) return false; //проверяем наличие стартового шаблона, если он задан и его нет в строке,
	//то можно не разбирать
	if (from_last >= (sizeof(serial_buff) - 1)) return false; //позиция начала поиска дальше, чем размер буффера 
	int zero = 0;
	// если мы ищем шаблон не сначала, а хотим пропустить часть строки и искать с нее выставим zero на данную позицию
	if (empty != NULL) zero = (strstr_P(serial_buff, empty) - serial_buff) + strlen_P(empty); 
	// если мы хотим начать поиск только с определенной позиции в строке сдвинем поиск на это количество символов
	if (from_last != 0) zero = from_last;
	//находим вхождение шаблона и переходим на первый символ за ним - это будет позиция first
	if (strstr_P(serial_buff + zero, tmpl) == NULL) return false; //начало шаблона не нашли, выделить кусочек строки не сможем
	int first = (strstr_P(serial_buff + zero, tmpl) - serial_buff) + strlen_P(tmpl); // ищем первое вхождение шаблона 
	// вместо \n добавили \r\n надо проверить
	if (strstr_P(serial_buff + first, delim) == NULL) return false; //конец шаблона не нашли, выделить кусочек строки не сможем
	//находим вхождение разделителя за шаблоном на позиции first и остаемся на нем, это будет позиция last
	int last = (strstr_P(serial_buff + first, delim) - serial_buff);

#ifdef _TRACE
	//Serial.println(strlen(cmd));
	Serial.print(F("ser buf len: "));
	Serial.println(strlen(serial_buff));
	Serial.println(serial_buff);
	Serial.print(F(" first pos: "));
	Serial.println(first);
	Serial.println(tmpl);
	Serial.print(F(" last pos: "));
	Serial.println(last);
#endif
	int len = (last - first); //длина выделяемого куска строки
#ifdef _TRACE
	Serial.print(F(" len: "));
	Serial.println(len);
#endif
	if (size_s <= len) len = size_s - 1; // если размер буфера меньше длины строки, то сохраняем в него часть и закрываем 0
	memset(s, '\0', size_s);
	strncpy(s, serial_buff + first, len); //копируем строку
	from_last = last; // возвращаем значение последнего найденного вхождения шаблона delim позиция начала 
	return true;
}
/**
   Функция вызывается при перезагрузке Arduino, например раз в неделю,
   сбрасывает модем программно,
   переключается на хранилище смс в телефоне и внутренней памяти
   и удаляет из памяти все смс
*/
boolean VGSM3::InitGSM() {
#ifdef _TRACE
	Serial.println(F("Send Reset"));
#endif
	// раскомментить
	if (SendATcommand4(F("AT + CSCLK = 0"), mdm_ok, mdm_error, WT5) != 1) return false; 
	if(SendATcommand4(F("AT+CFUN=1,1"), mdm_ok, mdm_error, 10000, 20000) != 1) return false;//команда перезагрузки модема отправляем в порт модема // ждем 10 секунд

	//if (SendATcommand4(F("AT+CPOWD=0"), mdm_ok, mdm_error, 10000, 60000) != 1) return false;
	//if (SendATcommand4(F("AT"), mdm_ok, mdm_error, 10000, 90000) != 1) return false;//команда перезагрузки модема отправляем в порт модема // ждем 10 секунд
	// здесь надо добавить ожидания до call ready
	if (SendATcommand4(F("AT"), mdm_ok, mdm_error, WT5) != 1) return false;//первая команда в модем, для проверки, что оклемался после 
	//перезагрузки// ждем 10 секунд чтобы все строки модем выдал в буфер
	if (SendATcommand4(F("AT+CPMS= \"SM\""), mdm_ok, mdm_error, WT5) != 1) return false; // переключаем хранилище смс на сим карту и телефон
	DeleteAllSMS();
	return true;
}
/**
функция отправляет приветственное сообщение пристарте или перезагрузке
*/
void VGSM3::SendInitSMSChr()
{
#ifdef _TRACE
	Serial.println(F("Init SMS Send"));
#endif	

	memset(out_msg_buff, '\0', sizeof(out_msg_buff));
	memset(out_phn_buff, '\0', sizeof(out_phn_buff));

	sprintf_P(out_msg_buff, PSTR("%S%c"), help_msg, 0x1A);
	strcpy(out_phn_buff, PHONENUM);
#ifdef _TRACE
	Serial.println("5");
	Serial.println(out_msg_buff);
	Serial.println(out_phn_buff);
#endif	
	SendSMSChr(out_msg_buff, out_phn_buff);
}
/**
функция проверяет наличие непрочитанных сообщений в модеме UNREAD
*/
boolean VGSM3::SMSCheckNewMsg() {
	//переделать на удаление приошибке, может при приходе русских сообщение выдается ошибка, тогда при выходе, надо еще и удалить сообщения
	if (SendATcommand4(F("AT+CMGF=1"), mdm_ok, mdm_error, WT5) != 1) return false; //переводим модем в текстовый режим
#ifdef _TRACE
	//Serial.println(serial_buff);
#endif
	if (SendATcommand4(F("AT+CPMS= \"SM\""), mdm_ok, mdm_error, WT5) != 1) return false; //выбираем хранилище
	if (SendATcommand4(F("AT+CMGL=\"ALL\""), mdm_ok, mdm_error, WT5) != 1) {//прочитаем все сообщения из модема и если переполнится то получим 0
		DeleteAllSMS();//когда придет большое сообщение на русском, оно переполнит будер и мы считаем только часть сообщения без OK и ERR и вернет 0, вот его мы и удалим
		return false; //выходим после удаления всякой хрени
	}
	//получили нормальное текстовое сообщение, его и будем парсить
	char reply[4]; //для двузначных смс
	int fl = 0;
	//нужно выделить номер сообщения в памяти +CMGL:1,1,76
	//+CMGL: 1,«REC UNREAD»,"+7XXXXXXXXXX",«Main»,«11/04/01,18:01:59+16»
	//Test message.
	/*AT + CMGL = "ALL"
		+ CMGL: 1, "REC UNREAD", "+31628870634", , "11/01/09,10:26:26+04"
		This is text message 1
		+ CMGL : 2, "REC UNREAD", "+31628870634", , "11/01/09,10:26:49+04"
		This is text message 2
		OK*/
	if (strstr_P(serial_buff, comma) == NULL) { //если при чтении новых сообщение нет "," , то сообщений новых нет. вернется только ОК
#ifdef _TRACE
		Serial.println(F("no comma"));
#endif
		return false; // и просто выходим
	}
#ifdef _TRACE
	Serial.println(F("find comma in msg"));
#endif
	if (strstr(serial_buff, PHONENUM) != NULL) {//смотрим, что сообщение с моего телефона, на другие не отвечаем			
#ifdef _TRACE
		Serial.println(F("find my phone sms"));
#endif
		//+CMGL: 1,"REC UNREAD","+31628870634",,"11/01/09,10:26:26+04"
		//This is text message 1
		//+CMGL: 2,"REC UNREAD", "+31628870634", , "11/01/09,10:26:49+04"
		//This is text message 2
		//OK
		if (ParseTemplateChr(fl, mdm_sms_list, comma, reply, sizeof(reply))) {//находим номер сообщения 
#ifdef _TRACE
			Serial.println(reply);
#endif
			int sms_index = atoi(reply); //преобразовываем номер в число
			if ((sms_index > 0) && (sms_index < 20)) { //уберем неадекватные числа
				if (ParseTemplateChr(fl, srn_msg, srn_msg, in_msg_buff, sizeof(in_msg_buff), srn_msg)) {//выделяем 	сам текст сообщения	 от ентера до ентера						
					DeleteAllSMS(); //удаляем все сообщения
#ifdef _TRACE
					Serial.println(F("find new msg"));
					Serial.println(reply);
					Serial.println(in_msg_buff);
#endif
					return true;
				}
			}
		}
	}
#ifdef _TRACE
	Serial.println(F("error rd sms"));
#endif
	DeleteAllSMS();
	return false;
}
/**
   Функция вызывается при перезагрузке Arduino, например раз в неделю,
   поднимает стек gprs для работы с интернет
*/
boolean VGSM3::InitGPRS() {
	//функция настраивает модем для передачи данных через инет
	// http://badembed.ru/sim900-tcp-soedinenie-s-serverom/
	//в случае ошибки на одно из этапов, уходим в перезагрузку 
	// Selects Single-connection mode
	if (SendATcommand4(F("AT+CREG?"), mdm_ok, mdm_error, WT5) != 1) return false;//проверяем регистрацию в сети
	if (SendATcommand4(F("AT+CGATT=1"), mdm_ok, mdm_error, WT5) != 1) 
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) return HardSocketReset();//подключаем модуль к GPRS сети
	if (SendATcommand4(F("AT+CIPSHUT"), mdm_ok, mdm_error, WT5) != 1) 
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) return HardSocketReset(); //подждем еще 
	
	SendATcommand4(F("AT+CIPMODE?"), mdm_ok, mdm_error, WT5);

	if (SendATcommand4(F("AT+CIPMUX=0"), mdm_ok, mdm_error, WT5) != 1) return false;//настройка на соединение только с одним сервером
	if (SendATcommand4(F("AT+CIPRXGET=1"), mdm_ok, mdm_error, WT5) != 1) return false;//получение ответа от сервера вручную
	// Sets the APN, user name and password CSTT
	if (SendATcommand4(F(command_APN), mdm_ok, mdm_error, WT5) != 1) 
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) return HardSocketReset();//подключение модема к сотовому оператору
	// Waits for status IP START
	if (!WaitResponse_P(F("AT+CIPSTATUS"), mdm_start, mdm_start)) return false;
	// Brings Up Wireless Connection
	if (SendATcommand4(F("AT+CIICR"), mdm_ok, mdm_error, WT5) != 1) 
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) 
			if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) 
				return HardSocketReset(); //подждем еще return false;//включаем GPRS связь с настройками выше
	
	// Waits for status IP GPRSACT
	if (!WaitResponse_P(F("AT+CIPSTATUS"), mdm_gprsact, mdm_gprsact)) return false;
	// Gets Local IP Address
	if (SendATcommand4(F("AT+CIFSR"), mdm_ip_ok, mdm_error, WT5) != 1) return false;//получаем ip адрес
	// Waits for status IP STATUS
	if (!WaitResponse_P(F("AT+CIPSTATUS"), mdm_ip_status, mdm_ip_status)) return false;
#ifdef _TRACE
	Serial.println(F("Openning TCP/UDP")); //Для удобства наладки дублируем на терминал(надо потом закоментировать)
#endif
	return true;
}
/**
функция передает данные на сервер по протоколу TCP
текущие показания температуры, признак включения обогрева
hf - флаг прихода команды через смс, и тогда надо ее отразить на сервере
roomtemp - температура в комнате
htrflag - флаг включенного отопления
htr - объект отопления
*/
boolean  VGSM3::TCPSendData2(double boxtemp, double roomtemp, boolean htrflag, boolean hollflag, boolean wtrflag, boolean irrflag, Heater& htr, 
	Refrigerator& holl, Water& wtr, Irrigation &irr, boolean hf, boolean rf, boolean wf, boolean irrf, boolean htf, boolean hdf)
{
	//// Gets Local IP Address
	if (SendATcommand4(F("AT+CIFSR"), mdm_ip_ok, mdm_error, WT5) != 1) {
		return HardSocketReset(); //проверяем наличие ip адреса
	}
	if (SendATcommand4(F(data_ip_protocol), mdm_cnct_ok, mdm_already_cnct, WT5) == 0) {//попробуем соединиться с сервером
		//подождем еще
		if (!WaitResponse_P(NULL, mdm_cnct_ok, mdm_already_cnct)) return false;
	}
	if (!WaitResponse_P(F("AT+CIPSTATUS"), mdm_cnct_ok, mdm_cnct_ok)) return false; //еще и статус проверим

	memset(out_msg_buff, '\0', sizeof(out_msg_buff));
	//переводим температуру в комнате в строку
	char str_rt[6];
	char str_bt[6];
	ConvertTempToStr(boxtemp, str_bt, sizeof(str_rt));
	ConvertTempToStr(roomtemp, str_rt, sizeof(str_rt));
	// формат строки в сервер =26FD52AD4E93=O1S1=21.01=23.01=ON=ON=ON=OFF=25=2
	// если в этом цикле команды смс не поступало, то =26FD52AD4E93=O1S1=21.01=23.01=====25=2
	// переводим максимальную температуру в строку
	char str_mt[3];
	memset(str_mt, '\0', sizeof(str_mt));
	dtostrf(htr.max_room_temp, 2, 0, str_mt);
	// переводим дельту температуры в строку
	char str_dt[2];
	memset(str_dt, '\0', sizeof(str_dt));
	dtostrf(htr.delta_temp, 1, 0, str_dt);

#ifdef _TRACE
	Serial.println("+++++++++");
	Serial.println(str_mt);

	Serial.println("+++++++++");
	Serial.println(str_dt);

	Serial.println("StatusChr");
	Serial.println(out_msg_buff);
#endif	
	//формируем строку в сервер
	sprintf_P(out_msg_buff, fmt_http_sts_send, "GET /entry2/", "=" MAC_ADDRESS DEVICENAME, str_bt, "=", str_rt, "=",
		(hf) ? ((htrflag) ? "ON" : "OFF") : "", "=",
		(rf) ? ((hollflag) ? "ON" : "OFF") : "", "=",
		(wf) ? ((wtrflag) ? "ON" : "OFF") : "", "=",
		(irrf) ? ((irrflag) ? "ON" : "OFF") : "", "=",
		(htf) ? str_mt:"", "=", 
		(hdf) ? str_dt:"", " HTTP/1.1\r\n", "Host:194.87.144.141:3000\r\n", "User-Agent:ARDU\r\n", "Accept:text/html\r\n", "Connection:keep-alive\r\n", "\r\n\0");
#ifdef _TRACE
	Serial.println("+++++++++");
	Serial.println(out_msg_buff);
	Serial.println("+++++++++");
#endif	
	//выставляем длину данных которые отправим
	memset(aux_str, '\0', sizeof(aux_str));
	sprintf(aux_str, "AT+CIPSEND=%d", strlen(out_msg_buff));  //Указываем модулю число  байт равное  длине данных в  буфера  out_msg_buff
	if (SendATcommand4Str(aux_str, angbr, mdm_error, WT5) != 1) {//если не получили приглашение ">"  
		if (!WaitResponse_P(NULL, angbr, angbr)) { //подождем пришлашение еще надо получить приглашалку >			
			return HardSocketReset();
		}
	}
	if (SendATcommand4Str(out_msg_buff, mdm_send_ok, mdm_error, WT5) != 1) {//Если всё прошло нормально, то в ответ получим "SEND OK" передаём buf_ip_data в порт. 
		if (!WaitResponse_P(NULL, mdm_send_ok, mdm_send_ok)) {//подждем еще подтверждение отправки
			return HardSocketReset();
		}
	}
	chf = false; //сбрасываем флаг, который передавали в случае прихода команды по смс
	crf = false; //сбрасываем флаг, который передавали в случае прихода команды по смс
	cwf = false; //сбрасываем флаг, который передавали в случае прихода команды по смс
	cirrf = false; //сбрасываем флаг, который передавали в случае прихода команды по смс
	chtf = false;
	chdf = false;
	//тут надо посмотреть ответ от сервера, может надо включить обогрев
	//???????????????? надо читать по двести в цикле три раза, потому что из серийного порта больше 256 не приходит+ заголовок команды 50 символол, поэтому не влезает
	if (SendATcommand4Str("AT+CIPRXGET=2,200", mdm_ok, mdm_error, 3000, 500) != 1) {//читаем ответ сервера он большой
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) { //подждем еще чтение, вдруг долго
			return HardSocketReset();
		}
	}
#ifdef _TRACE
	Serial.println("------1-");
	Serial.println(serial_buff);
#endif
	if (SendATcommand4Str("AT+CIPRXGET=2,200", mdm_ok, mdm_error, 3000, 500) != 1) {//читаем вторую часть
		if (!WaitResponse_P(NULL, mdm_ok, mdm_ok)) { //подждем еще чтение, вдруг долго
			return HardSocketReset();
		}
	}
#ifdef _TRACE
	Serial.println("------2-");
	Serial.println(serial_buff);
#endif
	TCPSocketResponse(htr, holl, wtr, irr); //обработаем ответ сервера
	return true;
}
boolean VGSM3::HardSocketReset() {
	SendATcommand4(F("AT+CIPCLOSE=1"), mdm_close_ok, mdm_error, WT5); //не удалось отправить
	return false;
}
/**
функция разбирает ответ от http сервера и получает оттуда
команду на включение.отключение обогревателя
показатель максимальной температуры
дельту температуры по включению обогрева 
*/
int VGSM3::TCPSocketResponse(Heater &htr, Refrigerator& holl, Water& wtr, Irrigation& irr) {
	// формат строки из сервера =26FD52AD4E93=o1s1==ON=25=2 переедачи данных на сервер
	// в ответе может быть сигнал включения устройства, максимальная температура и гестрезис
	char reply[4];
	memset(reply, '\0', sizeof(reply));//очищаем строку
#ifdef _TRACE
	Serial.println("--parseresponse---");
#endif
	int fl = 0;//начальный символ строки, с которого надо начинать поиск, здесь наинаем с начала строки
	//ищем вхождение =26FD52AD4E94=o2s1=== до следующего = , между ними выделяем ON или OFF
	if (ParseTemplateChr(fl, resp_md, eqv, reply, sizeof(reply))) {
#ifdef _TRACE
		Serial.print("--parse---");
		Serial.println(reply);
#endif
		//даем команду устройству на включение или отключение
		if (strstr_P(reply, on_msg)) {
			//получили команду на включение, но если обогреватель уже включен или находится в цикле прогрева с дельтой, то команду давать не надо
			if ((!htr.heat_started) || (!htr.delta_heat)) htr.heat_command = RC_DEVICEON;
		}
		else if (strstr_P(reply, off_msg)) {
			//получили команду на выключение устройства
			htr.heat_command = RC_DEVICEOFF;
		}
		memset(reply, '\0', sizeof(reply));//очищаем строку
		//смотрим строку дальше, там может быть включение холодильника, 
		//начинаем поиск с последнего значения last - =26FD52AD4E94=o2s1===ON
		//передаем в fl и ищем =ON=
		if (ParseTemplateChr(fl, eqv, eqv, reply, sizeof(reply))) {
		//даем команду устройству на включение или отключение
			if (strstr_P(reply, on_msg)) {
				//получили команду на включение, но если обогреватель уже включен или находится в цикле прогрева с дельтой, то команду давать не надо
				if (!holl.refr_started)  holl.refr_command = RC_DEVICEON;
			}
			else if (strstr_P(reply, off_msg)) {
				//получили команду на выключение устройства
				holl.refr_command = RC_DEVICEOFF;
			}
		}
		//смотрим строку дальше, там может быть включение воды, 
		//начинаем поиск с последнего значения last - =26FD52AD4E94=o2s1===ON=ON
		//передаем в fl и ищем =ON=
		if (ParseTemplateChr(fl, eqv, eqv, reply, sizeof(reply))) {
			//даем команду устройству на включение или отключение
			if (strstr_P(reply, on_msg)) {
				//получили команду на включение, но если обогреватель уже включен или находится в цикле прогрева с дельтой, то команду давать не надо
				if (!wtr.water_started)  wtr.water_command = RC_DEVICEON;
			}
			else if (strstr_P(reply, off_msg)) {
				//получили команду на выключение устройства
				wtr.water_command = RC_DEVICEOFF;
			}
		}
		//смотрим строку дальше, там может быть включение полива, 
		//начинаем поиск с последнего значения last - =26FD52AD4E94=o2s1===ON=ON=ON
		//передаем в fl и ищем =OFF=
		if (ParseTemplateChr(fl, eqv, eqv, reply, sizeof(reply))) {
			//даем команду устройству на включение или отключение
			if (strstr_P(reply, on_msg)) {
				//получили команду на включение, но если обогреватель уже включен или находится в цикле прогрева с дельтой, то команду давать не надо
				if (!irr.irr_started)  irr.irr_command = RC_DEVICEON;
			}
			else if (strstr_P(reply, off_msg)) {
				//получили команду на выключение устройства
				irr.irr_command = RC_DEVICEOFF;
			}
		}
		//смотрим строку дальше, там может быть максимальная температура, 
		//начинаем поиск с последнего значения last - =26FD52AD4E93=o1s1==ON
		//передаем в fl и ищем =25=
		if (ParseTemplateChr(fl, eqv, eqv, reply, sizeof(reply))) {
#ifdef _TRACE
			Serial.println(reply);
#endif
			int t = atoi(reply);
			if ((t>=0) && (t<=30)) htr.max_room_temp = t;
			memset(reply, '\0', sizeof(reply));//очищаем строку
			//смотрим строку дальше, там может быть гестерезис температуры, 
			//начинаем поиск с последнего значения last - =26FD52AD4E93=o1s1==ON=25
			//передаем в fl и ищем =2=
			if (ParseTemplateChr(fl, eqv, srn_msg, reply, sizeof(reply))) {
#ifdef _TRACE
			Serial.println(reply);
#endif
				t = atoi(reply);
				if ((t >= 0) && (t <= 10)) htr.delta_temp = t;
			}
		}
	}
}

/**
   Функция читает разобранный текст sms сообщения в in_msg_buff
   и ищет в нем команду модему. Найдет команду выставит флаги
   отопителю и нагревателю воды. Отправит обратные соощения с подтверждением
   hf вернет флаг, если пришла команда на включение или отключение и сообщит ее на сервер с очередным пакетом
*/
boolean VGSM3::CheckSMSCommand(Heater &htr, Refrigerator &holl, Water &wtr, Irrigation &irr, boolean &hf, boolean& rf, boolean& wf, boolean& irrf, boolean& htf, boolean& hdf)
{
	if ((in_msg_buff == NULL) || (in_msg_buff == "\0")) return false; //пустой буффер, не может содержать команд

#ifdef _TRACE
	Serial.println(F("Check SMS Command"));
	(*_lcd).log(F("Check SMS Command"));
#endif
	hf = false; // флаг, что внутри функции была команда, которая действительно включила отопление
	DeleteSpaceAndUpper(in_msg_buff); //очищаем текст сообщения от пробелов и переводим в верхний регистр
#ifdef _TRACE
	Serial.println(in_msg_buff);
	(*_lcd).log(in_msg_buff);
#endif
	if (strstr_P(in_msg_buff, cmd_start_all) != NULL) //start all пришла команда на включение всех приборов
	{
		htr.heat_command = RC_DEVICEON; //ставим флаг включения обогревателя
		holl.refr_command = RC_DEVICEON;
		wtr.water_command = RC_DEVICEON;
		hf = true; 
		rf = true;
		wf = true;

#ifdef _TRACE
		Serial.println(F("START ALL"));
		(*_lcd).log(F("START ALL"));
#endif
		return true;
	}
	//start no heat
	if (strstr_P(in_msg_buff, cmd_start_no_heat) != NULL) //start no heat
	{
		holl.refr_command = RC_DEVICEON;
		wtr.water_command = RC_DEVICEON;
		rf = true;
		wf = true;
		return true;
	}
	//stop all
	if (strstr_P(in_msg_buff, cmd_stop_all) != NULL) // stop all пришла команда выключения всего
	{
		htr.heat_command = RC_DEVICEOFF; //ставим флаг выключения обогревателя
		holl.refr_command = RC_DEVICEOFF;
		wtr.water_command = RC_DEVICEOFF;
		hf = true;
		rf = true;
		wf = true;
		return true;
	}
	// stop no heat
	if (strstr_P(in_msg_buff, cmd_stop_no_heat) != NULL)
	{
		holl.refr_command = RC_DEVICEOFF;
		wtr.water_command = RC_DEVICEOFF;
		rf = true;
		wf = true;
		return true;
	}
	// stop only heat
	if (strstr_P(in_msg_buff, cmd_stop_only_heat) != NULL) //пришла команда отключения только обогревателя
	{
		htr.heat_command = RC_DEVICEOFF;
		hf = true;
		return true;
	}
	//status
	if (strstr_P(in_msg_buff, cmd_status) != NULL) //пришла команда получения статуса
	{
#ifdef _TRACE
		Serial.println(F("sms status"));
#endif
		return true;
	}
	//set temp
	if (strstr_P(in_msg_buff, cmd_set_temp) != NULL) {
		htf = true;
		return SetDeltaTemp(htr, false, hf);
	}
	//set delta
	if (strstr_P(in_msg_buff, cmd_set_delta) != NULL) {
		hdf = true;
		return SetDeltaTemp(htr, true, hf); //проверяем наличие команды изменения дельты
	}
	//set start irr
	if (strstr_P(in_msg_buff, cmd_start_irr) != NULL)
	{
		irr.irr_command= RC_DEVICEON;
		irrf = true;
		return true;
	}
	//set stop irr
	if (strstr_P(in_msg_buff, cmd_stop_irr) != NULL)
	{
		irr.irr_command = RC_DEVICEOFF;
		irrf = true;
		return true;
	}

	//set help
	if (strstr_P(in_msg_buff, cmd_help) != NULL)
	{
		SendInitSMSChr();
#ifdef _TRACE
		Serial.println(F("sms help"));
#endif
		return true;
	}
	return false; //unknown command
}

void VGSM3::SendSMSChr(char text[], char phone[]) {
#ifdef _TRACE
	Serial.println(F("Send sms char"));
#endif
	if (SendATcommand4(F("AT+CMGF=1"), mdm_ok, mdm_error, WT5) != 1) return;//переводим модем на отправку в текстовом режиме
#ifdef _TRACE
	Serial.print(">cmgf>");
	Serial.println(serial_buff);
#endif
	memset(aux_str, '\0', sizeof(aux_str)); //обнулим буфер отправки
	sprintf_P(aux_str, fmt_sms_phone_send, phone);//отправим номер телефона в модем
	//sprintf_P(aux_str, PSTR("AT+CMGS=\"%s\""), phone);
	if (SendATcommand4Str(aux_str, angbr, mdm_error, WT5) != 1) //отправляем номер телефона и ждем приглашение для отправки текста  
		if (!WaitResponse_P(NULL, angbr, angbr)) return; //надо получить приглашалку >
	//if (SendATcommand4Str(text, angbr, mdm_error, 10000) != 1) return;
	if (SendATcommand4Str(text, mdm_sms_send, mdm_error, WT5) != 1)//подождем ответа от модема, что смс успешно ушла
		WaitResponse_P(NULL, mdm_sms_send, mdm_sms_send); //подождем ответа от модема, что смс успешно ушла
#ifdef _TRACE
	Serial.print(">cmgs>");
	Serial.println(serial_buff);
#endif
	
}
void VGSM3::ConvertTempToStr(double temp, char str_temp[], int str_temp_size) {
	memset(str_temp, '\0', sizeof(str_temp_size));
	dtostrf(temp, 4, 2, str_temp);
	str_temp[5] = 0x00;
#ifdef _TRACE
	Serial.println(str_temp);
#endif
}
void VGSM3::StatusChr(double boxtemp, double roomtemp, boolean wtrflag, boolean hollflag, boolean irrflag, boolean htrflag, int free_ram)
{
	char str_rt[6];
	char str_bt[6];
	Serial.println("StatusChr");
	ConvertTempToStr(boxtemp,str_bt, sizeof(str_rt));
	ConvertTempToStr(roomtemp, str_rt, sizeof(str_rt));
	
	memset(out_msg_buff, '\0', sizeof(out_msg_buff));
	sprintf_P(out_msg_buff, fmt_sms_sts_send,
		bt_msg, str_bt, sn_msg,
		rt_msg, str_rt, sn_msg,
		water_msg, wtrflag ? on_msg : off_msg, sn_msg,
		holl_msg, hollflag ? on_msg : off_msg, sn_msg,
		htr_msg, htrflag ? on_msg : off_msg, sn_msg,
		irr_msg, irrflag ? on_msg : off_msg, sn_msg,
		fr_msg, free_ram, ver_msg, VER, 0x1A);
#ifdef _TRACE
	Serial.println(out_msg_buff);
#endif
	SendSMSChr(out_msg_buff, out_phn_buff);
}

/**
Функция находит в строке command, присланной sms команды, значение температуры, 
на которую надо установить контроль отключения
и возвращает его в t
*/
boolean VGSM3::ConvertTempChr(char * command, int &t)
{
	int i = 0;
	char *istr;
	do
	{
		istr = &command[i++];
		if (istr[0] == '=')//ищем равно в строке
		{
#ifdef _TRACE
			Serial.println("get eq");
#endif
			istr = &command[i++]; // забираем число, оставшаяся часть строки
			t = atoi(istr); //преобразуем в число
			if (isnan(t)) return false; //не преобразовали в число, то ошибка
#ifdef _TRACE
			Serial.println(t);
#endif
			return true; //преобразовали в число, то все хорошо
		}
	} while (istr[0]);
}
/**
Функция изменяет значение максимальной температуры или дельты, 
выставляет флаг обработки команды для tcp и отправляет sms сообщение об этом
delta = true значит меняем дельту, иначе меняем максимальную температуру
*/

boolean VGSM3::SetDeltaTemp(Heater& htr, boolean delta, boolean& hf) {
	int t = 0;
	if (ConvertTempChr(in_msg_buff, t)) //отделяем часть в строке после равно
	{
		if ((t >= 0) && (t <= (delta?5:30))) {
			if (delta) htr.delta_temp = t; else htr.max_room_temp = t;
			hf = true;
			memset(out_msg_buff, '\0', sizeof(out_msg_buff));
			sprintf_P(out_msg_buff, fmt_sms_temp_send, delta?ds_msg:ts_msg, t, 0x1A);
			SendSMSChr(out_msg_buff, out_phn_buff);
		}
	}
#ifdef _TRACE
	Serial.println(F("set delta or temp"));
#endif
	return true;
}
