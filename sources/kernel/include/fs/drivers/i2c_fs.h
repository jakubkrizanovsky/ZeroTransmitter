#pragma once

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
        // cislo kanalu i2c
        uint8_t mChannelNumber;
        // kanal i2c
        AI2C_Base* mChannel;
        // vlastni I2C adresa
        uint8_t mAddress;
        // cilova I2C adresa
        uint8_t mTargetAddress;

        // handshake pri pouziti master rozhrani
        virtual bool Connect_Master() {
            char buf[4];
            bzero(buf, 4);
            
            bool ack = false;
            while (!ack || strncmp(buf, "ack", 4)) {
                mChannel->Send("syn", 4);

                TSWI_Result target;
                sProcessMgr.Handle_Process_SWI(NSWI_Process_Service::Sleep, 100, Deadline_Unchanged, 0, target);

                ack = mChannel->Receive(buf, 4);
            }
            
            return true;
        }

        // handshake pri pouziti slave rozhrani
        virtual bool Connect_Slave() {
            char buf[4];
            bzero(buf, 4);
            
            bool syn = false;
            while (!syn || strncmp(buf, "syn", 4)) {
                TSWI_Result target;
                sProcessMgr.Handle_Process_SWI(NSWI_Process_Service::Sleep, 100, Deadline_Unchanged, 0, target);

                syn = mChannel->Receive(buf, 4);
            }

            mChannel->Send("ack", 4);

            //Clear buffer of extra "syn"s received
            while(mChannel->Receive(buf, 4))
                ;
            
            return true;
        }

    public:
        CI2C_File(uint8_t channelNumber, AI2C_Base* channel)
            : IFile(NFile_Type_Major::Character), mChannelNumber(channelNumber), mChannel(channel), mAddress(0), mTargetAddress(0)
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
                if(mChannel->Receive(buffer, num) && buffer[0] != 0) {
                    return num;
                } 
            }

            return 0;
        }

        virtual uint32_t Write(const char* buffer, uint32_t num) override
        {
            if (num > 0 && buffer != nullptr)
            {
                mChannel->Send(buffer, num);

                return num;
            }

            return 0;
        }

        virtual bool Close() override
        {
            mChannel->Close();
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

                if(mChannelNumber <= 2) {
                    mChannel->Set_Address(mTargetAddress); //U mastera je potreba nastavit cilova adresa
                    Connect_Master(); //Handshake
                }

                if(mChannelNumber == 3) {
                    mChannel->Set_Address(mAddress); //U slave je potreba nastavit vlastni adresa
                    Connect_Slave(); //Handshake
                }

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
            AI2C_Base* channel;

            // jedina slozka path - cislo kanalu i2c
            int channelNumber = atoi(path);

            // na zakalade cisla kanalu zvolime driver a kanal
            switch (channelNumber)
            {
                case 0:
                    channel = &sI2C0;
                    break;
                case 1:
                    channel = &sI2C1;
                    break;
                case 3:
                    channel = &sI2CSlave;
                    break;

                default: //neznamy kanal
                    return nullptr;
                    break;
            }
            
            // otevreme driver
            if(!channel->Open())
                return nullptr;

            // otevreme soubor
            CI2C_File* f = new CI2C_File(channelNumber, channel);

            return f;
        }
};

CI2C_FS_Driver fsI2C_FS_Driver;
