#pragma once

#include <hal/intdef.h>

// parametry I2C pro prenos skrz IOCTL rozhrani
struct TI2C_IOCtl_Params
{
    //Adresa (poslednich 7 bitu) - u mastera konfiguruje cilovou adresu slave, u slave vlastni adresu
    uint8_t address;
};
