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

    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 9); // enable device + I2C mode

    return true;
}

void CI2C_Slave::Close()
{
    AI2C_Base::Close();

    Reg(hal::BSC_Slave_Reg::Control) = 0;
}

void CI2C_Slave::Set_Address(uint8_t addr)
{
    Reg(hal::BSC_Slave_Reg::Slave_Address) = addr;
}

void CI2C_Slave::Send(const char *buffer, uint32_t len)
{
    Reg(hal::BSC_Slave_Reg::Control) = (1 << 0) | (1 << 2) | (1 << 8) | (1 << 9); // enable device + I2C mode + transmit enable + receive enable
 
    for (uint32_t i = 0; i < len; i++) {
        Reg(hal::BSC_Slave_Reg::Data) = buffer[i];
    }
}

bool CI2C_Slave::Receive(char* buffer, uint32_t len)
{
    uint32_t buffer_original_position = mBufferReadPosition;
    for(int i = 0; i < len; i++) {
        if(mBufferReadPosition == mBufferWritePosition) {//ran out of data to read - abort
            mBufferReadPosition = buffer_original_position; //return buffer back to original position
            return false;
        }

        buffer[i] = mBuffer[mBufferReadPosition];
        mBufferReadPosition = (mBufferReadPosition + 1) % BUFFER_SIZE;
    }

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

        //Stop reading when buffer is full
        // if(mBufferReadPosition == mBufferWritePosition)
        //     return;
    }
}
