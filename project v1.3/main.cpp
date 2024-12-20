#include "mbed.h"

// Объект для работы с UART
BufferedSerial dev (D8, D2);

int main() 
{
    // Установка скорости передачи данных (бод)
    dev.set_baud(115200);

    // Основной цикл программы
    while (1) {
        int data = 42;

        // Отправка данных через UART
        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "Data: %d\r\n", data);
        dev.write(buffer, len);

        // Задержка перед следующей отправкой данных
        ThisThread::sleep_for(1s);
    }
}
