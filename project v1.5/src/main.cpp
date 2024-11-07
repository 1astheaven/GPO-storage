#include "mbed.h"
#include "BME280.h"
#include <cstring>
#include <cstdio>

#define MAX_DIGITS 4

// Объект для работы с UART
BufferedSerial pc(USBTX, USBRX);
BufferedSerial dev(D8, D2);
BME280 bme(I2C_SDA, I2C_SCL);

// хранит данные о климате
class LoraRAK
{
public:
    void Connect()
    {
        bool connected = false;
        dev.set_baud(115200);
        while (!connected) {
            printf("Connecting...\n");
            dev.write("at+join\r\n", 10);
            ThisThread::sleep_for(2s);
            connected = (Read_Lora() > 9);
        }
    }

    bool Send_Lora(const char *data)
    {
        char Serialbuffer[50];
        snprintf(Serialbuffer, sizeof(Serialbuffer), "at+send=lora:5:%s\n\r", data);
        dev.write(Serialbuffer, strlen(Serialbuffer));
        return Read_Lora() > 0;
    }

    void Sleep() { dev.write("at+set_config=device:sleep:1\r\n", 31); Read_Lora(); }
    void Wake_up() { dev.write("at+set_config=device:sleep:0\r\n", 31); Read_Lora(); }

private:
    int Read_Lora()
    {
        char buffer[20] = {};
        ThisThread::sleep_for(200ms);
        int len = dev.read(buffer, sizeof(buffer));
        if (len > 0) {
            printf("%s", buffer);
        }
        return len;
    }
};

struct Climat
{
    char Temperature[MAX_DIGITS + 1];
    char Pressure[MAX_DIGITS + 1];
    char Humidity[MAX_DIGITS + 1];

    void getClimat()
    {
        int temperature = bme.getTemperature();
    int pressure = bme.getPressure();
    int humidity = bme.getHumidity();
        printf("Temp: %d, Pressure: %d, Humidity: %d\n", temperature, pressure, humidity);
        
        snprintf(Temperature, sizeof(Temperature), "%d", bme.getTemperature());
        snprintf(Pressure, sizeof(Pressure), "%d", bme.getPressure());
        snprintf(Humidity, sizeof(Humidity), "%d", bme.getHumidity());
    }
};

class ClimatCollector
{
public:
    Climat data;

    const char *getStringClimat()
    {
        data.getClimat();
        static char mergedArray[3 * MAX_DIGITS + 3]; // Буфер для объединённых данных
        snprintf(mergedArray, sizeof(mergedArray), "%s%s%s", data.Temperature, data.Pressure, data.Humidity);
        printf("Temp: %s, Pressure: %s, Humidity: %s\n", data.Temperature, data.Pressure, data.Humidity);
        size_t len = strlen(mergedArray);
        mergedArray[len] = (len % 2 == 0) ? '0' : '1'; // Четность
        mergedArray[len + 1] = '0'; // Завершающий символ
        mergedArray[len + 2] = '\0';

        return mergedArray;
    }
};

int main()
{
    bme.initialize();
    LoraRAK Lora;
    ThisThread::sleep_for(2s);
    Lora.Connect();
    ThisThread::sleep_for(5s);

    ClimatCollector climatCollector;

    while (true)
    {
        const char *climatData = climatCollector.getStringClimat();
        Lora.Send_Lora(climatData);
        Lora.Sleep();
        ThisThread::sleep_for(15s);
        Lora.Wake_up();
        ThisThread::sleep_for(200ms);
    }
}




// НУЖНО РЕАЛИЗОВАТЬ
//Сделан буфер!

// отработка исключеий Lora коды ошибок ниже
// Класс для работы С  https://docs.rakwireless.com/Product-Categories/WisDuo/RAK811-Module/AT-Command-Manual/#at-command-syntax
// Создать класс для отпавки команд: Инициализация передатчика/отправит данные/усыпить передатчик/Пробудить передатчик/...
// Сделано: Отправка,подключение к серверу,Сон,Пробуждение
// Нужно сделать Инициализацию (не ясно как сделать чтоб объект подключался к сериал монитору при инициализации класса)
// Нужно сделать отработку ошибок каждого метода используя возвращаемые коды ошибок

// Нужно реализовать сон микроконтроллера для уменьшения потребления
// контроллер проснулся > разбудил передатчик> считал и отправил данные> Усыпил передатчик> Контроллер уснул до следующего пробуждения

//Нужно реализовать работу с EEPROM 
//Определить какие данные сохраниять
//Как сохранять 

//Абстрагироватся от Serial Вынести общение с компютером за пределы класса LORA и сделать метод который отправляет данные на комп
// Мнтоды лора в случае успеха отправляют полученную инфу от передатчика далее по ситуации