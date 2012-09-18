ArduinoController

Код для Arduino-контроллера, используемого в проекте majordomo ( http://majordomohome.com/ http://smartliving.ru/)

Отличается от созданного автором поддержкой:
 1-wire шины для сбора данных о температуре (с привязкой адресов датчиков)
 использованием дополнительных библиотек
 немного другой реализацией функций

IDE Arduino 1.0.1
Platform: Arduino UNO R3 (MEGA, MEGA 2560) + EthernetShield W5100

ArduinoController - только информер (передает данные на сервер)
ArduinoControllerWebServer - сервер (передает данные на сервер по запросу и управляется с сервера для включения/выключения устройств)
