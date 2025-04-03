#include "mbed.h"
#include "stm32f103c8t6.h"
#include "PinNames.h"
#include "BME280.h"
#include "FlashIAP.h"
#include "loraRAK.h"
#include <cstring>
#include <cstdio>

FlashIAP flash;

#define MAX_DIGITS 4
#define CONFIG_MODE_HOLD_TIME 2s

DigitalOut LED(PB_15);
InterruptIn mybutton(PA_8);
AnalogIn ain(PA_0);
BufferedSerial pc(PA_2, PA_3);
LoraRAK Lora(PB_10, PB_11);
BME280 bme(PB_7, PB_6);

const uint32_t flash_sector_size = 1024;  // Размер сектора
const uint32_t flash_address = 0x801fc00; // Последний сектор для хранения данных

// конфигурация
struct DeviceConfig
{
  char DevUI[16] = "123343454543534";
  char AppUI[16] = "123343454543534";
  char AppKey[32] = "123343454543534123343454543534";
  uint16_t sendPeriod = 10;
  int floor = 3333;
  int room = 3333;
  bool logEnabled = true;
  uint32_t signature = 0xDEADBEEF;
} config;

const char* encodeData(uint8_t floor, uint16_t room, float volt, float temp, float hum, uint16_t pressure) {
    static char buffer[32]; // Увеличен размер буфера
    snprintf(buffer, sizeof(buffer), "%02X %04X %02X %04X %04X %04X",
             floor, room, (uint16_t)(volt * 100), (uint16_t)(temp * 100), 
             (uint16_t)(hum * 100), pressure);
    return buffer;
}

class Climat {
private:
    char Temperature[MAX_DIGITS + 1];
    char Pressure[MAX_DIGITS + 1];
    char Humidity[MAX_DIGITS + 1];
    char Voltage[MAX_DIGITS + 1];
    float readV = 0;
/*    
public:                                                     пока вырезано
    float getBatteryVoltage() {
        return ain.read_voltage() * (110.0 / 10.0);
    }*/

    /*
    void update() {
        snprintf(Temperature, sizeof(Temperature), "%d", bme.getTemperature());
        snprintf(Pressure, sizeof(Pressure), "%04d", bme.getPressure());
        snprintf(Humidity, sizeof(Humidity), "%d", bme.getHumidity());
        snprintf(Voltage, sizeof(Voltage), "%d", static_cast<int>(getBatteryVoltage() * 100));
    }

    const char* getStringClimat() {
        static char mergedArray[4 * MAX_DIGITS + 3];
        snprintf(mergedArray, sizeof(mergedArray), "%s%s%s%s", Temperature, Pressure, Humidity, Voltage);
        return mergedArray;
    }
    */
};

// метод записи в память
void writeFlash()
{
  flash.init();

  if (flash_address % flash_sector_size != 0)
  {
    flash.deinit();
    return;
  }

  flash.erase(flash_address, flash_sector_size);
  flash.program(&config, flash_address, sizeof(config));

  flash.deinit();
}

// метод чтения памяти
void readFlash()
{
  flash.init();

  DeviceConfig tempConfig;

  flash.read(&tempConfig, flash_address, sizeof(tempConfig));
  flash.deinit();
  // проверка одного данного если чтото уедет то поменяется возможно это лишнее
  if (tempConfig.signature == 0xDEADBEEF)
  {
    config = tempConfig;
  }
}

void enterConfigMode() {
    pc.set_baud(9600);
    char buffer[64];
    size_t buffer_index = 0;

    while (true) {
        if (pc.readable()) {
            char c;
            pc.read(&c, 1);
            if (c == '\n' || c == '\r') {
                buffer[buffer_index] = '\0';
                buffer_index = 0;
                if (strcmp(buffer, "EXIT") == 0) break;
                if (strncmp(buffer, "SET_DEVUI:", 10) == 0) strncpy(config.DevUI, buffer + 10, 16);
                else if (strncmp(buffer, "SET_APPUI:", 10) == 0) strncpy(config.AppUI, buffer + 10, 16);
                else if (strncmp(buffer, "SET_APPKEY:", 11) == 0) strncpy(config.AppKey, buffer + 11, 32);
                else if (strncmp(buffer, "SET_PERIOD:", 11) == 0) config.sendPeriod = atoi(buffer + 11);
                else if (strncmp(buffer, "SET_FLOOR:", 10) == 0) strncpy(config.floor, buffer + 10, 4);
                else if (strncmp(buffer, "SET_ROOM:", 9) == 0) strncpy(config.room, buffer + 9, 4);
                else if (strcmp(buffer, "ENABLE_LOGS") == 0) config.logEnabled = true;
                else if (strcmp(buffer, "DISABLE_LOGS") == 0) config.logEnabled = false;
            } else if (buffer_index < sizeof(buffer) - 1) {
                buffer[buffer_index++] = c;
            }
        }
    }
    writeFlash();
}

void runDevice() {
    Climat climat;
    int trysend = 0;
    while (climat.getBatteryVoltage() > 3.4) {
        LED = 1;
        //climat.update();
        while (!Lora.send(encodeData(7,707,climat.getBatteryVoltage(),bme.getTemperature(),bme.getHumidity(),bme.getPressure())) && trysend != 10) {
            printf("Ошибка отправки, повторная попытка...\n");
            trysend++;
            ThisThread::sleep_for(5s);
        }
        trysend = 0;
        Lora.sleep();
        LED = 0;
        ThisThread::sleep_for(config.sendPeriod * 1s);
        Lora.wakeUp();
    }
    while (true) {
        LED = !LED;
        ThisThread::sleep_for(500ms);
    }
}

int main() {
    pc.set_baud(9600);
    ain.set_reference_voltage(3.3);
    bme.initialize();
    //readFlash();
    //if (config.signature != 0xDEADBEEF) enterConfigMode();
    Lora.restart();
    LED = 1;
    ThisThread::sleep_for(2s);
    LED = 0;
    while (!Lora.connect()) {
        printf("Ошибка подключения к LoRa, повторная попытка...\n");
        ThisThread::sleep_for(3s);
    }
    runDevice();
    return 0;
}

