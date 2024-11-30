#pragma once

#include <hal/intdef.h>

// parametry I2C pro prenos skrz IOCTL rozhrani
struct TI2C_IOCtl_Params
{
    //Vlastni adresa (poslednich 7 bitu) 
    uint8_t address;

    //Cilova adresa (poslednich 7 bitu)
    uint8_t targetAddress;
};
