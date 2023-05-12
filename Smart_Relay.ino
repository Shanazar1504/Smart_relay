#include <ESP8266WiFi.h> // подключаем библиотеку для работы с WiFi
#include <PubSubClient.h> // подключаем библиотеку для работы с MQTT
#include <EEPROM.h> // подключаем библиотеку для доступа к EEPROM памяти
#include <time.h> // подключаем библиотеку для того чтобы получать время из Интернета 
#include <Wire.h> // подключаем библиотеку "Wire"

#define relay_control_pin 0 // пин к которому вы подключили реле 
#define eeprom_size       512 // указываем размер EEPROM памяти (0-4096 байт)

#define relay_control_topic     "relay/onoff"    // имя топика на который будет приходить состояние реле
#define switching_time_topic    "time/onoff"     // имя топика на который будет приходить то, включается или выключается реле по времени
#define switching_hours_topic   "first/hours"    // имя топика на который будут приходить часы включения по времени
#define switching_minutes_topic "first/minutes"  // имя топика на который будут приходить минуты включения по времени
#define shutdown_hours_topic    "second/hours"   // имя топика на который будут приходить часы выключения по времени 
#define shutdown_minutes_topic  "second/minutes" // имя топика на который будут приходить минуты выключения по времени
#Someting
#define BUFFER_SIZE 100

const char *ssid_name      = "LDR";         // название вашей WiFi сети
const char *ssid_password  = "12345678"; // пароль указанной выше WiFi сети

const char *broker_address = "test.mosquitto.org"; // адресс MQTT сервера 
const int  server_port     = 1883;                   // порт сервера
const char *user_name      = "didar";                // имя пользователя
const char *user_password  = "12345";           // пароль пользователя

String received_payload; // переменная для хранения значения полученого от MQTT клиента 

int hours;   // переменная для хранения текущих часов
int minutes; // переменная для хранения текущих минут
int seconds; // переменная для хранения текущих секунд

unsigned long unix_switching_time;
unsigned long unix_current_time;
unsigned long unix_shutdown_time;

long timezone = 2;    // указываем часовой пояс
byte daysavetime = 1; // указываем летнее время

int relay_state;    // переменная для хранения состояния реле
int switching_time; // переменная для хранения того будет ли реле включаться или выключаться по времени

int switching_hours = -1;   // переменная для хранения часов включения по времени
int switching_minutes = -1; // переменная для хранения минут включения по времени
int shutdown_hours = -1;    // переменная для хранения часов выключения по времени
int shutdown_minutes = -1;  // переменная для хранения минут выключения по времени

int time_timing;           // переменная для таймера текущего времени
int time_switching_timing; // переменная для таймера режима включения и выключения по времени

void MQTTCallbacks(const MQTT::Publish& pub) { // функция получения данных от MQTT клиента
  Serial.print(pub.topic());   // выводим в сериал порт название топика
  Serial.print(":");
  Serial.println(pub.payload_string()); // выводим в сериал порт значение полученных данных
  
  received_payload = pub.payload_string(); // сохраняем полученные данные
  
  if (String(pub.topic()) == relay_control_topic) { // если топик на который пришли данные, равен топику на который приходит состояние реле, то
    relay_state = received_payload.toInt(); // преобразуем полученные данные в тип integer
    digitalWrite(relay_control_pin, relay_state);  //  включаем или выключаем светодиод в зависимоти от полученных значений данных
  }

  if (String(pub.topic()) == switching_time_topic) { // если топик на который пришли данные, равен топику на который приходит состояние режима включения и выключения по времени, то
    switching_time = received_payload.toInt(); // преобразуем полученные данные в тип integer
    if (switching_time == 1) { // если включение и выключение по времени включено, то
      Serial.println("Включение и выключение по времени установлено!");
      EEPROM.write(0, switching_time); // записываем в EEPROM состояние режима включения и выключения по времени
    }

    if (switching_time == 0) { // если включение и выключение по времени отключено, то
      Serial.println("Включение и выключение по времени отключено!");
      EEPROM.write(0, switching_time); // записываем в EEPROM состояние режима включения и выключения по времени
    }
  }

  if (String(pub.topic()) == switching_hours_topic) {  // если топик на который пришли данные, равен топику на который приходят часы включения по времени, то
    switching_hours = received_payload.toInt(); // преобразуем полученные данные в тип integer   
  }

  if (String(pub.topic()) == switching_minutes_topic) { // если топик на который пришли данные, равен топику на который приходят минуты включения по времени, то
    switching_minutes = received_payload.toInt(); // преобразуем полученные данные в тип integer   
  }

  if (String(pub.topic()) == shutdown_hours_topic) { // если топик на который пришли данные, равен топику на который приходят часы выключения по времени, то
    shutdown_hours = received_payload.toInt(); // преобразуем полученные данные в тип integer   
  }

  if (String(pub.topic()) == shutdown_minutes_topic) { // если топик на который пришли данные, равен топику на который приходят минуты выключения по времени, то
    shutdown_minutes = received_payload.toInt(); // преобразуем полученные данные в тип integer
    unix_switching_time = switching_hours * 60 + switching_minutes; // конвертируем время включения в формат UNIX (часы включения * 60 + минуты включения)
    unix_shutdown_time = shutdown_hours * 60 + shutdown_minutes;    // конвертируем время выключения в формат UNIX (часы выключения * 60 + минуты выключения)

    Serial.print("Время включения: ");
    Serial.print(switching_hours);
    Serial.print(":");
    Serial.print(switching_minutes);
    Serial.print(" ");
    Serial.println(unix_switching_time);

    Serial.print("Время выключения: ");
    Serial.print(shutdown_hours);
    Serial.print(":");
    Serial.print(shutdown_minutes);
    Serial.print(" ");
    Serial.println(unix_shutdown_time);

    EEPROM.write(1, switching_hours);   // записываем в EEPROM часы включения по времени
    EEPROM.write(2, switching_minutes); // записываем в EEPROM минуты включения по времени
    EEPROM.write(3, shutdown_hours);    // записываем в EEPROM часы выключения по времени
    EEPROM.write(4, shutdown_minutes);  // записываем в EEPROM минуты выключения по времени

    if (EEPROM.commit()) { // проверяем записаны ли данные в EEPROM и выполняем запись
      Serial.println("Запись в EEPROM прошла успешно!");
    } else {
      Serial.println("Запись в EEPROM не удалась :(");
    }
  }
}

WiFiClient wifi_client; // создаём объект обозначающий WiFi клиент  
PubSubClient client(wifi_client, broker_address, server_port); // создаём объект обозначающий MQTT клиент

void setup() {
  Serial.begin(115200); // указываем скорость работы COM порта
  pinMode(relay_control_pin, OUTPUT); // настраиваем пин к которому подключено реле как выход
  digitalWrite(relay_control_pin, LOW); // выключаем реле
  EEPROM.begin(eeprom_size); // запускаем EEPROM память и указываем её размер
  Serial.println("Прочитываем данные из EEPROM ...");

  switching_time    = EEPROM.read(0); // прочитываем из EEPROM состояние режима включения и выключения по времени
  switching_hours   = EEPROM.read(1); // прочитываем из EEPROM часы включения по времени
  switching_minutes = EEPROM.read(2); // прочитываем из EEPROM минуты включения по времени
  shutdown_hours    = EEPROM.read(3); // прочитываем из EEPROM часы выключения по времени
  shutdown_minutes  = EEPROM.read(4); // прочитываем из EEPROM минуты выключения по времени

  unix_switching_time = switching_hours * 60 + switching_minutes; // конвертируем время включения в формат UNIX (часы включения * 60 + минуты включения)
  unix_shutdown_time = shutdown_hours * 60 + shutdown_minutes;    // конвертируем время выключения в формат UNIX (часы выключения * 60 + минуты выключения)

  Serial.print("Включение по времени: "); Serial.println(switching_time); 
  Serial.print("Часы включения: ");       Serial.println(switching_hours);
  Serial.print("Минуты включения: ");     Serial.println(switching_minutes);
  Serial.print("Часы выключения: ");      Serial.println(shutdown_hours);
  Serial.print("Минуты выключения: ");    Serial.println(shutdown_minutes);
  Serial.print("Время включения: ");      Serial.println(unix_switching_time);
  Serial.print("Время выключения: ");     Serial.println(unix_shutdown_time);
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) { // если плата ESP8266 не подключена к WiFi, то
    Serial.print("Подключаемся к WiFi точке: ");
    Serial.println(ssid_name);
    WiFi.begin(ssid_name, ssid_password); // подключаемся к WiFi используя указаные вами название WiFi точки и её пароль

    if (WiFi.waitForConnectResult() != WL_CONNECTED) // если плата ESP8266 подключается к WiFi, то
      return;
    Serial.println("Подключились к WiFi!");
    Serial.print("Локальный IP платы ESP8266: ");
    Serial.println(WiFi.localIP()); // получаем и выводим IP платы ESP8266

    // настраиваем получение времени из Интернета
    configTime(3600*timezone, daysavetime, "pool.ntp.org", "time.nist.gov");
    Serial.println("Получаем время из Интернета ...");

    while(!time(nullptr)){
     Serial.print("*");
     delay(1000);
    }
    
    Serial.println("Время отклика ... OK");
  }

  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) { // если плата ESP8266 подключена к WiFi
    if (!client.connected()) { // если плата ESP8266 не подключена к MQTT серверу
      Serial.println("Подключаемся к MQTT серверу ...");
      if (client.connect(MQTT::Connect("arduinoClient2")
         .set_auth(user_name, user_password))) { // авторизируемся на MQTT сервере
        Serial.println("Подключились к MQTT серверу!");
        client.set_callback(MQTTCallbacks); // указываем функцию получения данных от MQTT клиента
        // подписываемся на топики, которые вы указали
        client.subscribe(relay_control_topic);
        client.subscribe(switching_time_topic);
        client.subscribe(switching_hours_topic);
        client.subscribe(switching_minutes_topic);
        client.subscribe(shutdown_hours_topic);
        client.subscribe(shutdown_minutes_topic);
      } else {
        Serial.println("Не удалось подключиться к MQTT серверу :(");
      }
    }

    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now); // записываем полученое время в переменную

    if (client.connected()) { // если плата ESP8266 подключена к MQTT серверу
      client.loop();
      // отправка данных на топики, на которые подписана плата ESP8266
    }

    // выводим полученое время из Интернета
    if (millis() - time_timing > 1000) {
      time_timing = millis();
      Serial.print(p_tm->tm_hour);
      Serial.print(":");
      Serial.print(p_tm->tm_min);
      Serial.print(":");
      Serial.print(p_tm->tm_sec);

      Serial.print(" ");

      unix_current_time = p_tm->tm_hour * 60 + p_tm->tm_min; // конвертируем полученое время из Интернета в формат UNIX
      Serial.println(unix_current_time);
    }

    if (switching_time == 1 and switching_hours != -1 and switching_minutes != -1 and shutdown_hours != -1 and shutdown_minutes != -1) { // если время включения и выключения установлено, то
      if (unix_current_time >= unix_switching_time and unix_current_time < unix_shutdown_time) { // если текущее время больше чем время включения, но меньше чем время выключения, то
        digitalWrite(relay_control_pin, HIGH); // замыкаем реле
        if (millis() - time_switching_timing > 5000) {
          time_switching_timing = millis();
          Serial.println("Включение по времени!");
        }
      } else { // иначе
        digitalWrite(relay_control_pin, LOW); // размыкаем реле
        if (millis() - time_switching_timing > 5000) {
          time_switching_timing = millis();
          Serial.println("Выключение по времени!");
        }   
      }
    }
  }
}
