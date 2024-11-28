//КОД v1.5, исправлена отправка значений давления при их отправке по сравнению с версией кода 1.4
#include "mbed.h"
#include "BME280.h"
#include <cstring>
#include <cstdio>



#define MAX_DIGITS 4

DigitalOut LED(LED1);
DigitalIn mybutton(BUTTON1);
AnalogIn ain(PA_0);

// Объект для работы с UART
BufferedSerial pc(USBTX, USBRX);
BufferedSerial dev(D8, D2);
BME280 bme(I2C_SDA, I2C_SCL);

// хранит данные о климате
class LoraRAK
{
public:
    void restart() // добалена процедура перезагрузки
    {
        dev.write("at+set_config=device:restart\r\n", 32);
        Read_Lora();
    }

    bool Connect()
    {
        printf("Connecting...\n");
        dev.write("at+join\r\n", 11);
        ThisThread::sleep_for(2s);
        if(Read_Lora() == 5){
            return true;
        }
        else{
            return false;
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
    int tryConnect = 0;

    int Read_Lora()
    {
        char buffer[300] = {};
        while (!dev.readable())
        {
            ThisThread::sleep_for(20ms);
        }
        ThisThread::sleep_for(150ms);

        dev.read(buffer, sizeof(buffer));
        removeNewlines(buffer);

        printf("%s\n", buffer);
        
        // до этого момента можно ничего не менять

        int numErorr = 0;                                                            // номер ошибки
        const char *errors[6] = {"No Errors","ERROR: 2", "ERROR: 99", "ERROR: 86", "ERROR: 80","OK Join Success"}; // список ошибок
        for (int i = 0; i < 4; ++i)                                                  // Цикл проверки наличия строки ошики в обратной связи от LORA
        {
            if (strstr(buffer, errors[i]) != nullptr)
            {
                numErorr = i;
            }
        }

        switch (numErorr) // выбор действия
        {
        case 1:
            printf("Недопустимый параметр в команде AT.\n");
            break;
        case 3:
            printf("Не удалось подключиться к сети LoRa.\n");
            tryConnect++;
            this->Connect();
            break;
        case 2:
            printf("Устройство не подключено к сети LoRa.\n");
            if(tryConnect<8){
            tryConnect++;
            this->Connect();
            }
            else
            {
                printf("Количесво попыток достигло %d\n", tryConnect);
                ThisThread::sleep_for(60s);
                tryConnect = 0;
            }
            break;
        case 4:
            printf("Приемопередатчик LoRa занят, не удалось обработать новую команду.\n");
            break;
        case 5:
            printf("Устройство подключено к сети LoRa.\n");
            tryConnect = 0;
            break;
        default:
            break;
        }

        ThisThread::sleep_for(200ms);
        return numErorr;
    }

    void removeNewlines(char *str)
    {
        int i = 0, j = 0;

        // Пробегаем по строке
        while (str[i] != '\0')
        {
            if (str[i] != '\n')
            {
                str[j] = str[i]; // Перемещаем все символы, которые не '\n'
                j++;
            }
            i++;
        }

        str[j] = '\0'; // Завершаем строку
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

class Battery
{
    float R1 = 100.0;
    float R2 = 10.0;
    float K = R2 / (R1 + R2);

    float readV = 0;
    float Battery = 0;

public:
    float getVoltage()
    {

        readV = ain.read_voltage();
        Battery = readV / K;
        return Battery;
    }
};

int main()
{
    float RefV = 3.3;
    ain.set_reference_voltage(RefV);
    Battery BTR;
    printf("Voltage: %2.2f\n", BTR.getVoltage());

    mybutton.mode(PullUp);

    bme.initialize();

    dev.set_baud(9600);
    LoraRAK Lora;
    Lora.restart();
    LED = 1;
    ThisThread::sleep_for(2s);
    LED = 0;
    Lora.Connect();
    ThisThread::sleep_for(5s);
    LED = 1;
    ThisThread::sleep_for(100ms);
    LED = 0;
    ClimatCollector climatCollector;
    

    while (true)
    {
        LED = 1;
        const char *climatData = climatCollector.getStringClimat();
        Lora.Send_Lora(climatData);
        Lora.Sleep();
        delete[] climatData;
        LED = 0;
        ThisThread::sleep_for(900s);
        Lora.Wake_up();
        ThisThread::sleep_for(200ms);
    }
}
// НУЖНО РЕАЛИЗОВАТЬ

// отработка исключеий Lora коды ошибок ниже
// Класс для работы С  https://docs.rakwireless.com/Product-Categories/WisDuo/RAK811-Module/AT-Command-Manual/#at-command-syntax
// Создать класс для отпавки команд: Инициализация передатчика/отправит данные/усыпить передатчик/Пробудить передатчик/...
// Сделано: Отправка,подключение к серверу,Сон,Пробуждение
// Нужно сделать Инициализацию (не ясно как сделать чтоб обект подключался к сериал монитору при инициализации класса)
// Нужно сделать отработку ошибок каждого метода используя возвращаемые коды ошибок

// Нужно реализовать сон микроконтроллера для уменьшения потребления
// контроллер проснулся > разбудил передатчик> считал и отправил данные> Усыпил передатчик> Контроллер уснул до следующего пробуждения

//Нужно реализовать работу с EEPROM 
//Определить какие данные сохраниять
//Как сохранять 

//Абстрагироватся от Serial Вынести общение с компютером за пределы класса LORA и сделать метод который отправляет данные на комп
// Мнтоды лора в случае успеха отправляют полученную инфу от передатчика далее по ситуации