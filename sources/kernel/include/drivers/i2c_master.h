#pragma once

#include <drivers/i2c_base.h>

class CI2C_Master : public AI2C_Base
{
    protected:
        volatile uint32_t& Reg(hal::BSC_Reg reg);

        // vycka, az je dokoncena probihajici I2C operace
        void Wait_Ready();

    public:
        CI2C_Master(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function);

        // otevre driver
        bool Open() override;
        // zavre driver
        void Close() override;

        // odesle pres I2C na danou adresu obsah bufferu
        void Send(uint8_t addr, const char* buffer, uint32_t len) override;
        // prijme z I2C z dane adresy obsah do bufferu o dane delce
        void Receive(uint8_t addr, char* buffer, uint32_t len) override;
};

extern CI2C_Master sI2C1;
