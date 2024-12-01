#pragma once

#include <hal/intdef.h>
#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>

constexpr const uint32_t BUFFER_SIZE = 64;

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
        // alternativni funkce pro piny
        NGPIO_Function mGPIO_Function;

        // buffer pro cteni
        char mBuffer[BUFFER_SIZE];
        // pozice nasledujici hodnoty ctene z bufferu
        int mBufferReadPosition = 0;
        // pozice nasledujici hodnoty vlozene do bufferu
        int mBufferWritePosition = 0;

    public:
        AI2C_Base(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function);

        // otevre driver
        virtual bool Open();
        // zavre driver
        virtual void Close();
        // je driver otevreny?
        bool Is_Opened() const;

        // nastavi I2C adresu
        virtual void Set_Address(uint8_t addr) = 0;

        // odesle pres I2C na danou adresu obsah bufferu
        virtual void Send(const char* buffer, uint32_t len) = 0;
        // prijme z I2C z dane adresy obsah do bufferu o dane delce
        virtual bool Receive( char* buffer, uint32_t len) = 0;
};
