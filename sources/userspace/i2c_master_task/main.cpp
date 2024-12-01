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
	write(log_fd, message, 32);
}

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 128);
	log(logpipe, "Master start");

	uint32_t i2c_file = open("DEV:i2c/3", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 2;
	params.targetAddress = 1;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	char log_buffer[16];
	bzero(log_buffer, 16);
	strncpy(log_buffer, "Recv: ", 6);

	char msg_buffer[5];
	bzero(msg_buffer, 5);

	char fb[5];
	bzero(fb, 5);

	sleep(sleep_time);

	while (true)
	{
		uint32_t num_read = read(i2c_file, msg_buffer, 5);
		if(num_read) {
			float f = *(float*) msg_buffer;
			strncpy(log_buffer + 6, ftoa(f, fb, 2), 5);
			log_buffer[11] = 0;
			log(logpipe, log_buffer);
		} else {
			log(logpipe, "No data to read");
		}

		sleep(sleep_time);
	}

    return 0;
}
