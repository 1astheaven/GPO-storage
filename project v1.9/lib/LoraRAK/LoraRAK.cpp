#include "loraRAK.h"

LoraRAK::LoraRAK(PinName tx, PinName rx) : dev(tx, rx, 9600), tryConnect(0) {}

void LoraRAK::restart() {
    dev.write("at+set_config=device:restart\r\n", 32);
    readLora();
}

bool LoraRAK::connect() {
    dev.write("at+join\r\n", 11);
    ThisThread::sleep_for(2s);
    int result = readLora();
    if (result != 5) {
        //throw std::runtime_error("Ошибка подключения к LoRaWAN");
        return false;
    }
    return true;
}

void LoraRAK::setDevUI(const char* devUI) {
    snprintf(command, sizeof(command), "at+set_config=lora:dev_eui:%s\r\n", devUI);
    dev.write(command, strlen(command));
    if (readLora() != 0) {
        //throw std::runtime_error("Ошибка установки DevUI");
    }
}

void LoraRAK::setAppUI(const char* appUI) {
    snprintf(command, sizeof(command), "at+set_config=lora:app_eui:%s\r\n", appUI);
    dev.write(command, strlen(command));
    if (readLora() != 0) {
        //throw std::runtime_error("Ошибка установки AppUI");
    }
}

void LoraRAK::setAppKey(const char* appKey) {
    snprintf(command, sizeof(command), "at+set_config=lora:app_key:%s\r\n", appKey);
    dev.write(command, strlen(command));
    if (readLora() != 0) {
        //throw std::runtime_error("Ошибка установки AppKey");
    }
}

bool LoraRAK::send(const char* data) {
    snprintf(command, sizeof(command), "at+send=lora:5:%s\n\r", data);
    dev.write(command, strlen(command));
    int result = readLora();
    if (result != 0) {
        return true;
    } else {
        //throw std::runtime_error("Ошибка отправки данных");
        return false;
    }
}

void LoraRAK::sleep() {
    dev.write("at+set_config=device:sleep:1\r\n", 31);
    readLora();
}

void LoraRAK::wakeUp() {
    dev.write("at+set_config=device:sleep:0\r\n", 31);
    readLora();
}

int LoraRAK::readLora() {
    char buffer[300] = {};
    for (int retries = 0; retries < 50 && !dev.readable(); ++retries) {
        ThisThread::sleep_for(20ms);
    }
    ThisThread::sleep_for(150ms);

    if (dev.read(buffer, sizeof(buffer)) > 0) {
        removeNewlines(buffer);
    }

    int numError = 0;
    const char* errors[] = {"No Errors", "ERROR: 2", "ERROR: 99", "ERROR: 86", "ERROR: 80", "OK Join Success"};
    for (int i = 0; i < 6; ++i) {
        if (strstr(buffer, errors[i]) != nullptr) {
            numError = i;
        }
    }

    switch (numError) {
        case 1:
            //throw std::runtime_error("Недопустимый параметр AT-команды.");
        case 2:
            //throw std::runtime_error("Устройство не подключено к сети LoRa.");
        case 3:
            //throw std::runtime_error("Не удалось подключиться к сети LoRa.");
        case 4:
            //throw std::runtime_error("LoRa модуль занят.");
        case 5:
            return 5;
        default:
            return 0;
    }
}

void LoraRAK::removeNewlines(char* str) {
    int i = 0, j = 0;
    while (str[i] != '\0') {
        if (str[i] != '\n') {
            str[j++] = str[i];
        }
        ++i;
    }
    str[j] = '\0';
}
