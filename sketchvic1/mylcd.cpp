#include "mylcd.h"

#define printIIC(args)	Wire.write(args)

inline size_t MYLCD::write(uint8_t value) {
  send(value, Rs);
  return 1;
}
/************ low level data pushing commands **********/

// write either command or data
void MYLCD::send(uint8_t value, uint8_t mode) {
  uint8_t highnib = value & 0xf0;
  uint8_t lownib = (value << 4) & 0xf0;
  write4bits((highnib) | mode);
  write4bits((lownib) | mode);
}
void MYLCD::write4bits(uint8_t value) {
  expanderWrite(value);
  pulseEnable(value);
}
void MYLCD::expanderWrite(uint8_t _data) {
  Wire.beginTransmission(_Addr);
  printIIC((int)(_data) | _backlightval);
  Wire.endTransmission();
}
void MYLCD::pulseEnable(uint8_t _data) {
  expanderWrite(_data | En);	// En high
  delayMicroseconds(1);		// enable pulse must be >450ns

  expanderWrite(_data & ~En);	// En low
  delayMicroseconds(50);		// commands need > 37us to settle
}

MYLCD::MYLCD(uint8_t lcd_Addr, uint8_t lcd_cols, uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _backlightval = LCD_NOBACKLIGHT;
}
void MYLCD::init()
{
  Wire.begin();
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  begin(_cols, _rows);
}
void MYLCD::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != 0) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delay(50);

  // Now we pull both RS and R/W low to begin commands
  expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
  delay(1000);

  //put the LCD into 4 bit mode
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03 << 4);
  delayMicroseconds(4500); // wait min 4.1ms

  // second try
  write4bits(0x03 << 4);
  delayMicroseconds(4500); // wait min 4.1ms

  // third go!
  write4bits(0x03 << 4);
  delayMicroseconds(150);

  // finally, set to 4-bit interface
  write4bits(0x02 << 4);


  // set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for roman languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  home();

}
/********** high level commands, for the user! */
void MYLCD::clear() {
  command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void MYLCD::home() {
  command(LCD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}
/*********** mid level commands, for sending data/cmds */

inline void MYLCD::command(uint8_t value) {
  send(value, 0);
}
void MYLCD::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void MYLCD::backlight(void) {
  _backlightval = LCD_BACKLIGHT;
  expanderWrite(0);
}

void MYLCD::setCursor(uint8_t col, uint8_t row) {
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if (row > _numlines) {
    row = _numlines - 1;    // we count rows starting w/0
  }
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}
//PRINT
size_t MYLCD::print(const __FlashStringHelper *ifsh)
{
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    if (write(c)) n++;
    else break;
  }
  return n;
}
size_t MYLCD::print(const char str[])
{
  return write(str);
}

size_t MYLCD::print(char c)
{
  return write(c);
}
size_t MYLCD::print(long n, int base)
{
  if (base == 0) {
    return write(n);
  }
  else if (base == 10) {
    if (n < 0) {
      int t = print('-');
      n = -n;
      return printNumber(n, 10) + t;
    }
    return printNumber(n, 10);
  }
  else {
    return printNumber(n, base);
  }
}
size_t MYLCD::printNumber(unsigned long n, uint8_t base)
{
  char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
  char *str = &buf[sizeof(buf) - 1];

  *str = '\0';

  // prevent crash if called with base == 1
  if (base < 2) base = 10;

  do {
    char c = n % base;
    n /= base;

    *--str = c < 10 ? c + '0' : c + 'A' - 10;
  } while (n);

  return write(str);
}

size_t MYLCD::printFloat(double number, uint8_t digits)
{
  size_t n = 0;

  if (isnan(number)) return print("nan");
  if (isinf(number)) return print("inf");
  if (number > 4294967040.0) return print("ovf");  // constant determined empirically
  if (number < -4294967040.0) return print("ovf"); // constant determined empirically

  // Handle negative numbers
  if (number < 0.0)
  {
    n += print('-');
    number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i = 0; i < digits; ++i)
    rounding /= 10.0;

  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  n += print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0) {
    n += print(".");
  }

  // Extract digits from the remainder one at a time
  while (digits-- > 0)
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    n += print(toPrint);
    remainder -= toPrint;
  }

  return n;
}
size_t MYLCD::print(double n, int digits)
{
  return printFloat(n, digits);
}
size_t MYLCD::print(unsigned int n, int base)
{
  return print((unsigned long)n, base);
}
size_t MYLCD::print(unsigned long n, int base)
{
  if (base == 0) return write(n);
  else return printNumber(n, base);
}
size_t MYLCD::print(int n, int base)
{
  return print((long)n, base);
}
size_t MYLCD::write(const uint8_t *buffer, size_t size)
{
  size_t n = 0;
  while (size--) {
    if (write(*buffer++)) n++;
    else break;
  }
  return n;
}

void MYLCD::Init()
{
  init(); // Инициализируем экран
  backlight(); //включаем подсветку
  setCursor(0, 0); //Устанавливаем позицию начиная с которой выводится текст.
  print("DISPLAY INIT"); //выводим строку 1
#ifdef _TRACE
  Serial.println(F("display init"));
#endif
}
//выводит на дисплей информационное состояние показателей системы
void MYLCD::Status(double boxtemp, double roomtemp, boolean hollflag, boolean wtrflag, boolean irrflag, boolean htrflag, boolean gsmflag)
{
  setCursor(0, 0); // устанавливаем курсор в начало дисплея
  setCursor(0, 0);
  print(F("TB"));
  print(boxtemp);
  print(F(" TR"));
  print(roomtemp);

  setCursor(0, 1);
  print(F("HL"));
  print(hollflag ? F("+") : F("_"));
  print(F("WT"));
  print(wtrflag ? F("+") : F("_"));
  print(F("IR"));
  print(irrflag ? F("+") : F("_"));
  print(F("HT"));
  print(htrflag ? F("+") : F("_"));
  print(F("GSM"));
  print(gsmflag ? F("+") : F("_"));
}
void MYLCD::gsmInit()
{
  setCursor(0, 1);
  print(F("GSM INIT"));
}
void MYLCD::gsmFail()
{
  setCursor(0, 1);
  print(F("GSM FAIL"));
}
void MYLCD::gsmInitSMS()
{
  clear();
  setCursor(0, 0);
  print(F("GSM INIT"));
  setCursor(0, 1);
  print(F("SMS INIT SEND"));
  Serial.println(F("\nLCD DOne"));
}
void MYLCD::clearStr(int n) {
    setCursor(0, n);//устанавливаем курсор на вторую строку дисплея
    for (int i = 0; i < 20; i++) write(' ');
}
void MYLCD::log(const __FlashStringHelper* ifsh)
{
    clearStr(3);
    setCursor(0, 3);
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    size_t n = 0;
    while (1) {
        unsigned char c = pgm_read_byte(p++);
        if (c == 0) break;
        if (write(c)) n++;
        else break;
        if (n >= 20) break;
    }
}
void MYLCD::log(int n, int base)
{
    clearStr(3);
    setCursor(0, 3);//устанавливаем курсор на вторую строку дисплея
    print(n, base);
}
void MYLCD::log(const char str[])
{
    clearStr(3);
    const char* p = str;;
    int i = 0;
    setCursor(0, 3);//устанавливаем курсор на вторую строку дисплея
    while ((*p != '\0') && (i < 20)) { // пока значение в строке не стало 0 или на экран не вывели 20 символов
        if ((*p != '\r') && (*p != '\n')) { //если строка не перевод или возврат то символ выводим иначе не выводим
            i++;
            write(*p);
        }
        p++;
    }
}

