#ifndef _EM_EEPROM_H_
#define _EM_EEPROM_H_

#include <EEPROM.h>

void EEPromBegin() {
    EEPROM.begin(512);
}

template <class T>
unsigned short writeToEEProm(T &input, unsigned short startAddress) {
    const byte *bytes = (const byte*)(void*)&input;
    for(unsigned char i = 0; i < sizeof(T); i++, startAddress++, bytes++) {
        EEPROM.write(startAddress, *bytes);
    }
    EEPROM.commit();
    return startAddress;
}

template <class T>
unsigned short readFromEEProm(T &output, unsigned short startAddress) {
    byte *bytes = (byte *)(void *)&output;
    for(unsigned char i = 0; i < sizeof(T); i++, startAddress++, bytes++) {
        *bytes = EEPROM.read(startAddress);
    }
    return startAddress;
}

#endif
