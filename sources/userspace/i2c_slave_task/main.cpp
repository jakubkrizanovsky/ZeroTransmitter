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
	log(logpipe, "Slave start");

	uint32_t i2c_file = open("DEV:i2c/2", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 1;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	char buffer[10];
	bzero(buffer, 10);

	while (true)
	{
		uint32_t num_read = read(i2c_file, buffer, 5);
		if(num_read) {
			log(logpipe, "Data received");
			write(logpipe, buffer, 5);
		} else {
			log(logpipe, "No data to read");
		}

		sleep(sleep_time);
	}

    return 0;
}
