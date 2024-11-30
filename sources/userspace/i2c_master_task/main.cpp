#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>


/**
 * Task testujici I2C
 **/

constexpr uint32_t sleep_time = 0x10000;

void log(uint32_t log_fd, const char* message) {
	write(log_fd, message, strlen(message) + 1);
}

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 64);
	log(logpipe, "Master start");

	uint32_t i2c_file = open("DEV:i2c/1", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 1;
	params.targetAddress = 2;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	while (true)
	{
		log(logpipe, "Data sent");
		write(i2c_file, "text", 5);

		sleep(sleep_time);
	}

    return 0;
}
