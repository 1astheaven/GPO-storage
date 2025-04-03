#ifndef LORARAK_H
#define LORARAK_H

#include "mbed.h"
#include <stdexcept>

class LoraRAK {
public:
    LoraRAK(PinName tx, PinName rx);
    void restart();
    bool connect();
    void setDevUI(const char* devUI);
    void setAppUI(const char* appUI);
    void setAppKey(const char* appKey);
    bool send(const char* data);
    void sleep();
    void wakeUp();

private:
    BufferedSerial dev;
    char command[128];
    int tryConnect;

    int readLora();
    void tryReconnect();
    void removeNewlines(char* str);
};

#endif // LORARAK_H
