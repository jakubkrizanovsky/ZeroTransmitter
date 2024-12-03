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

bool CI2C_Slave::Open()
{
    if (!AI2C_Base::Open()) 
        return false;

    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 9); // enable device + I2C mode + receive enable
    Reg(hal::BSC_Slave_Reg::IFLS) = 0; // Interrupt on FIFO 1/8 full

    return true;
}

void CI2C_Slave::Close()
{
    Reg(hal::BSC_Slave_Reg::Control) = 0;

    AI2C_Base::Close();
}

void CI2C_Slave::Set_Address(uint8_t addr)
{
    Reg(hal::BSC_Slave_Reg::Slave_Address) = addr;
}

void CI2C_Slave::Send(const char *buffer, uint32_t len)
{
    sUART0.Write("\r\nSlave sending: ");
    sUART0.Write(buffer, len);

    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 8) | (1 << 9); // enable device + I2C mode + transmit enable + receive enable
 
    for (uint32_t i = 0; i < len; i++) {
        Reg(hal::BSC_Slave_Reg::Data) = buffer[i];
    }
}

bool CI2C_Slave::Receive(char* buffer, uint32_t len)
{
    uint8_t available = (mBufferWritePosition + BUFFER_SIZE - mBufferReadPosition) % BUFFER_SIZE;
    if(available < len) 
        return false;

    for(int i = 0; i < len; i++) {
        buffer[i] = mBuffer[mBufferReadPosition];
        mBufferReadPosition = (mBufferReadPosition + 1) % BUFFER_SIZE;
    }

    sUART0.Write("\r\nSlave received: ");
    sUART0.Write(buffer, len);

    return true;
}

bool CI2C_Slave::Is_IRQ_Pending()
{
    volatile uint32_t& r = Reg(hal::BSC_Slave_Reg::RIS);
    return r & (1 << 0);
}

void CI2C_Slave::IRQ_Callback()
{
    volatile uint32_t& f = Reg(hal::BSC_Slave_Reg::Flag);
    while (!(f & (1 << 1)))
    {
        mBuffer[mBufferWritePosition] = Reg(hal::BSC_Slave_Reg::Data);
        mBufferWritePosition = (mBufferWritePosition + 1) % BUFFER_SIZE;
    }
}
