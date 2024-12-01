#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <drivers/bridges/i2c_defs.h>


/**
 * Task testujici I2C
 **/

constexpr uint32_t sleep_time = 0x10000;

const char* data = "aaaaabbbbbcccccdddddeeeeefffff";
const uint8_t data_len = 30;

const float f = 2.5f;

void log(uint32_t log_fd, const char* message) {
	write(log_fd, message, 32);
}

// void log(uint32_t log_fd, const char* message) {
// 	uint32_t uart_file = open("DEV:uart/0", NFile_Open_Mode::Write_Only);
// 	// while(uart_file == 0) {
// 	// 	sleep(sleep_time);
// 	// 	uart_file = open("DEV:uart/0", NFile_Open_Mode::Write_Only);
// 	// }

// 	write(uart_file, message, strlen(message) + 1);

// 	close(uart_file);
// }

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 128);
	log(logpipe, "Slave start");

	uint32_t i2c_file = open("DEV:i2c/1", NFile_Open_Mode::Read_Write);
	TI2C_IOCtl_Params params;
	params.address = 1;
	params.targetAddress = 2;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	char log_buffer[16];
	bzero(log_buffer, 16);
	strncpy(log_buffer, "Sent: ", 6);

	char* data_ptr = (char*) data;
	char msg_buffer[5];
	bzero(msg_buffer, 5);

	sleep(sleep_time);

	while (data_ptr < (data + data_len))
	{
		// strncpy(msg_buffer, data_ptr, 5);
		// data_ptr += 5;
		memcpy(&f, msg_buffer, 4);
		write(i2c_file, msg_buffer, 5);

		strncpy(log_buffer + 6, msg_buffer, 5);
		log_buffer[11] = 0;

		log(logpipe, log_buffer);
		
		sleep(sleep_time);
	}

	log(logpipe, "Slave done");

    return 0;
}
