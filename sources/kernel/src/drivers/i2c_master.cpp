#include <drivers/i2c_master.h>

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

bool CI2C_Master::Open()
{
    if (!AI2C_Base::Open()) 
        return false;

    //Reg(hal::BSC_Reg::Control) = (1 << 15) /*| (1 << 7)*/ | (1 << 4) | (1 << 0); // zapoceti cteni (enable bsc + clear fifo + start transfer + read)

    return true;
}

void CI2C_Master::Close()
{
    AI2C_Base::Close();

    Reg(hal::BSC_Reg::Control) = 0;
}

void CI2C_Master::Set_Address(uint8_t addr)
{
    Reg(hal::BSC_Reg::Slave_Address) = addr;
}

void CI2C_Master::Send(const char* buffer, uint32_t len)
{
    Reg(hal::BSC_Reg::Data_Length) = len;

    //Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7) | (1 << 4); // zapoceti noveho prenosu (enable bsc + start transfer)
    
    for (uint32_t i = 0; i < len; i++) {
        Reg(hal::BSC_Reg::Data_FIFO) = buffer[i];
    }
}

bool CI2C_Master::Receive(char* buffer, uint32_t len)
{
    Reg(hal::BSC_Reg::Data_Length) = len;

    Reg(hal::BSC_Reg::Status) = (1 << 9) | (1 << 8) | (1 << 1); // reset "slave clock hold", "slave fail" a "status" bitu
    Reg(hal::BSC_Reg::Control) = (1 << 15) | (1 << 7) | (1 << 0); // zapoceti cteni (enable bsc + clear fifo + start transfer + read)

    for (uint32_t i = 0; i < len; i++)
        buffer[i] = Reg(hal::BSC_Reg::Data_FIFO);

    return true; //TODO - buffered
}

bool CI2C_Master::Is_IRQ_Pending()
{
    //volatile uint32_t& r = Reg(hal::BSC_Slave_Reg::RIS);
    return false;//r & (1 << 0);
}

void CI2C_Master::IRQ_Callback()
{

    // mBuffer[mBufferWritePosition] = byte;
    // mBufferWritePosition = (mBufferWritePosition + 1) % BUFFER_SIZE;
}