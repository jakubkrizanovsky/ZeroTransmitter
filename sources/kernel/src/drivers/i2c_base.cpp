#include <drivers/i2c_base.h>
#include <stdstring.h>

AI2C_Base::AI2C_Base(unsigned long base, uint32_t pin_sda, uint32_t pin_scl, NGPIO_Function gpio_function)
    : mBSC_Base(reinterpret_cast<volatile uint32_t*>(base)), mOpened(false), mSDA_Pin(pin_sda), mSCL_Pin(pin_scl), 
    mGPIO_Function(gpio_function), mBufferReadPosition(0), mBufferWritePosition(0)
{
    bzero(mBuffer, BUFFER_SIZE);
}

bool AI2C_Base::Open()
{
    if (!sGPIO.Reserve_Pin(mSDA_Pin, true, true))
        return false;

    if (!sGPIO.Reserve_Pin(mSCL_Pin, true, true))
    {
        sGPIO.Free_Pin(mSDA_Pin, true, true);
        return false;
    }

    sGPIO.Set_GPIO_Function(mSDA_Pin, mGPIO_Function);
    sGPIO.Set_GPIO_Function(mSCL_Pin, mGPIO_Function);

    mOpened = true;

    return true;
}

void AI2C_Base::Close()
{
    sGPIO.Set_GPIO_Function(mSDA_Pin, NGPIO_Function::Input);
    sGPIO.Set_GPIO_Function(mSCL_Pin, NGPIO_Function::Input);

    sGPIO.Free_Pin(mSDA_Pin, true, true);
    sGPIO.Free_Pin(mSCL_Pin, true, true);

    mOpened = false;
}

bool AI2C_Base::Is_Opened() const
{
    return mOpened;
}