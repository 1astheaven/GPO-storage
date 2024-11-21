#include "mbed.h"
#include "stm32f103c8t6.h" //добавлено
#include "PinNames.h" //добавлено
//#include "USBSerial.h" //добавлено, но пока не работает
#include "stdio.h"
#include "BME280.h"
#include <cstring>
#include <cstdio>

#define MAX_DIGITS 4

//USBSerial usbSerial(0x1f00, 0x2012, 0x0001,  false);
// Объект для работы с UART
//BufferedSerial pc(USBTX, USBRX);
BufferedSerial dev(PA_9, PA_10); //добавлено
BME280 bme(PB_9, PB_8); //добавлено

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
        
        snprintf(Temperature, sizeof(Temperature), "%d", temperature);
        snprintf(Pressure, sizeof(Pressure), "%04d", pressure);  // Форматируем давление с ведущими нулями
        snprintf(Humidity, sizeof(Humidity), "%d", humidity);
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
    confSysClock();
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
        ThisThread::sleep_for(900s);
        Lora.Wake_up();
        ThisThread::sleep_for(200ms);
    }
}

