#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "Waveshare4InchTftShield.h"
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoRS485.h> // the ArduinoDMX library depends on ArduinoRS485
#include "ArduinoDMX.h"
Adafruit_NeoPixel led = Adafruit_NeoPixel(23, A2, NEO_GRB + NEO_KHZ800);
#define sensitiv_minus A3 
#define sensitiv_plus A4
#define freezone 26
#define blackout 24
#define manual 22
bool _freezone = false;
bool _blackout = false;
//Stałe kolory
#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
///////////////////////
bool havePrev = false;
uint8_t IteratorConfig = 0;
String QuestionString[9];
uint16_t answerUser[9];
uint16_t maxFilds[9];
uint8_t _device;
bool saveDevice = false;
String menuOperation[5];
uint8_t opIterator = 0;
String textDrawDevicesFilds[] = {"1", "2", "3", "4", "5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31","32"};
uint16_t redGlobalChannel[32];
uint16_t greenGlobalChannel[32];
uint16_t blueGlobalChannel[32];
uint16_t panGlobalChannel[32];
uint16_t tilGlobalChannel[32];
uint16_t goboGlobalChannel[32];
uint16_t shutterGlobalChannel[32];
struct deviceArray{
  uint16_t startAddress;
  uint8_t totalChannel;
  uint8_t shutterChannel;
  uint8_t redChannel;
  uint8_t greenChannel;
  uint8_t blueChannel;
  uint8_t panChannel;
  uint8_t tilChannel;
  uint8_t goboChannel;
}Devices[32];

//*******************************************************
#define MAX_Y 320       //Maksymalny rozmiar po Y w px  *
#define MAX_X 480       //Maksymalny rozmiar po X w px  *
//*******************************************************
#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])
template<typename T, size_t N> size_t ArraySize(T(&)[N]) {
  return N;
}
namespace
{
    Waveshare4InchTftShield Waveshield;
}

uint16_t returnStartPoint(uint16_t _size, uint8_t amountFilds, uint8_t space, bool X)
{
  uint16_t MAX = 0;
  if (X) MAX = MAX_X;
  else MAX = MAX_Y;
  return (MAX - (( _size * amountFilds) + (space * (amountFilds - 1 )))) / 2;
}

uint16_t returnIterationPoint(uint16_t _size, uint16_t point, uint8_t iteration, uint8_t space)
{
  return point + (_size * iteration) + (space * iteration);
}

void drawWindow(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY)
{
  Waveshield.fillRect(startX, startY, endX, endY, BLUE);
}
/*
 FUNKCJA RYSUJACA POLE DO WPROWADZENIA DANYCH
 amountFilds - Ilość pul do narysowania
 _size - romziar jednego pola
 */
void drawInputPlace(uint8_t amountFilds, uint8_t _size)
{
  uint16_t startXFirstPlace = returnStartPoint(_size, amountFilds, 5, true);
  uint16_t startYPosition = returnIterationPoint(_size, MAX_Y-50-_size, 0, 5);
  for(uint8_t i = 0; i < amountFilds; i++)
  {
    if(i==0)
      drawWindow(startXFirstPlace, startYPosition, startXFirstPlace+_size, startYPosition+_size);
     else
     {
        uint16_t XPlacePosition = returnIterationPoint(_size, startXFirstPlace, i, 5);
        drawWindow(XPlacePosition, startYPosition, XPlacePosition+_size, startYPosition+_size);
     }
  }
}

uint16_t yFont(uint16_t windowY, uint8_t width, uint8_t sizeFontY)
{
  return ((width - sizeFontY) / 2) + windowY;
}
void drawCommand(String command)
{
  setText(command, 0, 5, MAX_X, 7);
}
/*
  Funkcja obliczajaca srodek tekstu po pozycji X
  @PARM:
  startX - poczatek obszaru wysrodkowania
  endX - koniec obszaru wysrodkowania
  length - ilosc wyswietlanych znakow
  sizeFontX - rozmiar czcionki w pozycji X uzywanej do pisania
  {Przyklad
    Font24 - rozmiar 17 etc.
  }
  return - zwraca srodek w jakim nalezy pisac czcionke w pozycji X
*/
uint16_t xFont(uint16_t windowX, uint16_t height, uint8_t lengthWord, uint8_t sizeFontX)
{
  return ((height - (lengthWord * sizeFontX)) / 2) + windowX;
}

void setText(String text, uint16_t windowX, uint16_t windowY, uint16_t height, uint16_t width)
{
  uint16_t startX = xFont(windowX, height, text.length(), 11);
  uint16_t startY = yFont(windowY, width, 4);
  char *_text = text.c_str();
  Waveshield.setCursor(startX,startY);
  Waveshield.print(_text);
}

uint8_t returnNavigationDifference(uint8_t widthButton, uint8_t amountFilds)
{
  return widthButton / amountFilds;
}

uint8_t returnInputPlace(uint8_t inputPlace, uint8_t amountFilds)
{
  return inputPlace/amountFilds;
}

void drawNavigation()
{
  drawWindow(5, MAX_Y - 45, 5 + 80, MAX_Y - 5 ); //Wstecz
  setText("WSTECZ", 5, MAX_Y - 45, 80, 40);
  drawWindow(MAX_X - 85, MAX_Y - 45, MAX_X - 5, MAX_Y - 5); //OK
  setText("OK", MAX_X - 85 , MAX_Y - 45, 80, 40);
}


void drawFilds(uint16_t amountFildsX, uint16_t amountFildsY, uint16_t height, uint16_t width, uint16_t spaceX, uint16_t spaceY, bool navigationButtons, uint8_t inputPlace = 0, uint8_t topStart=0)
{
  uint16_t pointX = returnStartPoint(height, amountFildsX, spaceX, true);
  uint16_t pointY = topStart==0?returnStartPoint(width, amountFildsY, spaceY, false):topStart;
  if (navigationButtons)
  {
    width = width - returnNavigationDifference(50, amountFildsY) - returnInputPlace(inputPlace, amountFildsY);
  }
  for (uint8_t i = 0; i < amountFildsX; i++)
  {
    uint16_t _pointX = returnIterationPoint(height, pointX, i, spaceX);
    for (uint8_t j = 0; j < amountFildsY; j++)
    {
      uint16_t _pointY = returnIterationPoint(width, pointY, j, spaceY);
      drawWindow(_pointX, _pointY,height,width);
    }
  }
}

void drawVerticalMasterMenu(uint8_t sizeArray, String textDisplay[], uint16_t height, bool navigationButtons, uint8_t inputPlace = 0)
{
  havePrev = false;
  Waveshield.fillScreen(BLACK);
  uint16_t widthRows = (MAX_Y / sizeArray) - 15;
  drawFilds(1, sizeArray, height, widthRows, 15, 10, navigationButtons, inputPlace);
  uint16_t sizeX = returnStartPoint(height, 1, 15, true);
  uint16_t sizeY = returnStartPoint(widthRows, sizeArray, 10, false);
  if (navigationButtons) {
    widthRows = widthRows - returnNavigationDifference(50, sizeArray) - returnInputPlace(inputPlace, sizeArray);
  }
  for (uint16_t i = 0; i < sizeArray; i++)
  {
    uint16_t _sizeX = 0;
    uint16_t _sizeY = 0;
    if (i == 0)
    {
      setText(textDisplay[i], sizeX, sizeY, height, widthRows);
    }
    else
    {
      _sizeX = returnIterationPoint(height, sizeX, 0, 15);
      _sizeY = returnIterationPoint(widthRows, sizeY, i, 10);
      setText(textDisplay[i], _sizeX, _sizeY, height, widthRows);
    }

  }
  if (navigationButtons)
  {
    drawNavigation();
    havePrev = true;
  }
}

void drawFildsText(String textDisplay[], uint16_t amountFildsX, uint16_t amountFildsY, uint16_t height, uint16_t width, uint16_t spaceX, uint16_t spaceY, bool navigationButtons, uint8_t inputPlace = 0, uint8_t topStart=0)
{
  uint16_t pointX = returnStartPoint(height, amountFildsX, spaceX, true);
  uint16_t pointY = topStart==0?returnStartPoint(width, amountFildsY, spaceY, false):topStart;
  if (navigationButtons)
  {
    width = width - returnNavigationDifference(50, amountFildsY) - returnInputPlace(inputPlace, amountFildsY);
  }
  uint8_t temp = 0;
  for (uint8_t i = 0; i < amountFildsX; i++)
  {
    uint16_t _pointX = returnIterationPoint(height, pointX, i, spaceX);
    for (uint8_t j = 0; j < amountFildsY; j++)
    {
      uint16_t _pointY = returnIterationPoint(width, pointY, j, spaceY);
      setText(textDisplay[temp],_pointX, _pointY,height,width);
      temp++;
    }
  }
}


uint16_t returnClick(uint16_t amountFildsX, uint16_t amountFildsY, uint16_t height, uint16_t width, uint16_t spaceX, uint16_t spaceY, bool navigationButtons, uint16_t xPosition, uint16_t yPosition, uint8_t inputPlace = 0, uint8_t topStart=0)
{
  uint16_t pointX = returnStartPoint(height, amountFildsX, spaceX, true);
  uint16_t pointY = topStart==0?returnStartPoint(width, amountFildsY, spaceY, false):topStart;
  if (navigationButtons)
  {
    width = width - returnNavigationDifference(50, amountFildsY) - returnInputPlace(inputPlace, amountFildsY);
  }
  uint8_t temp = 1;
  for (uint8_t i = 0; i < amountFildsX; i++)
  {
    uint16_t _pointX = returnIterationPoint(height, pointX, i, spaceX);
    for (uint8_t j = 0; j < amountFildsY; j++)
    {
      uint16_t _pointY = returnIterationPoint(width, pointY, j, spaceY);
      if(xPosition >= _pointX && xPosition <= (_pointX+height))
      {
        if(yPosition >= _pointY && yPosition <= (_pointY+width))
        {
          return temp;
        }
      }
      temp++;
    }
  }
  return 513;
}



void mainMenu()
{
  String text[] = {"START", "KONFIGURUJ", "OPCJE"};
  
  drawVerticalMasterMenu(ARRAY_SIZE(text), text, 200, false, 40);
}

void startMenu()
{
  String text[] = {"AUTOMATYCZNY", "NA PULAP", "RECZNY"};
  drawVerticalMasterMenu(ARRAY_SIZE(text), text, 200, true, 40);
}
void drawArrow()
{
  drawWindow(155, MAX_Y - 45, 80, MAX_Y - 5 ); //Left
  setText("<", 155, MAX_Y - 45, 80, 40);
  drawWindow(245, MAX_Y - 45, 80, MAX_Y - 5); //Right
  setText(">", 245 , MAX_Y - 45, 80, 40);
}

String whatArrowClick(uint16_t x, uint16_t y )
{
  if(x>=155 && x <= 235 && y >= (MAX_Y - 45) && y <(MAX_Y-5))
    return "left";
  if(x>=245 && x <= 325 && y >= (MAX_Y - 45) && y <(MAX_Y-5))
    return "right";
  return "";
}

void drawDevicesFilds()
{
  Waveshield.fillScreen(BLACK);
  drawCommand("Wybierz urzadzenie");
  drawFilds(8, 4, 50, 65, 5, 5, true, 0, 40);
  drawFildsText(textDrawDevicesFilds, 8, 4, 50, 65, 5, 5, true, 0, 40);
  drawNavigation();
  havePrev = true;
}
String digit[] = {"0", "1", "2", "3", "4", "5", "6", "7","8","9"};
void drawKeyboard()
{
  drawFilds(5, 2, 75, 110, 5, 5, true, 30, 0);
  drawFildsText(digit,5, 2, 75, 110, 5, 5, true, 30, 0);
}

void drawThreeDigit(String text[], uint8_t amountFilds)
{
  drawFilds(amountFilds, 1, 35, 100, 5, 5, true, 0, 210);
  drawFildsText(text,amountFilds, 1, 35, 100, 5, 5, true, 0, 210);
}

void drawQuestionForUsers(String question, uint8_t amountFilds, uint8_t deviceNumber)
{
  Waveshield.fillScreen(BLACK);
  if(amountFilds == 3)
  {
    String text[] = {"0", "0", "0"};
    drawThreeDigit(text, amountFilds);
  }else
  {
    String text[] = {"0", "0"};
    drawThreeDigit(text, amountFilds);
  }
  drawCommand(question);
  setText(textDrawDevicesFilds[deviceNumber-1], 0, 0, 25, 25);
  drawArrow();
  drawNavigation();
  havePrev = true;
  drawKeyboard();
}

bool isBack(int x, int y)
{
  if( (x >= 5 && x <=85) && (y >= (MAX_Y - 45) && y <= (MAX_Y-5)))
    return true;
  return false;
}

bool isOk(int x, int y)
{
    if( (x >= (MAX_X-85) && x <=(MAX_X-5)) && (y >= (MAX_Y - 45) && y <= (MAX_Y-5)))
    return true;
  return false;
}

int convertToInt(String number[], uint8_t _length)
{
  int value = 0;
  uint8_t base = 1;
  for(int8_t i = _length-1; i >=0 ; i--)
  {
    value = value + (base * (number[i][0] - '0' ));
    base = base * 10;
  }
  return value;
}

uint8_t countDigit(uint16_t n) 
{ 
    int count = 0; 
    while (n != 0) { 
        n = n / 10; 
        ++count; 
    } 
    return count; 
} 

bool checkRecurrence(uint16_t values)//Sprawdz powtarzenie sie kanalow
{
  for(int8_t i = IteratorConfig-1; i > 1; i--)
  {
     if(answerUser[i] == values && values != 0) return false;
  }
  return true;
}

bool correctValue(String adress[], int correctValue)
{
  int intValues = convertToInt(adress, countDigit(correctValue));
  if(IteratorConfig <=1)
  {
    if(intValues == 0) return false;
  }
  if(IteratorConfig > 1){
    if(!checkRecurrence(intValues)) return false;
    if(answerUser[1]<intValues) return false;
  }
  if(intValues >= correctValue) return false;
  return true;
}

void drawErrorInformation(String information)
{
  Waveshield.fillScreen(BLACK);
  drawCommand(information);
  drawNavigation();
  havePrev = true;
}

void optionsMenu()
{
  String text[] = {"ZAPIS PAMIECI", "ODCZYT PAMIECI", "INFORMACJE"};
  drawVerticalMasterMenu(ARRAY_SIZE(text), text, 200, true, 40);
}

void configInit()
{
  QuestionString[0] = "Podaj adres poczatkowy: ";
  QuestionString[1] = "Podaj ilosc kanalow: ";
  QuestionString[2] = "Podaj kanal otwarcia lampy: ";
  QuestionString[3] = "Podaj kanal R: ";
  QuestionString[4] = "Podaj kanal G: ";
  QuestionString[5] = "Podaj kanal B: ";
  QuestionString[6] = "Podaj kanal Pan: ";
  QuestionString[7] = "Podaj kanal Til: ";
  QuestionString[8] = "Podaj kanal Gobo: ";
  maxFilds[0] = 512;
  maxFilds[1] = 16;
  maxFilds[2] = 16;
  maxFilds[3] = 16;
  maxFilds[4] = 16;
  maxFilds[5] = 16;
  maxFilds[6] = 16;
  maxFilds[7] = 16;
  maxFilds[8] = 16;
}

void callFunction(String funcName)
{
    if( funcName == "startMenu" )
      startMenu();
    else if( funcName == "mainMenu" )
      mainMenu();
    else if( funcName == "drawDevicesFilds")
      drawDevicesFilds();
    else if( funcName == "optionsMenu")
      optionsMenu();
    else if( funcName == "drawQuestionForUsers" ){
      drawQuestionForUsers(QuestionString[IteratorConfig], countDigit(maxFilds[IteratorConfig]), _device);
    }
      
}

void cleanAfterError()
{
  for(uint8_t i = 0; i <= 8; i++)
  {
    answerUser[i] = 0;
  }
  IteratorConfig = 0;
  _device = "";
}

void back()
{
  if(opIterator != 0)
      {
        menuOperation[opIterator] = "";
        opIterator--;
        callFunction(menuOperation[opIterator]);
      }
}

void saveDeviceToArray()
{
  if(saveDevice)
  {
    if(_device != 0)
    {
      Devices[_device-1].startAddress = answerUser[0];
      Devices[_device-1].totalChannel = answerUser[1];
      Devices[_device-1].shutterChannel = answerUser[2];
      Devices[_device-1].redChannel = answerUser[3];
      Devices[_device-1].greenChannel = answerUser[4];
      Devices[_device-1].blueChannel = answerUser[5];
      Devices[_device-1].panChannel = answerUser[6];
      Devices[_device-1].tilChannel = answerUser[7];
      Devices[_device-1].goboChannel = answerUser[8];
    }
  }
}
int16_t noiceValue = 0;
double avg = 0;
uint8_t avgTemp = 1;

void setLevelNoice(uint8_t level)
{
  for(uint8_t i = 0; i < 10; i++)
  {
    led.setPixelColor(i, led.Color(0,0,0));
    if(i < level) led.setPixelColor(i, led.Color(0,255,0));
  }
  led.show();
}

void clearLed()
{
  for(uint8_t i =0; i < 23; i++)
  {
    led.setPixelColor(i, led.Color(0,0,0));
  }
  led.show();
}

void setLevelResistor(uint8_t level)
{
  for(uint8_t i = 10; i < 20; i++)
  {
    led.setPixelColor(i, led.Color(0,0,0));
    if(i < level+10) led.setPixelColor(i, led.Color(0,255,0));
  }
  led.show();
}

void setSignalLevel()
{
  led.setPixelColor(22, led.Color(255, 0, 0));
  led.show();
}
void unsetSignalLevel()
{
  led.setPixelColor(22, led.Color(0, 0, 0));
  led.show();
}

void setShutter(uint8_t value)
{
  for(uint8_t i = 0; i < 32; i++)
  {
    if(shutterGlobalChannel[i]!=0){
      DMX.write(shutterGlobalChannel[i], value);
      Serial.println(shutterGlobalChannel[i]);
    }
  }
}

void sendSignalToDevices()
{
  DMX.beginTransmission();
  if(_blackout) setShutter(0);
  else setShutter(255);
  if(_freezone) {
    DMX.endTransmission();
    return;
  }
  
  uint8_t red = random(0, 255);
  uint8_t green = random(0, 255);
  uint8_t blue = random(0, 255);

  uint8_t tilt = random(0, 255);
  uint8_t pan = random(0, 255);
  uint8_t gobo = random(0, 255);
  for(uint8_t i = 0; i < 32; i++)
  {
    if(redGlobalChannel[i]!=0) DMX.write(redGlobalChannel[i], red);
    if(greenGlobalChannel[i]!=0) DMX.write(greenGlobalChannel[i], green);
    if(blueGlobalChannel[i]!=0) DMX.write(blueGlobalChannel[i], blue);
    if(panGlobalChannel[i]!=0) DMX.write(panGlobalChannel[i], pan);
    if(tilGlobalChannel[i]!=0) DMX.write(tilGlobalChannel[i], tilt);
    if(goboGlobalChannel[i]!=0) DMX.write(goboGlobalChannel[i], gobo);
  }
  DMX.endTransmission();
}

double mainAvg = 0.0;
uint8_t stepInOnce = 0;
void controllStart(String _typeControl, TSPoint _click)
{
 while(1){ 
  uint16_t sens_down = analogRead(sensitiv_minus);
  uint16_t sens_up = analogRead(sensitiv_plus);
  int16_t _sens_down = map(sens_down, 0, 1000, -30, 30);
  int16_t _sens_up = map(sens_up, 0, 1000, -50, 20);
  if(_freezone) led.setPixelColor(21, led.Color(0,0,255));
  else led.setPixelColor(21, led.Color(0,0,0));
  if(_blackout) led.setPixelColor(20, led.Color(0,0,255));
  else led.setPixelColor(20, led.Color(0,0,0));
  if(digitalRead(freezone) == HIGH){
    _freezone = !_freezone;
    delay(250);}
  if(digitalRead(blackout) == HIGH){
    _blackout = !_blackout;
    delay(250);}
  _click = Waveshield.getPoint();
  int resistor = analogRead(A1);
  int _resistor = map(resistor, 0, 1000, 0, 10);
  Waveshield.normalizeTsPoint(_click);
  static int lasEcmLevel = 0;
  int input = analogRead(A0);
  input = abs(input - (512+_sens_up));
  int ecmLevel = map(input, 30+_sens_down, 360+_sens_up, 0, 10);
  if(ecmLevel > lasEcmLevel)
  lasEcmLevel++;
  else if(ecmLevel < lasEcmLevel)
  lasEcmLevel--;
  lasEcmLevel = ecmLevel;
  setLevelNoice(lasEcmLevel);
  if (_click.z > MINPRESSURE && _click.z < MAXPRESSURE){
        break;
  }
  if(_typeControl=="AUTOMATYCZNY"){
    if(avgTemp == 10){
      avgTemp = 1;
      avg = avg / 10;
      mainAvg = avg;
      avg = 0;
    }
    if(lasEcmLevel >= mainAvg+2) stepInOnce ++;
    if(stepInOnce == 5) stepInOnce = 0;
    if(lasEcmLevel >= mainAvg+2 && stepInOnce == 0)
      {
        setSignalLevel(); //REAKCJA DMX DODATKOWO
        sendSignalToDevices();
        delay(75);
      }
      else
        unsetSignalLevel();
    avg = avg + lasEcmLevel;
    avgTemp++;
  }
  if(_typeControl=="NA PULAP"){
    setLevelResistor(_resistor);
    if(lasEcmLevel >= _resistor) stepInOnce ++;
    if(stepInOnce == 3) stepInOnce = 0;
    if(lasEcmLevel >= _resistor && stepInOnce == 0)
    {
      setSignalLevel(); //REAKCJA DMX DODATKOWO
      sendSignalToDevices();
      delay(75);
    }
    else
      unsetSignalLevel();
  }
  if(_typeControl=="RECZNY"){
    if(digitalRead(manual) == HIGH)
    {
      setSignalLevel();
      sendSignalToDevices();
      delay(200);
    }
    else
      unsetSignalLevel();
  }
  delay(10);
 }
 
}

void supportDrawing(String textinMiddle, String text, TSPoint _click, bool _break)
{
  Waveshield.fillScreen(BLACK);
  Waveshield.setCursor(xFont(0, MAX_X, textinMiddle.length(), 11),0);
  Waveshield.print(textinMiddle);
  Waveshield.setCursor(13,30);
  Waveshield.print(text);
  while(1)
  {
    if(_break) break;
    else{
      controllStart(textinMiddle, _click);
      clearLed();
      break;
    }
     _click = Waveshield.getPoint();
     Waveshield.normalizeTsPoint(_click);
     delay(100);
      if (_click.z > MINPRESSURE && _click.z < MAXPRESSURE){
        break;
      }
  }
}

void cleanEEPROM()
{
for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

void saveToEEPROM()
{
  uint16_t addressInMemory = 0;
  cleanEEPROM();
  for(uint8_t i = 0; i < 32; i++)
  {
    if(Devices[i].startAddress != 0) //Czy jest zapisane urzadznie?
    {
      EEPROM.write(addressInMemory,i);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].startAddress);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].totalChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].shutterChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].redChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].greenChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].blueChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].panChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].tilChannel);
      addressInMemory++;
      EEPROM.write(addressInMemory,Devices[i].goboChannel);
      addressInMemory++;
    }
  }
}

void readFromEEPROM()
{
  uint16_t addressInMemory = 0;
  for(uint8_t i = 0; i < 32; i++)
  {
      uint8_t deviceN = EEPROM.read(addressInMemory);
      addressInMemory++;
      Devices[deviceN].startAddress = EEPROM.read(addressInMemory);
      if(Devices[deviceN].startAddress != 0)
      {
        addressInMemory++;
        Devices[deviceN].totalChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].shutterChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].redChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].greenChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].blueChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].panChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].tilChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
        Devices[deviceN].goboChannel = EEPROM.read(addressInMemory);
        addressInMemory++;
      }else
      {
        addressInMemory+=9;
      }
      
  }
}

void showToSerial()
{
  for(uint8_t i = 0; i < 32; i++)
  {
    if(Devices[i].startAddress != 0)
    {
      Serial.print("Numer urzadzenia: ");
      Serial.print(i+1);
      Serial.print(" Otwarcie lampy: ");
      Serial.print(Devices[i].shutterChannel);
      Serial.print(" R: ");
      Serial.print(Devices[i].redChannel);
      Serial.print(" G: ");
      Serial.print(Devices[i].greenChannel);
      Serial.print(" B: ");
      Serial.print(Devices[i].blueChannel);
      Serial.println(" ");
    }
  }
}

void fillArray()
{
  uint8_t j = 1;
  for(uint8_t i = 0; i < 3; i++)
  {
      Devices[i].startAddress = j;
      j++;
      Devices[i].totalChannel = 16;
      j++;
      Devices[i].shutterChannel = j;
      j++;
      Devices[i].redChannel = j;
      j++;
      Devices[i].greenChannel = j;
      j++;
      Devices[i].blueChannel = j;
      j++;
      Devices[i].panChannel = j;
      j++;
      Devices[i].tilChannel = j;
      j++;
      Devices[i].goboChannel = j;
      j++;
      j=i+2;
  }
}

String whatClickMainMenu(uint16_t x, uint16_t y)
{
  if((x >= 140 && y >= 13) && (x<=340 && y<=104))
  {
    if(menuOperation[opIterator] == "mainMenu") return "START";
    if(menuOperation[opIterator] == "startMenu") return "AUTOMATYCZNY";
    if(menuOperation[opIterator] == "optionsMenu") return "ZAPIS PAMIECI";
  }
  if((x >= 140 && y >= 114) && (x<=340 && y<=205))
  {
    if(menuOperation[opIterator] == "mainMenu") return "KONFIGURUJ";
    if(menuOperation[opIterator] == "startMenu") return "NA PULAP";
    if(menuOperation[opIterator] == "optionsMenu") return "ODCZYT PAMIECI";
  }
  if((x >= 140 && y >= 215) && (x<=340 && y<=306))
  {
    if(menuOperation[opIterator] == "mainMenu") return "OPCJE";
    if(menuOperation[opIterator] == "startMenu") return "RECZNY";
    if(menuOperation[opIterator] == "optionsMenu") return "INFORMACJE";
  }
  return "";
}

void fillGlobalArray()
{
  for(uint8_t i = 0; i < 32; i++){
    if(Devices[i].startAddress != 0)
    {
      if(Devices[i].shutterChannel != 0) shutterGlobalChannel[i] = Devices[i].startAddress + Devices[i].shutterChannel - 1;
      
      if(Devices[i].redChannel != 0) redGlobalChannel[i] = Devices[i].startAddress + Devices[i].redChannel - 1;
      if(Devices[i].greenChannel != 0) greenGlobalChannel[i] = Devices[i].startAddress + Devices[i].greenChannel - 1;
      if(Devices[i].blueChannel != 0) blueGlobalChannel[i] = Devices[i].startAddress + Devices[i].blueChannel - 1;

      if(Devices[i].panChannel != 0) panGlobalChannel[i] = Devices[i].startAddress + Devices[i].panChannel - 1;
      if(Devices[i].tilChannel != 0) tilGlobalChannel[i] = Devices[i].startAddress + Devices[i].tilChannel - 1;
      if(Devices[i].goboChannel != 0) goboGlobalChannel[i] = Devices[i].startAddress + Devices[i].goboChannel - 1;
    }
  }
}

const int universeSize = 512;

void setup() {
  SPI.begin();
  led.begin();
  Waveshield.begin();
  Serial.begin(9600);
  if (!DMX.begin(universeSize)) {
    while (1); // wait for ever
  }
  led.show();
  Waveshield.setRotation(1);
  Waveshield.setTextColor(WHITE);
  Waveshield.setTextSize(2);
  menuOperation[opIterator] = "mainMenu";
  callFunction(menuOperation[opIterator]);
  configInit();
  pinMode(freezone, INPUT);
  pinMode(blackout, INPUT);
  pinMode(manual, INPUT_PULLUP);
}

void loop() {
  TSPoint _click = Waveshield.getPoint();
  Waveshield.normalizeTsPoint(_click);
    if (_click.z > MINPRESSURE && _click.z < MAXPRESSURE) {
        if(whatClickMainMenu(_click.x, _click.y) == "START")
        {
          opIterator++;
          menuOperation[opIterator] = "startMenu";
          callFunction(menuOperation[opIterator]);
        }
        else if(whatClickMainMenu(_click.x, _click.y) == "KONFIGURUJ" && menuOperation[opIterator] != "drawDevicesFilds")
        {
          opIterator++;
          menuOperation[opIterator] = "drawDevicesFilds";
          callFunction(menuOperation[opIterator]);
        }
        else if(whatClickMainMenu(_click.x, _click.y) == "OPCJE" && menuOperation[opIterator] != "drawDevicesFilds")
        {
          opIterator++;
          menuOperation[opIterator] = "optionsMenu";
          callFunction(menuOperation[opIterator]);
        }
        else if((whatClickMainMenu(_click.x, _click.y) == "AUTOMATYCZNY" || 
        whatClickMainMenu(_click.x, _click.y) == "NA PULAP"||
        whatClickMainMenu(_click.x, _click.y) == "RECZNY") &&
        menuOperation[opIterator] != "drawDevicesFilds")
        {
          String control = whatClickMainMenu(_click.x, _click.y);
          fillGlobalArray();
          supportDrawing(control, "ABY PRZERWAC KLIKNIJ W DOWOLNE MIEJSCE", _click, false);
          callFunction(menuOperation[opIterator]);
        }
        else if(whatClickMainMenu(_click.x, _click.y) == "ZAPIS PAMIECI")
        {
          supportDrawing("Operacja", "Trwa zapisywanie prosze czekac", _click, true);
          saveToEEPROM();
          supportDrawing("Zapis udany", "Kliknij aby powrocic", _click, true);
          callFunction(menuOperation[opIterator]);
        }
        else if(whatClickMainMenu(_click.x, _click.y) == "ODCZYT PAMIECI")
        {
          supportDrawing("Operacja", "Trwa odczytywanie prosze czekac", _click, true);
          readFromEEPROM();
          supportDrawing("Odczyt udany", "Kliknij aby powrocic", _click, true);
          showToSerial();
          callFunction(menuOperation[opIterator]);
        }
        else if(whatClickMainMenu(_click.x, _click.y) == "INFORMACJE")
        {
          supportDrawing("INFORMACJE", "PROJEKT INZYNIERSKI\n\n TYTUL STEROWANIE OSWIETLENIEM\n SCENICZNYM PRZY WYKORZYSTANIU\n MIKROKONTROLERA ARDUINO\n\n AUTOR MAREK KOPEC", _click, false);
          callFunction(menuOperation[opIterator]);
        }
        else if(menuOperation[opIterator] == "drawDevicesFilds" && returnClick(8, 4, 50, 65, 5, 5, true,_click.x,_click.y ,0, 40)!=513){ //Wybranie numeru urzadzenia
          uint8_t deviceNumber = returnClick(8, 4, 50, 65, 5, 5, true,_click.x,_click.y ,0, 40); //Wczytanie numeru urządzenia
          _device = deviceNumber-1;
          opIterator++;
          String adress[] = {"0", "0", "0"};
          menuOperation[opIterator] = "drawQuestionForUsers";
          callFunction(menuOperation[opIterator]);
          uint8_t digitPlace = 0;
          while(1){
            _click = Waveshield.getPoint();
            Waveshield.normalizeTsPoint(_click);
            Waveshield.drawLine(_click.x, _click.y, _click.x, _click.y, WHITE);
            if (_click.z > MINPRESSURE && _click.z < MAXPRESSURE) {
              if(havePrev == true && isBack(_click.x, _click.y)) //Klikniecie powrotu
              {
                _click.x = 0;
                _click.y = 0;
                if(!saveDevice) cleanAfterError();
                if(saveDevice){ 
                  saveDeviceToArray();
                  cleanAfterError();
                  saveDevice = false;
                }
                back();
                break;
              }
              else if(isOk(_click.x, _click.y) && (menuOperation[opIterator] != "drawErrorInformation")) // akceptacja
              {
                _click.x = 0;
                _click.y = 0;
                if(IteratorConfig == 8)
                {
                  drawErrorInformation("Ok. Zapisuje");
                  saveDevice = true;
                  saveDeviceToArray();
                  cleanAfterError();
                  break;
                }
                if(correctValue(adress, maxFilds[IteratorConfig]) == false)
                {
                  drawErrorInformation("Zla wartosc !");
                  cleanAfterError();
                  adress[0] = "0";
                  adress[1] = "0";
                  adress[2] = "0";
                  digitPlace = 0;
                  delay(1000);
                }
                else
                {
                  answerUser[IteratorConfig] = convertToInt(adress, countDigit(maxFilds[IteratorConfig]));
                  IteratorConfig++;
                  adress[0] = "0";
                  adress[1] = "0";
                  adress[2] = "0";
                  digitPlace = 0;
                  callFunction(menuOperation[opIterator]);
                }
              }
              else if(whatArrowClick(_click.x, _click.y) == "left" && (menuOperation[opIterator] != "drawErrorInformation")) //Klikniecie strzalki w lewo
              {
                if(digitPlace != 0) digitPlace--;
                _click.x = 0;
                _click.y = 0;
                delay(1000);
              }
              else if(whatArrowClick(_click.x, _click.y) == "right" && (menuOperation[opIterator] != "drawErrorInformation")) //Klikniecie strzalki w prawo
              {
                if(digitPlace != (countDigit(maxFilds[IteratorConfig])-1)) digitPlace++;
                _click.x = 0;
                _click.y = 0;
                delay(1000);
              }
              else if(returnClick(5, 2, 75, 110, 5, 5, true,_click.x,_click.y ,30, 0) != 513 && (menuOperation[opIterator] != "drawErrorInformation")) //Klikniecie cyfry z klawiatury
              {
                uint8_t digitSelected = returnClick(5, 2, 75, 110, 5, 5, true,_click.x,_click.y ,30, 0);
                adress[digitPlace] = digit[digitSelected-1];
                drawThreeDigit(adress, countDigit(maxFilds[IteratorConfig]));
                if(digitPlace!=2) digitPlace++;
                _click.x = 0;
                _click.y = 0;
                delay(1000);
              }
              
            }
          }
        }
     if(havePrev == true && isBack(_click.x, _click.y))
     {
      _click.x = 0;
      _click.y = 0;
      if(saveDevice) saveDevice = false;
      back();
     }
     if(isOk(_click.x, _click.y) && saveDevice){ 
      back();
      saveDevice = false;
     }
  }
}
