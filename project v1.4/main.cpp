#include "mbed.h"
#include "BME280.h"
#include <cstdio>

// Объект для работы с UART
BufferedSerial dev (D8, D2);
BME280 bme (I2C_SDA, I2C_SCL);

int main() 
{
    dev.set_baud(115200); // Установка скорости передачи данных (бод)
    bme.initialize();
    char buffer[32];

    int len;
    len = snprintf(buffer, sizeof(buffer), "at+join\r\n");
    dev.write(buffer,len);
    dev.read(buffer, '\n');
    ThisThread::sleep_for(5s);

    while (1) 
    {
        int data = bme.getTemperature();
        char data2 = data;

        // Отправка данных через UART
        len = snprintf(buffer, sizeof(buffer), "at+send=lora:5:%d\n\r", data2);
        //len = snprintf(buffer, sizeof(buffer), "Data: %d\r\n", data);
        dev.write(buffer, len);
        dev.read(buffer, '\n');

        //len = snprintf(buffer, sizeof(buffer), "at+set_config=device:sleep:1\n\r");
        //dev.write(buffer, len);
        // Задержка перед следующей отправкой данных
        ThisThread::sleep_for(10s);
    }
}
