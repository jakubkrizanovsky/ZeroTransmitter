#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>

/**
 * Task testujici I2C
 **/

constexpr uint32_t sleep_time = 0x10000;

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 32);

	uint32_t i2c_file = open("DEV:i2c/2", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 2;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	char buffer[10];

	while (true)
	{
		read(i2c_file, buffer, 5);
		write(logpipe, "Data received", 14);
		write(logpipe, buffer, 5);

		sleep(sleep_time);
	}

    return 0;
}
