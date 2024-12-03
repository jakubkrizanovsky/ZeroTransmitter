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
        // kanal i2c
        uint8_t mChannel;
        // vlastni I2C adresa
        uint8_t mAddress;
        // cilova I2C adresa
        uint8_t mTargetAddress;

        AI2C_Base* Active_Channel() {
            if (mChannel == 0)
                return &sI2C0;

            if (mChannel == 1)
                return &sI2C1;

            if (mChannel == 3)
                return &sI2CSlave;

            return nullptr;
        }

    public:
        CI2C_File(uint8_t channel)
            : IFile(NFile_Type_Major::Character), mChannel(channel), mAddress(0), mTargetAddress(0)
        {
            //
        }

        ~CI2C_File()
        {
            Close();
        }

        virtual uint32_t Read(char* buffer, uint32_t num) override
        {
            if (num > 0 && buffer != nullptr)
            {
                AI2C_Base* i2c = Active_Channel();

                if(i2c->Receive(buffer, num)) {
                    return num;
                } else {
                    //TODO - block
                    return 0;
                }
            }

            return 0;
        }

        virtual uint32_t Write(const char* buffer, uint32_t num) override
        {
            if (num > 0 && buffer != nullptr)
            {
                AI2C_Base* i2c = Active_Channel();
                i2c->Send(buffer, num);

                return num;
            }

            return 0;
        }

        virtual bool Close() override
        {
            AI2C_Base* i2c = Active_Channel();
            i2c->Close();

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
                params->address = mAddress;
                params->targetAddress = mTargetAddress;
                return true;
            }

            // proces chce nastavit parametry
            else if (dir == NIOCtl_Operation::Set_Params)
            {
                TI2C_IOCtl_Params* params = reinterpret_cast<TI2C_IOCtl_Params*>(ctlptr);
                mAddress = params->address;
                mTargetAddress = params->targetAddress;

                
                if(mChannel <= 2) {
                    Active_Channel()->Set_Address(mTargetAddress); //U mastera je potreba nastavit cilova adresa
                    Connect_Master();
                }

                if(mChannel == 3) {
                    sI2CSlave.Set_Address(mAddress); //U slave je potreba nastavit vlastni adresa
                    Connect_Slave();
                }

                return true;
            }
            return false;
        }

        virtual bool Connect_Master() {
            char buf[4];
            bzero(buf, 4);
            
            bool ack = false;
            while (!ack || strncmp(buf, "ack", 4)) {
                sI2C1.Send("syn", 4);

                sUART0.Write("\r\nMaster waiting for ack");
                TSWI_Result target;
                sProcessMgr.Handle_Process_SWI(NSWI_Process_Service::Sleep, 100, Deadline_Unchanged, 0, target);

                ack = sI2C1.Receive(buf, 4);
            }
            
            return true;
        }

        virtual bool Connect_Slave() {
            char buf[4];
            bzero(buf, 4);
            
            bool syn = false;
            while (!syn || strncmp(buf, "syn", 4)) {

                sUART0.Write("\r\nSlave waiting for syn");

                TSWI_Result target;
                sProcessMgr.Handle_Process_SWI(NSWI_Process_Service::Sleep, 100, Deadline_Unchanged, 0, target);

                syn = sI2CSlave.Receive(buf, 4);
            }

            sI2CSlave.Send("ack", 4);

            //Clear buffer of extra "syn"s
            while(sI2CSlave.Receive(buf, 1))
                ;
            
            return true;
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
            // jedina slozka path - kanal i2c
            int channel = atoi(path);
            if (channel != 0 && channel != 1 && channel != 3) // mame master kanaly 0, 1 a slave kanal 3
                return nullptr;

            if (channel == 0 && !sI2C0.Open())
                return nullptr;

            if (channel == 1 && !sI2C1.Open())
                return nullptr;

            if (channel == 3 && !sI2CSlave.Open())
                return nullptr;

            CI2C_File* f = new CI2C_File(channel);

            return f;
        }
};

CI2C_FS_Driver fsI2C_FS_Driver;
