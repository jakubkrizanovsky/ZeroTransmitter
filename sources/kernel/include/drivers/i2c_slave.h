#pragma once

#include <hal/peripherals.h>
#include "i2c.h"

class CI2C_Slave
{
    private:
        // baze pro registry BSC (I2C)
        volatile uint32_t* const mBSC_Base;
        // priznak otevreni
        bool mOpened;

        // data pin I2C
        uint32_t mSDA_Pin;
        // clock pin I2C
        uint32_t mSCL_Pin;

    protected:
        volatile uint32_t& Reg(hal::BSC_Reg reg);

        // vycka, az je dokoncena probihajici I2C operace
        void Wait_Ready();

    public:
        CI2C_Slave(unsigned long base, uint32_t pin_sda, uint32_t pin_scl);

        // otevre driver
        bool Open();
        // zavre driver
        void Close();
        // je driver otevreny?
        bool Is_Opened() const;

        // odesle pres I2C na danou adresu obsah bufferu
        void Send(uint16_t addr, const char* buffer, uint32_t len);
        // prijme z I2C z dane adresy obsah do bufferu o dane delce
        void Receive(uint16_t addr, char* buffer, uint32_t len);

        // zapocne novou transakci
        CI2C_Transaction& Begin_Transaction(uint16_t addr);
        // ukonci transakci
        void End_Transaction(CI2C_Transaction& transaction, bool commit = true);
};

extern CI2C_Slave sI2CSlave;
