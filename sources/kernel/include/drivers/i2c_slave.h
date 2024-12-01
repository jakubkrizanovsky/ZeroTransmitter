#pragma once

#include <drivers/i2c_base.h>

class CI2C_Slave : public AI2C_Base
{
    protected:
        volatile uint32_t& Reg(hal::BSC_Slave_Reg reg);

    public:
        CI2C_Slave(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function);

        // otevre driver
        bool Open() override;
        // zavre driver
        void Close() override;

        // nastavi I2C adresu - vlastni adresu
        virtual void Set_Address(uint8_t addr) override;

        // odesle pres I2C na danou adresu obsah bufferu
        virtual void Send(const char* buffer, uint32_t len) override;
        // prijme z I2C z dane adresy obsah do bufferu o dane delce
        virtual bool Receive(char* buffer, uint32_t len) override;

        // ceka IRQ?
        bool Is_IRQ_Pending();
        // obslouzi IRQ
        void IRQ_Callback();
};

extern CI2C_Slave sI2CSlave;
