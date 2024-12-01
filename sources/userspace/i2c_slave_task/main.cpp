#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>


/**
 * Task testujici I2C
 **/

constexpr uint32_t sleep_time = 0x1000;

const float data[] = {
	2.5f,
	12.1f,
	1.35f,
	3.4f,
	2.2f,
	7.3f,
	5.1f,
	5.2f,
	3.5f,
	7.7f
};

const uint8_t data_len = sizeof(data) / sizeof(float);

void log(uint32_t log_fd, const char* message) {
	write(log_fd, message, 32);
}

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 128);
	log(logpipe, "Slave start");

	uint32_t i2c_file = open("DEV:i2c/3", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 1;
	params.targetAddress = 2;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	char log_buffer[16];
	bzero(log_buffer, 16);
	strncpy(log_buffer, "Sent: ", 6);

	float* f_ptr = (float*) data;
	char msg_buffer[5];
	bzero(msg_buffer, 5);

	char fb[10];
	bzero(fb, 10);

	sleep(sleep_time);

	while (f_ptr < (data + data_len))
	{

		char type = 'v';
		msg_buffer[0] = type;
		memcpy(f_ptr, msg_buffer+1, 4);
		write(i2c_file, msg_buffer, 5);

		log_buffer[6] = type;
		strncpy(log_buffer + 7, ftoa(*f_ptr, fb, 2), 10);
		f_ptr++;

		log(logpipe, log_buffer);
		
		sleep(sleep_time);
	}

	close(i2c_file);
	log(logpipe, "Slave done");

    return 0;
}
