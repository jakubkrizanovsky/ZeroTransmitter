#include <drivers/i2c_slave.h>

#include <drivers/gpio.h>
#include <drivers/uart.h>

CI2C_Slave sI2CSlave(hal::BSC_Slave_Base, 18, 19, NGPIO_Function::Alt_3);

CI2C_Slave::CI2C_Slave(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function)
    : AI2C_Base(base, pin_sda, pin_scl, gpio_function)
{
    //
}

volatile uint32_t& CI2C_Slave::Reg(hal::BSC_Slave_Reg reg)
{
    return mBSC_Base[static_cast<uint32_t>(reg)];
}

void CI2C_Slave::Wait_Ready()
{
    //TODO
    // volatile uint32_t& s; = Reg(hal::BSC_Reg::Status);

    // // pockame, dokud nebude ve status registru zapnuty ready bit
    // while( !(s & (1 << 1)) )
    //     ;
}

bool CI2C_Slave::Open()
{
    if (!AI2C_Base::Open()) 
        return false;

    Reg(hal::BSC_Slave_Reg::Slave_Address) = 1; //Hardcoded slave address 1 for now
    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 9); // enable device + I2C mode

    return true;
}

void CI2C_Slave::Close()
{
    AI2C_Base::Close();

    Reg(hal::BSC_Slave_Reg::Control) = 0;
}

void CI2C_Slave::Send(uint8_t addr, const char* buffer, uint32_t len)
{
    // Reg(hal::BSC_Reg::Slave_Address) = addr;
    // Reg(hal::BSC_Reg::Data_Length) = len;

    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 8); // enable device + I2C mode + transmit enable

    for (uint32_t i = 0; i < len; i++)
        Reg(hal::BSC_Slave_Reg::Data) = buffer[i];

    // Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    // Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7); // zapoceti noveho prenosu (enable bsc + start transfer)

    //Wait_Ready();

    // volatile uint32_t& d = Reg(hal::BSC_Slave_Reg::Data);
    // char buf[64];
    // sUART0.Write(itoa(d, buf, 2));
    // sUART0.Write("\r\n");

}

void CI2C_Slave::Receive(uint8_t addr, char* buffer, uint32_t len)
{
    Reg(hal::BSC_Slave_Reg::Slave_Address) = addr;
    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 9); // zapoceti cteni (enable device + I2C mode + clear FIFO + receive enable)

    volatile uint32_t& f = Reg(hal::BSC_Slave_Reg::Flag);
    while (f & (1 << 1))
        ;

    for (uint32_t i = 0; i < len; i++) {
        buffer[i] = Reg(hal::BSC_Slave_Reg::Data);
    }
}
