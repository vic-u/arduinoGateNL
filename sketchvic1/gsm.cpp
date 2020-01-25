#include "gsm.h"
//инициализируем состояние приема из порта, с ожиданием освобождения 
void VGSM::RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
	rx_state = RX_NOT_STARTED; // выставляем флаг ожидания прихода данных в порт
    start_reception_tmout = start_comm_tmout; //выставляем время ожидания ответа из порта после отправки туда ат команды
    interchar_tmout = max_interchar_tmout; // выставляем время ожидания получения очередного байта из порта
    prev_time = millis(); // выставляем в таймер текущий счетчик тиков ардуино для отсчета времени ожидания
    comm_buf[0] = 0x00; // end of string в начало буфера ставим признак конца строки делая наш буфер пустым
    p_comm_buf = &comm_buf[0]; // ставим текущий указатель заполнения буфера в начало буфера
    comm_buf_len = 0; // сбрасываем счетчик длины буфера
    _cell.flush(); // erase rx circular buffer ожидаем отправки текущей ат команды, до окончания отправки не начинаем прием
}
// функция читает данные из порта в буфер обмена comm buf[]
byte VGSM::IsRxFinished(void)
{
     byte num_of_bytes;
     byte ret_val = RX_NOT_FINISHED;  // default not finished

     if (rx_state == RX_NOT_STARTED) // первый раз зашли в функцию или в очередной раз заходим, а чтение из буфера все не началось 
     {
       if (!_cell.available())  // в буфере ничего нет
       { 
         if ((unsigned long)(millis() - prev_time) >= start_reception_tmout)  // когда чтение из буфера так и не началось, то проверяем превышение ожидания ответа на команду
         {
            comm_buf[comm_buf_len] = 0x00; // чтение не удалось, возвращаем пустой буфер и возвращаем ошибку чтения данных из порта
            ret_val = RX_TMOUT_ERR;
         }
       } 
       else 
       { // данные в буфере какие-то есть, надо их прочитать с учетом интервала чтения символов из буфера
          prev_time = millis(); // init tmout for inter-character space ставим новый отсчет, но уже чтения очередного байта из буфера
          rx_state = RX_ALREADY_STARTED; // меняем признак, что чтение данных из порта началось
       }
     }
      
     if (rx_state == RX_ALREADY_STARTED) 
     { // обнаружили в буфере данные, надо их прочитать в буфер 
       num_of_bytes = _cell.available();
       if (num_of_bytes) prev_time = millis();// if there are some received bytes postpone the timeout включаем счетчик ожидания следующей порции данных, пока будем обрабатывать эту 
       while (num_of_bytes) 
       {// read all received bytes
         num_of_bytes--;
         if (comm_buf_len < COMM_BUF_LEN) 
         { // пока буфер не переполнится читаем в него, потом просто читаем
            *p_comm_buf = _cell.read(); // читаем очередной символ из буфера и сдвигаем указатель для следующего символа
             p_comm_buf++;
             comm_buf_len++;
             comm_buf[comm_buf_len] = 0x00;  // прописываем нуль конец, чтобы строка всегда была готова если количество символов в буфере порта закончится
         } 
         else 
         { // место в нашем буфере закончилось. просто читаем чтобы все из буфера вычитать
           _cell.read();
         }
       }
       // проверяем ожидание чтения очередной порции данных из порта, если спустя заданный интервал очередная порция не приходит, то устанавливаем в нашем буфере нуль строку и завершаем чтение данных  
       if ((unsigned long)(millis() - prev_time) >= interchar_tmout) 
       {
         comm_buf[comm_buf_len] = 0x00;  // for sure finish string again // but it is not necessary
         ret_val = RX_FINISHED; // возвращаем признак окончания чтения данных из буфера порта в наш буфер
       }
     }
     return (ret_val);
}
/*ожидает ответа от модема после отправки в него команды и заполняет буффер comm buf[]. 
первый параметр, время ожидания появления данных в порте
второй параметр, время ожидания очередной партии данных после прочтения предыдущей
каждое такое чтение ставит указатель в начало буффера, но не очищает его, но ставит признак конца строки
*/
byte VGSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
	byte status;
	RxInit(start_comm_tmout, max_interchar_tmout); //включаем порт на прием данных
	// будем читать из буфера, пока не придет подтверждение, что в буфере ничего нет
	do 
	{
		status = IsRxFinished(); //будем читать из порта, пока не опустеет буфер или не пройдет таймаут ожидания
	} 
	while (status == RX_NOT_FINISHED);
	return (status);
}
/*ожидает ответа от модема после отправки в него команды и заполняет буффер comm buf[] и ищет вхождение нужной строки.
первый параметр, время ожидания появления данных в порте
второй параметр, время ожидания очередной партии данных после прочтения предыдущей
третий параметр, искомая строка в буффере
каждое такое чтение ставит указатель в начало буффера, но не очищает его, но ставит признак конца строки
*/
byte VGSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, char const *expected_resp_string)
{
     byte status;
     byte ret_val;

     RxInit(start_comm_tmout, max_interchar_tmout); // включаем порт на прием данных
     // wait until response is not finished
     do {
          status = IsRxFinished();
     } while (status == RX_NOT_FINISHED);
	 //прочитали в буффер ответ данные из порта
     if (status == RX_FINISHED) {
          // something was received but what was received?
          // ---------------------------------------------

          if(IsStringReceived(expected_resp_string)) {
               // expected string was received
               // ----------------------------
               ret_val = RX_FINISHED_STR_RECV;
          } else {
               ret_val = RX_FINISHED_STR_NOT_RECV;
          }
     } else {
          // nothing was received
          // --------------------
          ret_val = RX_TMOUT_ERR;
     }
     return (ret_val);
}
//функция проверяет нахождение искомой строки в буффере comm buf, прочитанном из порта
byte VGSM::IsStringReceived(char const *compare_string)
{
	if(comm_buf_len) // текущий заполненный буфер не пустой
	{
		return (strstr((char *)comm_buf, compare_string) != NULL)? 1:0; // возвращаем один, если нужный шаблон ответа есть в буфере 
	}
	return 0; // если буфер пустой, то и шаблон в нем не найдется
}
// посылает в порт ат команду модема и ожидает результата определенное количество попыток
char VGSM::SendATCmdWaitResp(char const *AT_cmd_string, uint16_t start_comm_tmout, uint16_t max_interchar_tmout, char const *response_string, byte no_of_attempts)
{
     byte status;
     char ret_val = AT_RESP_ERR_NO_RESP;

     for (byte i = 0; i < no_of_attempts; i++) 
     { // запускаем цикл отправки ат команды заданное количество попыток
          if (i > 0) delay(500);// ожидаем 500 msec перед тем, как послать следующую ат команду. при первой попытки не ждем
          _cell.println(AT_cmd_string); // отправляем команду в порт
          status = WaitResp(start_comm_tmout, max_interchar_tmout); // ждем определенный интервал прихода команд с заданным интервалом заполнения очередной порции буфера
          if (status == RX_FINISHED) 
          { // в принципе команду получили, посмотрим, что в ней
               if(IsStringReceived(response_string)) //ищем в строке ответа нужный шаблон ответа
               {
                    ret_val = AT_RESP_OK;
                    break;  // response is OK => finish как только получили ответ выходим из функции
               } 
               else ret_val = AT_RESP_ERR_DIF_RESP; // получили, но в ответе нет заданного шаблона правильного ответа
          } 
          else 
          {
               ret_val = AT_RESP_ERR_NO_RESP; // вариант когда ответа от модема не получили
          }

     }

     WaitResp(1000, 5000); //подождем 
     return (ret_val);
}

void VGSM::InitParam(byte group)
{
  switch (group) 
  {
     case PARAM_SET_0:
          SetCommLineStatus(CLS_ATCMD);
          // Reset to the factory settings
          SendATCmdWaitResp("AT&F", 1000, 50, str_ok, 5);
          // switch off echo
          SendATCmdWaitResp("ATE0", 500, 50, str_ok, 5);
          SetCommLineStatus(CLS_FREE);
          break;

     case PARAM_SET_1:
          SetCommLineStatus(CLS_ATCMD);
          // Request calling line identification
          SendATCmdWaitResp("AT+CLIP=1", 500, 50, str_ok, 5);
          // Mobile Equipment Error Code
          SendATCmdWaitResp("AT+CMEE=0", 500, 50, str_ok, 5);
          // set the SMS mode to text
          SendATCmdWaitResp("AT+CMGF=1", 500, 50, str_ok, 5);
          SetCommLineStatus(CLS_FREE);
          InitSMSMemory();
          // select phonebook memory storage
          SendATCmdWaitResp("AT+CPBS=\"SM\"", 1000, 50, str_ok, 5);
          SendATCmdWaitResp("AT+CIPSHUT", 500, 50, "SHUT OK", 5);
          break;
   }
}
//включает или выключает эхо. возвращение комманд 0 выкл, 1 вкл
void VGSM::Echo(byte state)
{
	SetCommLineStatus(CLS_ATCMD);
	_cell.print("ATE");
	_cell.print((int)state);
	_cell.print("\r");
	delay(500);
	SetCommLineStatus(CLS_FREE);   
}
char VGSM::InitSMSMemory(void)
{
     char ret_val = -1;

     if (CLS_FREE != GetCommLineStatus()) return (ret_val);
     SetCommLineStatus(CLS_ATCMD);
     ret_val = 0; // not initialized yet

     // Disable messages about new SMS from the GSM module
     SendATCmdWaitResp("AT+CNMI=2,0", 1000, 50, str_ok, 2);

     // send AT command to init memory for SMS in the SIM card
     // response:
     // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
     if (AT_RESP_OK == SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", 1000, 1000, "+CPMS:", 10)) {
          ret_val = 1;
     } else ret_val = 0;

     SetCommLineStatus(CLS_FREE);
     return (ret_val);
}

byte VGSM::SendSMS(char const *number_str, char const *message_str)
{
  if(strlen(message_str)>159) Serial.println(F("Don't send message longer than 160 characters"));
  char ret_val = -1;
  char end[2];
  end[0]=0x1a;
  end[1]='\0';
     
  for (byte i = 0; i < 1; i++) 
  {
    // send  AT+CMGS="number_str"
    _cell.print(F("AT+CMGS=\""));
    _cell.print(number_str);
    _cell.println("\"");
    // 1000 msec. for initial comm tmout
    // 50 msec. for inter character timeout
    if (RX_FINISHED_STR_RECV == WaitResp(1000, 500, ">")) 
    {
      // send SMS text
      _cell.print(message_str);
      _cell.println(end);
      //_cell.flush(); // erase rx circular buffer
      if (RX_FINISHED_STR_RECV == WaitResp(7000, 5000, "+CMGS")) 
      {
        // SMS was send correctly
        ret_val = 1;
        break;
      } else continue;
    } else {
      // try again
      continue;
    }
 }
 SetCommLineStatus(CLS_FREE);
 return (ret_val);
}
//удаляем сообщение из памяти телефона по индексу
byte VGSM::DeleteSMS(byte position)
{
     char ret_val = -1;

     if (position == 0) return (-3); //сообщения начинаются с индекса 1
     if (CLS_FREE != GetCommLineStatus()) return (ret_val); // если порт занят возвращаем неудачу удаления
     SetCommLineStatus(CLS_ATCMD);// выставляем флаг занятости порта на передачу команды
     ret_val = 0; // not deleted yet

     //send "AT+CMGD=XY" - where XY = position
     _cell.print(F("AT+CMGD="));
     _cell.print((int)position);
	 _cell.println(F(",4"));


     // 5000 msec. for initial comm tmout
     // 20 msec. for inter character timeout
     switch (WaitResp(5000, 50, "OK")) {
     case RX_TMOUT_ERR:
          // response was not received in specific time
          ret_val = -2;
          break;

     case RX_FINISHED_STR_RECV:
          // OK was received => SMS deleted
          ret_val = 1;
          break;

     case RX_FINISHED_STR_NOT_RECV:
          // other response: e.g. ERROR => SMS was not deleted
          ret_val = 0;
          break;
     }

     SetCommLineStatus(CLS_FREE);
     return (ret_val);
}

byte VGSM::IsSMSPresent(byte required_status)
{
     char ret_val = -1;
     char *p_char;
     byte status;

     if (CLS_FREE != GetCommLineStatus()) return (ret_val);
     SetCommLineStatus(CLS_ATCMD);
     ret_val = 0; // still not present

     switch (required_status) 
     {
     case SMS_UNREAD:
          _cell.println(F("AT+CMGL=\"REC UNREAD\""));
          break;
     case SMS_READ:
          _cell.println(F("AT+CMGL=\"REC READ\""));
          break;
     case SMS_ALL:
          _cell.println(F("AT+CMGL=\"ALL\""));
          break;
     }

     // 5 sec. for initial comm tmout
     // and max. 1500 msec. for inter character timeout
     RxInit(5000, 1500);
     // wait response is finished
     do {
          if (IsStringReceived("OK")) 
          {
               // perfect - we have some response, but what:

               // there is either NO SMS:
               // <CR><LF>OK<CR><LF>

               // or there is at least 1 SMS
               // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
               // <CR><LF> <data> <CR><LF>OK<CR><LF>
               status = RX_FINISHED;
               break; // so finish receiving immediately and let's go to
               // to check response
          }
          status = IsRxFinished();
     } while (status == RX_NOT_FINISHED);

     switch (status) 
     {
     case RX_TMOUT_ERR:
          // response was not received in specific time
          ret_val = -2;
          break;

     case RX_FINISHED:
          // something was received but what was received?
          // ---------------------------------------------
          if(IsStringReceived("+CMGL:")) 
          {
               // there is some SMS with status => get its position
               // response is:
               // +CMGL: <index>,<stat>,<oa/da>,,[,<tooa/toda>,<length>]
               // <CR><LF> <data> <CR><LF>OK<CR><LF>
               p_char = strchr((char *)comm_buf,':');
               if (p_char != NULL) 
               {
                    ret_val = atoi(p_char+1);
               }
          } else {
               // other response like OK or ERROR
               ret_val = 0;
          }

          // here we have WaitResp() just for generation tmout 20msec. in case OK was detected
          // not due to receiving
          WaitResp(20, 20);
          break;
     }

     SetCommLineStatus(CLS_FREE);
     return (ret_val);
}
// получает сообщение по его номеру из гсм модуля
byte VGSM::GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len)
{
     char ret_val = -1;
     char *p_char;
     char *p_char1;
     byte len;

     if (position == 0) return (-3);// номер сообщения начинается с единицы
     if (CLS_FREE != GetCommLineStatus()) return (ret_val); // проверяем, что контроллер записи в модуль свободен
     SetCommLineStatus(CLS_ATCMD);//ставим признак занятости контроллера на запист АТ комманд
     phone_number[0] = 0;  // end of string for now
     ret_val = GETSMS_NO_SMS; // still no SMS

     //send "AT+CMGR=X" - where X = position
     _cell.print(F("AT+CMGR=")); //посылаем через последовательный порт модуля команду на получение сообщения по номеру
     _cell.println((int)position);

     // 5000 msec. for initial comm tmout
     // 100 msec. for inter character tmout
     switch (WaitResp(5000, 100, "+CMGR")) {
     case RX_TMOUT_ERR:
          // response was not received in specific time
          ret_val = -2;
          break;

     case RX_FINISHED_STR_NOT_RECV:
          // OK was received => there is NO SMS stored in this position
          if(IsStringReceived("OK")) {
               // there is only response <CR><LF>OK<CR><LF>
               // => there is NO SMS
               ret_val = GETSMS_NO_SMS;
          } else if(IsStringReceived("ERROR")) {
               // error should not be here but for sure
               ret_val = GETSMS_NO_SMS;
          }
          break;

     case RX_FINISHED_STR_RECV:
          // find out what was received exactly

          //response for new SMS:
          //<CR><LF>+CMGR: "REC UNREAD","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
          //There is SMS text<CR><LF>OK<CR><LF>
          if(IsStringReceived("\"REC UNREAD\"")) {
               // get phone number of received SMS: parse phone number string
               // +XXXXXXXXXXXX
               // -------------------------------------------------------
               ret_val = GETSMS_UNREAD_SMS;
          }
          //response for already read SMS = old SMS:
          //<CR><LF>+CMGR: "REC READ","+XXXXXXXXXXXX",,"02/03/18,09:54:28+40"<CR><LF>
          //There is SMS text<CR><LF>
          else if(IsStringReceived("\"REC READ\"")) {
               // get phone number of received SMS
               // --------------------------------
               ret_val = GETSMS_READ_SMS;
          } else {
               // other type like stored for sending..
               ret_val = GETSMS_OTHER_SMS;
          }

          // extract phone number string
          // ---------------------------
          p_char = strchr((char *)(comm_buf),',');
          p_char1 = p_char+2; // we are on the first phone number character
          p_char = strchr((char *)(p_char1),'"');
          if (p_char != NULL) {
               *p_char = 0; // end of string
               strcpy(phone_number, (char *)(p_char1));
          }


          // get SMS text and copy this text to the SMS_text buffer
          // ------------------------------------------------------
          p_char = strchr(p_char+1, 0x0a);  // find <LF>
          if (p_char != NULL) {
               // next character after <LF> is the first SMS character
               p_char++; // now we are on the first SMS character

               // find <CR> as the end of SMS string
               p_char1 = strchr((char *)(p_char), 0x0d);
               if (p_char1 != NULL) {
                    // finish the SMS text string
                    // because string must be finished for right behaviour
                    // of next strcpy() function
                    *p_char1 = 0;
               }
               // in case there is not finish sequence <CR><LF> because the SMS is
               // too long (more then 130 characters) sms text is finished by the 0x00
               // directly in the gsm.WaitResp() routine

               // find out length of the SMS (excluding 0x00 termination character)
               len = strlen(p_char);

               if (len < max_SMS_len) {
                    // buffer SMS_text has enough place for copying all SMS text
                    // so copy whole SMS text
                    // from the beginning of the text(=p_char position)
                    // to the end of the string(= p_char1 position)
                    strcpy(SMS_text, (char *)(p_char));
               } else {
                    // buffer SMS_text doesn't have enough place for copying all SMS text
                    // so cut SMS text to the (max_SMS_len-1)
                    // (max_SMS_len-1) because we need 1 position for the 0x00 as finish
                    // string character
                    memcpy(SMS_text, (char *)(p_char), (max_SMS_len-1));
                    SMS_text[max_SMS_len] = 0; // finish string
               }
          }
          break;
     }

     SetCommLineStatus(CLS_FREE);
     return (ret_val);
}
//проверяем доступен ли модем. отправляем в него пустую команду, если положительной команды не придет функция вернет false и надо будет перегружать модем
boolean VGSM::ModemIsAlive()
{
	SetCommLineStatus(CLS_ATCMD); //выставляем признак передачи в порт ат комманды для модема
	boolean result = (AT_RESP_OK == SendATCmdWaitResp(str_at, 500, 100, str_ok, 5)) ? true : false; // ждем положительного результата от отправленной команды
	SetCommLineStatus(CLS_FREE); // выставляем признак свободной линии для передачи комманд
	return result;
}
//включает и сбрасывает модем. отправляем в него команду на инициализацию
int VGSM::begin(long baud_rate)
{
	// Set pin modes
	pinMode(GSM_ON, OUTPUT);
	pinMode(GSM_RESET, OUTPUT);
	int response = -1;
	//int cont = 0;
	boolean norep = true; // флаг отсутствия ответа от устройства
	boolean turnedON = false; //флаг включенного устройства
	SetCommLineStatus(CLS_ATCMD); //выставляем признак передачи в порт ат комманды для модема
	_cell.begin(baud_rate);//открываем последовательный порт для работы на данной скорости
	p_comm_buf = &comm_buf[0]; // выставляем указатель позиции в буфере на начало буффера
	setStatus(IDLE); //устанавливаем неопределенный статус работы модема
	// три раза попытаемся запустить модуль 
	for (byte cont = 0; cont < 3; cont++) 
	{
		if (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp(str_at, 500, 100, str_ok, 5) && !turnedON) // отправляем команду АТ с ожиданием ответа в 5 попыток && !turnedON - зачем тут, не понятно
		{	//модем не ответил на простую проверочную команду, тогда перегружаем его	
			digitalWrite(GSM_ON, HIGH); //проверяли доступность модема и не получили команду ответа. попробуем включить модем и со второй попытки в цикле опросить модем
			delay(1200);
			digitalWrite(GSM_ON, LOW);
			delay(10000);
			WaitResp(1000, 1000); //дочитываем из буфера всякую дрянь инициализации
		} 
		else 
		{
			WaitResp(1000, 1000); //дочитываем из буфера всякую дрянь
		}
	}
	// после трех посылок команды АТ, и в случае необходимости перезагрузки модема, читаем четвертый раз 
	if (AT_RESP_OK == SendATCmdWaitResp(str_at, 500, 100, str_ok, 5)) // модем успешно ответил на наш тестовый запрос, ставим признак включенности модема и снимаем признак недоступности
	{
		turnedON = true;
		norep = false;
	}
  
	SetCommLineStatus(CLS_FREE); // выставляем признак свободной линии для передачи комманд
	if(turnedON) 
	{
		WaitResp(50, 50);
		//InitParam(PARAM_SET_0);
		//InitParam(PARAM_SET_1);//configure the module
		//Echo(0);               //enable AT echo
		setStatus(READY);
		return(1);
	} 
	else 
	{
		return(0);
	}
}
//устанавливает флаг включения для gsm модуля и отправляет сообщение на телефон, после чего показывает информацию на экране
boolean VGSM::Init(MYLCD& lcd)
{
	if (begin(9600))
	{
		#ifdef _TRACE
			Serial.println(F("\n gsm status=READY"));
		#endif
		gsm_started = true;
		lcd.gsmInit();
		if (SendInitSMS()) lcd.gsmInitSMS();
		return true;
	} 
	#ifdef _TRACE
	  Serial.println(F("\nstatus=IDLE"));
	#endif
	gsm_started = false;
	lcd.gsmFail();
	return false;
}
boolean VGSM::SendInitSMS()
{
	if(gsm_started) 
	{
		if (SendSMS(PHONENUM, "ARDUINO GSM INIT") && SendSMS(PHONENUM2, "ARDUINO GSM INIT")) //отправляем сообщения на два номера
		{
			#ifdef _TRACE
				Serial.println(F("\nSMS sent OK"));
			#endif
			for(int i = 1; i <= 20; i++) //очищаем очередь сообщений сим карты
			{
				Serial.println(F("\ndelete sms"));
				while (DeleteSMS(i) != 1) {}; //будем очищать очередь сообщений, пока не удалим
			}
			Serial.println(F("\nDelete sms done"));
			return true;
		} 
  }
  return false;
} 
boolean VGSM::SendHelp()
{
	if (gsm_started)
	{
		if (SendSMS(PHONENUM, "START ALL\nSTART NO HEAT\nSTOP ALL\nSTOP NO HEAT\nSTOP ONLY HEAT\nSTART IRR\nSTOP IRR\nSTATUS\nSET TEMP\nSET DELTA")&&
			SendSMS(PHONENUM2, "START ALL\nSTART NO HEAT\nSTOP ALL\nSTOP NO HEAT\nSTOP ONLY HEAT\nSTART IRR\nSTOP IRR\nSTATUS\nSET TEMP\nSET DELTA"))
		{
			return true;
		}
	}
	return false;
}
boolean VGSM::SendTempOrDelta(char * txt)
{
	if (gsm_started)
	{
		if (SendSMS(PHONENUM, txt)&& SendSMS(PHONENUM2, txt))
		{
			return true;
		}
	}
	return false;
}
boolean VGSM::ConvertTemp(char * command, int &t)
{
	int i = 0;
	char *istr;
	do
	{
		istr = &command[i++];
		if (istr[0] == '=')
		{
			Serial.println("get eq");
			istr = &command[i++];
			t = atoi(istr);
			if (isnan(t)) return false;
			Serial.println(t);

			return true;
		}
	} while (istr[0]);
}
//функция удаляет пробелы из строки
char* VGSM::DeleteSpace(char* s)
{
	int i = 0, j = 0;

	while (s[i] != '\0')
	{
		if (s[i] != ' ')
		{
			s[j] = s[i];
			++j;
		}

		++i;
	}

	s[j] = '\0';

	return s;
}
//проверяем наличие нужной комманды в буфере sms
boolean VGSM::CheckSMSCommand(Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr)
{
	byte sms_position = 0;
	char sms_phone_number[14];
	char sms_text[80];

	const char cmd_start_all[]		PROGMEM  = {"STARTALL"};
	const char cmd_start_no_heat[]	PROGMEM  = {"STARTNOHEAT"};
	const char cmd_stop_all[]		PROGMEM  = {"STOPALL"};
	const char cmd_stop_no_heat[]	PROGMEM  = {"STOPNOHEAT"};
	const char cmd_stop_only_heat[]	PROGMEM  = {"STOPONLYHEAT"};
	const char cmd_status[]			PROGMEM  = {"STATUS"};
	const char cmd_set_temp[]		PROGMEM  = {"SETTEMP"};
	const char cmd_set_delta[]		PROGMEM  = {"SETDELTA"};
	const char cmd_help[]			PROGMEM  = {"HELP" };
	const char cmd_start_irr[]		PROGMEM = { "STARTIRR" };
	const char cmd_stop_irr[]		PROGMEM = { "STOPIRR" };
  
	if (!gsm_started) return false;
	#ifdef _TRACE
		Serial.println(F("sms check"));
    #endif
        
    sms_position = IsSMSPresent(SMS_UNREAD); //получаем позицию очередного непрочитанного сообщения
	if (!sms_position) return false; // новых сообщений нет, выходим
      
	GetSMS(sms_position, sms_phone_number, sms_text, 80);//получаем содержимое полученного сообщения
	while (DeleteSMS(sms_position) != 1) {}; // удаляем пока не удалим
	DeleteSpace(sms_text);//удаляем пробелы из текста
    #ifdef _TRACE
		Serial.println(sms_text);
    #endif
	if(strcasestr(sms_text, cmd_start_all) != NULL)
    {
		Serial.println(F("start all"));
        htr.setCommand(RC_DEVICEON);
        holl.setCommand(RC_DEVICEON);
        wtr.setCommand(RC_DEVICEON);
        return true;
    }
	if(strcasestr(sms_text, cmd_start_no_heat) != NULL)
    {
        holl.setCommand(RC_DEVICEON);
        wtr.setCommand(RC_DEVICEON);
        return true;
    }
	if (strcasestr(sms_text, cmd_start_irr) != NULL)
	{
		irr.setCommand(RC_DEVICEON);
		return true;
	}
    if(strcasestr(sms_text, cmd_stop_all) != NULL)
    {
        htr.setCommand(RC_DEVICEOFF);
        holl.setCommand(RC_DEVICEOFF);
        wtr.setCommand(RC_DEVICEOFF);
        return true;
    }
    if(strcasestr(sms_text, cmd_stop_no_heat) != NULL)
    {
        holl.setCommand(RC_DEVICEOFF);
        wtr.setCommand(RC_DEVICEOFF);
        return true;
    }
	  if (strcasestr(sms_text, cmd_stop_irr) != NULL)
	  {
		  irr.setCommand(RC_DEVICEOFF);
		  return true;
	  }
    if(strcasestr(sms_text, cmd_stop_only_heat) != NULL)
    {
        htr.setCommand(RC_DEVICEOFF);
        return true;
    }
    if(strcasestr(sms_text, cmd_status) != NULL)
    {
		#ifdef _TRACE
			Serial.println(F("sms status"));
		#endif
    return true;
    }
	if (strcasestr(sms_text, cmd_help) != NULL) // при получении вопроса, отвечаем справкой
	{
		SendHelp();
		#ifdef _TRACE
		  Serial.println(F("sms help"));
		#endif
		return true;
	}
	if (strcasestr(sms_text, cmd_set_temp) != NULL)
	{
		int t = 0;
		if(ConvertTemp(sms_text, t))
		{
			htr.setMaxRoomTemp(t);
			char txt[14];
			sprintf(txt, "%s%i", "TEMP SET ", t);
			SendTempOrDelta(txt);
		}
		#ifdef _TRACE
		  Serial.println(F("set temp"));
		#endif
		return true;
	}
	if (strcasestr(sms_text, cmd_set_delta) != NULL)
	{
		  int t = 0;
		  if (ConvertTemp(sms_text, t))
		  {
			  htr.setDeltaRoomTemp(t);
			  char txt[14];
			  sprintf(txt, "%s%i", "DELTA SET ", t);
			  SendTempOrDelta(txt);
		  }
		  #ifdef _TRACE
		    Serial.println(F("set delta temp"));
		  #endif
		  return true;
	 }
     return false; //unknown command
}
void VGSM::Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htr1flag, boolean htr2flag, boolean htr3flag)
{
	if (gsm_started)
	{
		char sms[80];
		const char bt[]     PROGMEM = { "STATUS\nBOX TEMP  = " };
		const char rt[]     PROGMEM = { "\nROOM TEMP = " };
		const char holl[]   PROGMEM = { "HOLL  " };
		const char water[]  PROGMEM = { "WATER " };
		const char irr[]    PROGMEM = { "IRR   " };
		const char htr1[]   PROGMEM = { "HEAT 1  " };
		const char htr2[]   PROGMEM = { "HEAT 1  " };
		const char htr3[]   PROGMEM = { "HEAT 1  " };
		const char on[]     PROGMEM = { "ON" };
		const char off[]    PROGMEM = { "OFF" };

		const char sn[]    PROGMEM = { "\n" };
		char str_bt[6];
		char str_rt[6];
		dtostrf(boxtemp, 4, 2, str_bt);
		dtostrf(roomtemp, 4, 2, str_rt);
		sprintf(sms, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			bt, str_bt, rt, str_rt, sn, holl, hollflag ? on : off, sn, water, wtrflag ? on : off, sn, irr, irrflag ? on : off, sn,
			htr1, htr1flag ? on : off, sn, htr2, htr2flag ? on : off, sn, htr3, htr3flag ? on : off, "\0");
		#ifdef _TRACE
			Serial.println(sms);
		#endif
		if (SendSMS(PHONENUM, sms)&& SendSMS(PHONENUM2, sms))
		{
			#ifdef _TRACE
				Serial.println(F("\nSMS sent OK"));
			#endif
		}
	}
} 
boolean VGSM::SendMoveAlarmSMS()
{
	if(gsm_started) 
	{
		if (SendSMS(PHONENUM, "Move sensor alarm!")&& SendSMS(PHONENUM2, "Move sensor alarm!"))
		{
			#ifdef _TRACE
			Serial.println(F("Alarm SMS sent OK"));
			#endif
			return true;
		} 
	}
	return false;
} 
