/**
* Platform: Arduino UNO R3 + EthernetShield W5100
* Подключение датчиков температуры DS18x20 по схеме с нормальным питанием (пример http://sourceforge.net/apps/trac/arduios/wiki/owmon)
* данные (зеленый провод со схемы по ссылке указанной выше) подключаем на D2
* ВНИМАНИЕ! Для того чтобы у Вас заработало получение данных с детчиков температуры, нужно прописать адреса ваших датчиков (ниже по коду)
*
* Остальные датчики:
* Digital 4 - Датчик гаражной двери на 7-ый пин
* Digital 5 - Датчик въездных ворот на 8-ый пин 
* Digital 6 - Датчик движения 1
* Digital 7 - Датчик движения 2
* Digital 8 - Датчик света
*  
*
*
**/
#include <Ethernet.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// MAC-адрес нашего устройства
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 8, 120 };
byte subnet[] = { 255, 255, 255, 0 };
byte gateway[] = { 192, 168, 8, 1 };
byte dns_server[] = { 192, 168, 8, 1 };
// ip-адрес удалённого сервера
byte server[] = { 192, 168, 10, 130 };

EthernetClient rclient;

// Pin controller for connection data pin DS18S20
#define ONE_WIRE_BUS 2 // D2 pin Arduino (куда подключен выход с шины датчиков DS18X20
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Настройка датчиков температуры (адреса удобно определять программой из примера DallasTemperature/Mulriple)
//Device 0 Address: 1060CF59010800E3
//Device 1 Address: 109ABE59010800FE
// Датчик температуры на 3-й пин (зал)
DeviceAddress Termometer1 =  { 0x10, 0x60, 0xCF, 0x59, 0x1, 0x8, 0x0, 0xE3 };
// Датчик температуры на 2-й пин (котёл)
DeviceAddress Termometer2 = { 0x10, 0x9A, 0xBE, 0x59, 0x1, 0x8, 0x0, 0xFE };

// Инициализация начальных значений
float tempC;
int old_temperature1=0;
int old_temperature2=0;
int old_garage=0;
int old_entry=0;
int old_movement_1=0;
int old_movement_2=0;
int old_light=0;
char buf[80];
char ipbuff[16];

// ------------------------------------------------------------------------------------------------

// Функция отправки HTTP-запроса на сервер
void sendHTTPRequest() {
  Serial.println(buf); 
  if (rclient.connect(server, 80)) { 
   Serial.println("OK"); 
   rclient.print(buf);
   rclient.println(" HTTP/1.0");
   rclient.print("Host: ");
   sprintf(ipbuff, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
   rclient.println(ipbuff); // ip адрес нашего контроллера в текстовом виде
   rclient.print("Content-Type: text/html\n");
   rclient.println("Connection: close\n");
   delay(2000);
   rclient.stop();
  } else {
   Serial.println("FAILED");     
  }
}

// ------------------------------------------------------------------------------------------------
void setup()
{
  // Для дебага будем выводить отладочные сообщения в консоль
  Serial.begin(9600);
  Serial.println("Start");

// Настройки 1-wire 
  sensors.begin(); // Инициализация шины 1-wire (для датчиков температуры)

// Определим сколько датчиков на шине
  Serial.print("1-wire: found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.print(" devices.");
  Serial.print(" Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

/*  
if (sensors.validAddress(Termometer1) == true) {
  Serial.println("Address is VALID");
  if (sensors.isConnected(Termometer1) == true) {
    Serial.println("Termometer1 isConnected");
    sensors.setResolution(Termometer1, TEMPERATURE_PRECISION);
    sensors.setResolution(Termometer2, TEMPERATURE_PRECISION);
  } else {
    Serial.println("Termometer1 isConnected=false");
  }
}
*/

 Ethernet.begin(mac, ip, dns_server, gateway, subnet); // Инициализируем Ethernet Shield

 pinMode(4, INPUT); // Датчик гаражной двери
 old_garage=digitalRead(4);

 pinMode(5, INPUT); // Датчик въездных ворот
 old_entry=digitalRead(5);

 pinMode(6, INPUT); // Датчик движения 1
 old_movement_1=digitalRead(6);

 pinMode(7, INPUT); // Датчик движения 2
 old_movement_2=digitalRead(7);

 pinMode(8, INPUT); // Датчик света
 old_light=digitalRead(8);

}

// ------------------------------------------------------------------------------------------------
void loop()
{

  // Перед каждым получением температуры надо ее запросить
  sensors.requestTemperatures();
  
  int valid_sensor=0;

  delay(1000); // задержка в 1 сек.  


  // TEMP SENSOR 1 --------------------------------------
  float current_temp1=0;
  tempC = sensors.getTempC(Termometer1); // Получим температуру от датчика (или ошибку)
  if (tempC == DEVICE_DISCONNECTED) { 
    // Устройство отсоеденнено
    Serial.println("Termometer1 is DEVICE_DISCONNECTED");
    // TODO Послать бы на сервер сигнал о том, что датчит не отвечает
    
  } else {
    // Устройство отдало реальное значение температуры (или осталось старое)
    Serial.print("T1: ");
    current_temp1 = tempC; // получаем температуру
    Serial.println(current_temp1);
    if ((old_temperature1!=(int)current_temp1) && (current_temp1<79) && (current_temp1>5)) {
      int temp1 = (current_temp1 - (int)current_temp1) * 100; // выделяем дробную часть
      sprintf(buf, "GET /objects/?object=sensorKotel&op=m&m=tempChanged&t=%0d.%d", (int)current_temp1, abs(temp1));   
      sendHTTPRequest();
    }
  old_temperature1=(int)current_temp1;
  }


  // TEMP SENSOR 2 --------------------------------------
  float current_temp2=0;
  tempC = sensors.getTempC(Termometer2); // Получим температуру от датчика (или ошибку)
  if (tempC == DEVICE_DISCONNECTED) { 
    // Устройство отсоеденнено
    Serial.println("Termometer2 is DEVICE_DISCONNECTED");
    // TODO Послать бы на сервер сигнал о том, что датчит не отвечает
    
  } else {
    // Устройство отдало реальное значение температуры (или осталось старое)
    Serial.print("T2: ");
    current_temp2 = tempC; // получаем температуру
    Serial.println(current_temp2);
    if ((old_temperature2!=(int)current_temp2) && (current_temp2<50) && (current_temp2>5)) {
      int temp1 = (current_temp2 - (int)current_temp2) * 100; // выделяем дробную часть
      sprintf(buf, "GET /objects/?object=sensorZal&op=m&m=tempChanged&t=%0d.%d", (int)current_temp2, abs(temp1));   
      sendHTTPRequest();
    }
  old_temperature2=(int)current_temp2;
  }
  

  //GARAGE GATES SENSOR
  Serial.println("G");
  int current_garage=digitalRead(4);
  //Serial.println(current_garage);
  if (current_garage!=(int)old_garage) {
    old_garage=(int)current_garage;
    sprintf(buf, "GET /objects/?object=sensorGarage&op=m&m=statusChanged&status=%i", (int)current_garage);
    sendHTTPRequest();
  }

 //ENTRY GATES SENSOR

  Serial.println("E");
  int current_entry=digitalRead(5);
  //Serial.println(current_entry);
  if (current_entry!=(int)old_entry) {
    old_entry=(int)current_entry;
    sprintf(buf, "GET /objects/?object=sensorEntry&op=m&m=statusChanged&status=%i", (int)current_entry);
    sendHTTPRequest();
  }

 //MOVEMENT 1 SENSOR
  Serial.println("M1");
  int current_movement_1=digitalRead(6);
  //Serial.println(current_movement_1);
  if (current_movement_1!=(int)old_movement_1) {
    old_movement_1=(int)current_movement_1;
    sprintf(buf, "GET /objects/?object=sensorMovement1&op=m&m=statusChanged&status=%i", (int)current_movement_1);
    sendHTTPRequest();
  }

 //MOVEMENT 2 SENSOR
  Serial.println("M2");
  int current_movement_2=digitalRead(7);
  //Serial.println(current_movement_2);
  if (current_movement_2!=(int)old_movement_2) {
    old_movement_2=(int)current_movement_2;
    sprintf(buf, "GET /objects/?object=sensorMovement2&op=m&m=statusChanged&status=%i", (int)current_movement_2);
    sendHTTPRequest();
  }


 //LIGHT SENSOR

  Serial.println("L");
  int current_light=digitalRead(8);
  //Serial.println(current_light);
  if (current_light!=(int)old_light) {
    old_light=(int)current_light;
    sprintf(buf, "GET /objects/?object=sensorLight&op=m&m=statusChanged&status=%i", (int)current_light);
    sendHTTPRequest();
  }  


}
