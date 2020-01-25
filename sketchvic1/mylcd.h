#include "def.h"
#include <arduino.h>

#ifndef VIC_LCD
#define VIC_LCD
#include <Wire.h>
// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80
// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00
// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00
#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit
//print
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

// класс управления дисплеем
class MYLCD {
private:
  
  void send(uint8_t, uint8_t);
  void write4bits(uint8_t);
  void expanderWrite(uint8_t);
  void pulseEnable(uint8_t);
  //PRINT
  size_t printFloat(double, uint8_t);
  size_t printNumber(unsigned long, uint8_t);

  uint8_t _Addr;
  uint8_t _displayfunction;
  uint8_t _displaycontrol;
  uint8_t _displaymode;
  uint8_t _numlines;
  uint8_t _cols;
  uint8_t _rows;
  uint8_t _backlightval;

public:
  MYLCD(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows); //задаем адрес LCD экрана 0x27, 16 символов, 2 строки
  ~MYLCD(){};
  size_t write(const uint8_t *buffer, size_t size);
  size_t write(const char *str) {
	  if (str == NULL) return 0;
	  return write((const uint8_t *)str, strlen(str));
  }
  size_t write(const char *buffer, size_t size) {
	  return write((const uint8_t *)buffer, size);
  }
  size_t write(uint8_t);
  void init();
  void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
  void clear();
  void home();
  void command(uint8_t);
  void display();
  void backlight();
  void setCursor(uint8_t, uint8_t);
//PRINT
  size_t print(const __FlashStringHelper *);
  size_t print(double, int = 2);
  size_t print(const char[]);
  size_t print(char);
  size_t print(long, int = DEC);
  size_t print(unsigned int, int = DEC);
  size_t print(unsigned long, int = DEC);
  size_t print(int, int = DEC);

  void Init();
  void Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htrflag, boolean gsmflag);
  void gsmInit();
  void gsmFail();
  void gsmInitSMS();
};
#endif
