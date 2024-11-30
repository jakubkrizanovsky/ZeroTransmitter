#pragma once

#include <drivers/uart.h>
#include <drivers/i2c_master.h>
#include <drivers/i2c_slave.h>
#include <hal/peripherals.h>
#include <memory/kernel_heap.h>
#include <fs/filesystem.h>
#include <stdstring.h>

// virtualni I2C soubor
class CI2C_File final : public IFile
{
    private:
        // vlastni I2C adresa
        uint8_t mAddress;
        uint8_t mTargetAddress; 

    public:
        CI2C_File(uint8_t address)
            : IFile(NFile_Type_Major::Character), mAddress(address), mTargetAddress(0)
        {
            //
        }

        ~CI2C_File()
        {
            Close();
        }

        virtual uint32_t Read(char* buffer, uint32_t num) override
        {
            //TODO - ring buffer, interrupts

            if (num > 0 && buffer != nullptr)
            {
                if (mAddress == 1)
                    sI2C1.Receive(mTargetAddress, buffer, num);

                if (mAddress == 2)
                    sI2C1.Receive(mAddress, buffer, num);

                return num;
            }

            return 0;
        }

        virtual uint32_t Write(const char* buffer, uint32_t num) override
        {
            //TODO - ring buffer, interrupts

            if (num > 0 && buffer != nullptr)
            {
                if (mAddress == 1)
                    sI2C1.Send(mTargetAddress, buffer, num);

                if (mAddress == 2)
                    sI2CSlave.Send(mAddress, buffer, num);

                return num;
            }

            return 0;
        }

        virtual bool Close() override
        {
            if (mAddress == 0)
                return false;

            if (mAddress == 1)
                sI2C1.Close();

            if (mAddress == 2)
                sI2CSlave.Close();

            mAddress = 0;
            mTargetAddress = 0;

            return IFile::Close();
        }

        virtual bool IOCtl(NIOCtl_Operation dir, void* ctlptr) override
        {
            // proces chce ziskat parametry - naformatujeme mu je do jim dodane struktury (v jeho adr. prostoru)
            if (dir == NIOCtl_Operation::Get_Params)
            {
                TI2C_IOCtl_Params* params = reinterpret_cast<TI2C_IOCtl_Params*>(ctlptr);
                params->address = mTargetAddress;
                return true;
            }

            // proces chce nastavit parametry
            else if (dir == NIOCtl_Operation::Set_Params)
            {
                TI2C_IOCtl_Params* params = reinterpret_cast<TI2C_IOCtl_Params*>(ctlptr);
                mTargetAddress = params->address;
                return true;
            }
            return false;
        }
};

class CI2C_FS_Driver : public IFilesystem_Driver
{
	public:
		virtual void On_Register() override
        {
            //
        }

        virtual IFile* Open_File(const char* path, NFile_Open_Mode mode) override
        {
            // jedina slozka path - vlastni i2c adresa (7bit - 1-127)

            int address = atoi(path);
            if (address == 0) // adresa nesmi byt 0
                return nullptr;

            if (address == 1 && !sI2C1.Open())
                return nullptr;

            if (address == 2 && !sI2CSlave.Open())
                return nullptr;

            CI2C_File* f = new CI2C_File(address);

            return f;
        }
};

CI2C_FS_Driver fsI2C_FS_Driver;
