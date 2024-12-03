#include <drivers/i2c_master.h>
#include <drivers/uart.h>

CI2C_Master sI2C0(hal::BSC0_Base, 0, 1, NGPIO_Function::Alt_0);
CI2C_Master sI2C1(hal::BSC1_Base, 2, 3, NGPIO_Function::Alt_0);

CI2C_Master::CI2C_Master(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function)
    : AI2C_Base(base, pin_sda, pin_scl, gpio_function)
{
    //
}

volatile uint32_t& CI2C_Master::Reg(hal::BSC_Reg reg)
{
    return mBSC_Base[static_cast<uint32_t>(reg)];
}

void CI2C_Master::Close()
{
    Reg(hal::BSC_Reg::Control) = 0;

    AI2C_Base::Close();
}

void CI2C_Master::Set_Address(uint8_t addr)
{
    Reg(hal::BSC_Reg::Slave_Address) = addr;
}

void CI2C_Master::Send(const char* buffer, uint32_t len)
{
    sUART0.Write("\r\nMaster sending: ");
    sUART0.Write(buffer, len);

    Reg(hal::BSC_Reg::Data_Length) = len;

    Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7) | (1 << 4); // zapoceti noveho prenosu (enable bsc + start transfer)
    
    for (uint32_t i = 0; i < len; i++) {
        Reg(hal::BSC_Reg::Data_FIFO) = buffer[i];
    }
}

bool CI2C_Master::Receive(char* buffer, uint32_t len)
{
    Reg(hal::BSC_Reg::Data_Length) = len;

    volatile uint32_t& s = Reg(hal::BSC_Reg::Status);
    s = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7) | (1 << 0); // zapoceti cteni (enable bsc + start transfer + read)

    // wait for operation to finish
    while (!(s & (1 << 1)))
        ;

    for (uint32_t i = 0; i < len; i++)
        buffer[i] = Reg(hal::BSC_Reg::Data_FIFO);

    if(s & (1 << 8)) // slave NACK
        return false;

    sUART0.Write("\r\nMaster received: ");
    sUART0.Write(buffer, len);

    return true;
}
