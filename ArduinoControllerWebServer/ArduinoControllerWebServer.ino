/**
* Контроллер-исполнительное устройство (к проекту http://smartliving.ru/)
* Platform: Arduino UNO R3 + EthernetShield W5100
* IDE: Arduino 1.0.1
*
* исполнительные устройства (реле) подключены к Digital 3 - 9
*
* обращение по http://xx.xx.xx.xx/ выдаст справочную информацию по этому устройству (нужно для того, чтобы когда обращаешься
* по IP к устройству понять что это за контроллер и пр.)
*
* /state - состояние всез портов
* /command - выполнение команды
*         команды можно вызывать серией в 1 запросе. Например http://xx.xx.xx.xx/command?3=CLICK&4=CLICK&5=ON&6=OFF
*         только длинна строки запроса не должна привышать maxLength
* /getdev - получить список всех устройст на 1-wire
*         формат вывода: 
*                T<номер устройства на шине>:<HEX адрес устройства>:<текущая температура в градусах цельсия>;[...]
*                (пример T0:1060CF59010800E3:24.06;T1:109ABE59010800FE:24.56;)
*
**/

#include <Ethernet.h>
#include <SPI.h>
#include <Arduino.h>
#include "WebServer.h" // Webduino (https://github.com/sirleech/Webduino)
#include <OneWire.h>
#include <DallasTemperature.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xE4, 0xDE, 0x35 }; // MAC-адрес нашего устройства
byte ip[] = { 192, 168, 8, 120 };
byte subnet[] = { 255, 255, 255, 0 };
byte gateway[] = { 192, 168, 8, 1 };
byte dns_server[] = { 192, 168, 8, 1 };
// ip-адрес удалённого сервера
byte rserver[] = { 192, 168, 10, 130 };

// Настройки выходов
int startPin=3;
int endPin=9;

// Pin controller for connection data pin DS18S20
#define ONE_WIRE_BUS 2 // Digital 2 pin Arduino (куда подключен выход с шины датчиков DS18X20
#define TEMPERATURE_PRECISION 9

#define VERSION_STRING "0.1"
#define COMPILE_DATE_STRING "2012-09-18"

P(Page_info) = "<html><head><title>smartliving.ru controller " VERSION_STRING "</title></head><body>\n";
P(location_info) = "underground server room";
P(pin_info) = "D2 - 1-wire (many DS18S20)<br>D3-light undergroung";
P(version_info) = VERSION_STRING ". Compile date: " COMPILE_DATE_STRING;

String url = String(25);
int maxLength=25; // Максимальная длинна строки запроса

#define delayClick 1000 // задержка при обычном CLICK
#define delayLClick 3000 // задержка при длинном LCLICK
#define MAX_COMMAND_LEN             (10)
#define MAX_PARAMETER_LEN           (10)
#define COMMAND_TABLE_SIZE          (8)
#define PREFIX ""

WebServer webserver(PREFIX, 80);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Для поиска
DeviceAddress Termometers;
float tempC; 

#define NAMELEN 32
#define VALUELEN 32

char gCommandBuffer[MAX_COMMAND_LEN + 1];
char gParamBuffer[MAX_PARAMETER_LEN + 1];
long gParamValue;

typedef struct {
  char const    *name;
  void          (*function)(WebServer &server);
} command_t;

command_t const gCommandTable[COMMAND_TABLE_SIZE] = {
//  {"LED",     commandsLed, },
  {"HELP",     commandsHelp, }, // Выводит список комманд (вызов http://xx.xx.xx.xx/command?8=HELP )
  {"ON",     commandsOn, }, // Устанавливает "1" на заданном цифровом порту (вызов http://xx.xx.xx.xx/command?8=ON )
  {"OFF",     commandsOff, }, // Устанавливает "0" на заданном цифровом порту (вызов http://xx.xx.xx.xx/command?8=OFF )
  {"STATUS",     commandsStatus, }, // Получить состояние цифрового порта (1 или 0) (вызов http://xx.xx.xx.xx/command?8=STATUS ),
                                    // если вместо номера порта передать ALL (вызов http://xx.xx.xx.xx/command?ALL=STATUS ), то получим состояние всех портов (Пример вывода P3=0;P4=0;P5=0;P6=0;P7=0;P8=1;P9=1;)
  {"CLICK",     commandsClick, }, // Кратковременная "1" на порту 1сек (время настраивается) (вызов http://xx.xx.xx.xx/command?8=CLICK )
  {"LCLICK",     commandsLClick, }, // Кратковременная "1" на порту 3сек (время настраивается) (вызов http://xx.xx.xx.xx/command?8=LCLICK )
  {NULL,      NULL }
};

/**********************************************************************************************************************
 *
 * Function:    cliProcessCommand
 *
 * Description: Look up the command in the command table. If the
 *              command is found, call the command's function. If the
 *              command is not found, output an error message.
 *
 * Notes:       
 *
 * Returns:     None.
 *
 **********************************************************************/
void cliProcessCommand(WebServer &server)
{
  int bCommandFound = false;
  int idx;

  gParamValue = strtol(gParamBuffer, NULL, 0);  // Convert the parameter to an integer value. If the parameter is empty, gParamValue becomes 0.
  for (idx = 0; gCommandTable[idx].name != NULL; idx++) {  // Search for the command in the command table until it is found or the end of the table is reached. If the command is found, break out of the loop.
    if (strcmp(gCommandTable[idx].name, gCommandBuffer) == 0) {
      bCommandFound = true;
      break;
    }
  }

  if (bCommandFound == true) {  // Если команда найдена (в массиве команд), то выполняем ее. Если нет - игнорируем
    (*gCommandTable[idx].function)(server);
  }
  else { // Command not found
    server.print("ERROR: Command not found");
  }
}


/**********************************************************************************************************************/
/* Обработчики команд */

void commandsOn(WebServer &server) {
  if (gParamValue>=startPin && gParamValue<=endPin) {
     digitalWrite(gParamValue,HIGH);
  } else ErrorMessage(server);
}

void commandsOff(WebServer &server) {
  if (gParamValue>=startPin && gParamValue<=endPin) {
     digitalWrite(gParamValue,LOW);
  } else ErrorMessage(server);
}

void commandsClick(WebServer &server) {
  if (gParamValue>=startPin && gParamValue<=endPin) {
     digitalWrite(gParamValue,HIGH);     
     delay(delayClick);
     digitalWrite(gParamValue,LOW);
  } else ErrorMessage(server);
}

void commandsLClick(WebServer &server) {
  if (gParamValue>=startPin && gParamValue<=endPin) {
     digitalWrite(gParamValue,HIGH);     
     delay(delayLClick);
     digitalWrite(gParamValue,LOW);
  } else ErrorMessage(server);
}

void commandsStatus(WebServer &server) {
   if  (strcmp(gParamBuffer,  "ALL") == 0) { // выдать состояние всех пинов
          for(int i=startPin;i<=endPin;i++) {
            int st=digitalRead(i);
            char my_st[5];
            itoa(st,my_st,10);
            server.print("P");
            server.print(i);
            server.print("=");
            server.print(my_st);
            server.print(";");
          }
   } else { // выдать состояние только 1 пина
          if (gParamValue>=startPin && gParamValue<=endPin) {
            server.print("P");
            server.print(gParamValue);
            server.print("=");
            server.print(digitalRead(gParamValue));
          } else ErrorMessage(server);
    }
}

void commandsHelp(WebServer &server) {
  int idx;
  for (idx = 0; gCommandTable[idx].name != NULL; idx++) {
    server.print(gCommandTable[idx].name);
    server.print("<br>");
  }
}

/**********************************************************************************************************************/

void ErrorMessage(WebServer &server) {
    server.print("ERROR: This Pin is not configured for I/O");
}

/**********************************************************************************************************************
* Разбор запроса
**/
void parsedRequest(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  int  name_len;
  char value[VALUELEN];
  int value_len;

  server.httpSuccess();  // this line sends the standard "we're all OK" headers back to the browser

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;

  if (strlen(url_tail))
    {
    while (strlen(url_tail)) // Разбор URI на составные части (выборка параметров)
      {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_EOS) {
  //      server.printP(Params_end);
      }
       else // Получили параметр (name) и его значение (value)
        {
        // Выполняем команды
        strcpy (gCommandBuffer, value); // параметры (значение)
        strcpy (gParamBuffer, name); // команда
        cliProcessCommand(server);
        }
      }
    }
/*    
  if (type == WebServer::POST)
  {
    server.printP(Post_params_begin);
    while (server.readPOSTparam(name, NAMELEN, value, VALUELEN))
    {
      server.print(name);
      server.printP(Parsed_item_separator);
      server.print(value);
      server.printP(Tail_end);
    }
  }
*/

}

void get1wireDevices(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  //TODO получить все устройства на шине и выдать на страницу
   int numberOfDevices = sensors.getDeviceCount();
   sensors.begin();
   for(int i=0;i<numberOfDevices; i++) {
      if(sensors.getAddress(Termometers, i))
      {
          server.print("T");
          server.print(i);
          server.print(":");
          for (uint8_t i = 0; i < 8; i++) {
            if (Termometers[i] < 16) server.print("0");
              server.print(Termometers[i], HEX);
          }
          float tempC = sensors.getTempC(Termometers);
          server.print(":");
          server.print(tempC);
          server.print(";");
      } else {
            // not found
            server.print("NOT FOUND");
      }
    }
}


void stateRequest(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  strcpy (gParamBuffer, "ALL");
  commandsStatus(server);
}

/**********************************************************************************************************************
* Генерация и вывод информации об устройстве
**/
void infoRequest(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.printP(Page_info);
  server.print("IP:");
  server.print(Ethernet.localIP());
  server.print("<br>Location:");
  server.printP(location_info);
  server.print("<hr>Pin info:<br>");
  server.printP(pin_info);
  server.print("<hr>Pin current state: ");
  strcpy (gParamBuffer, "ALL");
  commandsStatus(server);
  server.print("<hr><a href='/getdev'>1-wire devices</a>");
  server.print("<hr>Commands:<br>");
  commandsHelp(server);
  server.print("<hr><br>Version info: ");
  server.printP(version_info);
  
}

/**********************************************************************************************************************
* Поиск устройств (датчиков температуры на шине 1-wire)
**/
void searchDevices() {
   Serial.print("Start search on 1-wire");
   int numberOfDevices = sensors.getDeviceCount();
   sensors.begin();
   
   for(int i=0;i<numberOfDevices; i++) {
      if(sensors.getAddress(Termometers, i))
      {
          Serial.print("Found device ");
	  Serial.print(i, DEC);
          Serial.print(" with address: ");
          for (uint8_t i = 0; i < 8; i++) {
            if (Termometers[i] < 16) Serial.print("0");
              Serial.print(Termometers[i], HEX);
          }

          Serial.print("Resolution actually set to: ");
	  Serial.print(sensors.getResolution(Termometers), DEC); 
          Serial.println();
          float tempC = sensors.getTempC(Termometers);
          Serial.print(tempC);
          Serial.println("C");
      
      } else {
            // not found
      }
    }
}


/**********************************************************************************************************************/
void setup() {
  // Для дебага будем выводить отладочные сообщения в консоль
  //TODO Убрать вывод в консоль "за дабаг" (т.е. вывод только если скимпилированно с поддержкой дебага)
  Serial.begin(9600);
  Serial.println("Start");
  
  Ethernet.begin(mac, ip, dns_server, gateway, subnet); // Инициализируем Ethernet Shield

  webserver.setDefaultCommand(&infoRequest); // дефолтная страница вывода (информация о контроллере)
  webserver.addCommand("command", &parsedRequest); // команды
  webserver.addCommand("state", &stateRequest); // выдать состояния всех устройств
  webserver.addCommand("getdev", &get1wireDevices); // получить список устройств на 1-wire
  webserver.begin();
  
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  // Настройка портов на вывод
  for (int thisPin = startPin; thisPin <=endPin; thisPin++)  {
    pinMode(thisPin, OUTPUT);      
  }
  // Настройки 1-wire 
  sensors.begin(); // Инициализация шины 1-wire (для датчиков температуры)
  sensors.requestTemperatures(); // Перед каждым получением температуры надо ее запросить
  
  searchDevices();
  
}

/**********************************************************************************************************************/
void loop() {
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);  // process incoming connections one at a time forever
}
