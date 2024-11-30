#pragma once

#include <hal/intdef.h>
#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>

class AI2C_Base {
    protected:
        // baze pro registry BSC (I2C)
        volatile uint32_t* const mBSC_Base;
        // priznak otevreni
        bool mOpened;

        // data pin I2C
        uint32_t mSDA_Pin;
        // clock pin I2C
        uint32_t mSCL_Pin;

        NGPIO_Function mGPIO_Function;

    public:
        AI2C_Base(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function);

        // otevre driver
        virtual bool Open();
        // zavre driver
        virtual void Close();
        // je driver otevreny?
        bool Is_Opened() const;

        // odesle pres I2C na danou adresu obsah bufferu
        virtual void Send(uint8_t addr, const char* buffer, uint32_t len) = 0;
        // prijme z I2C z dane adresy obsah do bufferu o dane delce
        virtual void Receive(uint8_t addr, char* buffer, uint32_t len) = 0;
};