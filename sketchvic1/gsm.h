#include "def.h"
#include <arduino.h>
#include <SoftwareSerial.h>


#ifndef VIC_GSM1
#define VIC_GSM1

#include "heat.h"
#include "holl.h"
#include "water.h"
#include "irr.h"
#include "mylcd.h"

#define GSM_ON              8 // пин для включение GSM модуля 
#define GSM_RESET           9 // сбрасывает GSM модуль

// some constants for the IsRxFinished() method
#define RX_NOT_STARTED      0
#define RX_ALREADY_STARTED  1

// some constants for the InitParam() method
#define PARAM_SET_0   0
#define PARAM_SET_1   1

// Common string used
#define str_ok 		"OK"			//string to reduce stack usage
#define str_at		"AT"			//string to reduce stack usage

#define COMM_BUF_LEN        200 // буфер для обмена с модемом

// SMS type
// use by method IsSMSPresent()
enum sms_type_enum {
     SMS_UNREAD,
     SMS_READ,
     SMS_ALL,

     SMS_LAST_ITEM
};

enum comm_line_status_enum {
     // CLS like CommunicationLineStatus
     CLS_FREE,   // линия свободна и может использоваться для коммуникации line is free - not used by the communication and can be used
     CLS_ATCMD,  // линия занята передачей команды, включая ожидание ответа на команду line is used by AT commands, includes also time for response
     CLS_DATA,   // for the future - line is used in the CSD or GPRS communication
     CLS_LAST_ITEM
};

enum rx_state_enum {
     RX_NOT_FINISHED = 0,      // not finished yet
     RX_FINISHED,              // finished, some character was received
     RX_FINISHED_STR_RECV,     // finished and expected string received
     RX_FINISHED_STR_NOT_RECV, // finished, but expected string not received
     RX_TMOUT_ERR,             // finished, no character received
     // initial communication tmout occurred
     RX_LAST_ITEM
};
enum at_resp_enum {
     AT_RESP_ERR_NO_RESP = -1,   // nothing received
     AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
     AT_RESP_OK = 1,             // response_string was included in the response

     AT_RESP_LAST_ITEM
};
enum getsms_ret_val_enum {
     GETSMS_NO_SMS   = 0,
     GETSMS_UNREAD_SMS,
     GETSMS_READ_SMS,
     GETSMS_OTHER_SMS,

     GETSMS_NOT_AUTH_SMS,
     GETSMS_AUTH_SMS,

     GETSMS_LAST_ITEM
};
/*раскоментирую, когда уберу ссылку на сим 900
*/

// класс управления gsm
class VGSM {
  enum GSM_st_e { ERROR, IDLE, READY, ATTACHED, TCPSERVERWAIT, TCPCONNECTEDSERVER, TCPCONNECTEDCLIENT }; // статусы состояния модуля
private:
  SoftwareSerial _cell = SoftwareSerial(2, 3); // открываем работу с gsm модулем через программный эмулятор сериал через пин 2 и 3
  int _status;
  byte comm_line_status; //статус состояния порта 
  byte comm_buf[COMM_BUF_LEN+1]; // выделяем память для обмена с портом 
  byte *p_comm_buf; // указатель на позицию внутри буфера
  byte comm_buf_len; // длинна реально заполненных данных в буфере
  boolean gsm_started=false;
  
  byte rx_state;                  // internal state of rx state machine внутреннее состояние канала приема данных из порта
  uint16_t start_reception_tmout; // max tmout for starting reception время ожидания ответной команды из порта после отправки туда ат комманды
  uint16_t interchar_tmout;       // время ожидания после чтения очередного символа до прихода нового символа
  unsigned long prev_time;        // счетчик замера времени ожидания прихода данных в порту
  /**/
  void RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout); // инициализирует счетчики и указатели перед началом чтения из буфера порта
  byte IsRxFinished(void);
  byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout);
  byte WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, char const *expected_resp_string);
  byte IsStringReceived(char const *compare_string);
  char SendATCmdWaitResp(char const *AT_cmd_string, uint16_t start_comm_tmout, uint16_t max_interchar_tmout, char const *response_string, byte no_of_attempts);
  void InitParam(byte group);
  void Echo(byte state);
  char InitSMSMemory(void);
  
  inline void SetCommLineStatus(byte new_status) { comm_line_status = new_status;};
  inline byte GetCommLineStatus(void) {return comm_line_status;};
  inline void setStatus(GSM_st_e status) {_status = status;};
  inline int getStatus() { return _status;};
  boolean ConvertTemp(char * command, int &temp);
  char* DeleteSpace(char* s);
  boolean SendTempOrDelta(char * txt);
public:
  VGSM(){};
  ~VGSM(){};
  boolean Init(MYLCD& lcd);
  boolean ModemIsAlive(); //вызываем отправку команды АТ и ожидаем приема 
  boolean SendInitSMS();
  boolean SendHelp();
  
  virtual int begin(long baud_rate);
  byte SendSMS(char const *number_str, char const *message_str);  
  byte DeleteSMS(byte position);
  byte IsSMSPresent(byte required_status);
  byte GetSMS(byte position, char *phone_number, char *SMS_text, byte max_SMS_len);
  inline boolean getStarted(){ return gsm_started;};
  boolean CheckSMSCommand(Heater& htr, Refrigerator& holl, Water& wtr, Irrigation& irr);
  void Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htr1flag, boolean htr2flag, boolean htr3flag); 
  boolean SendMoveAlarmSMS();
};
#endif
