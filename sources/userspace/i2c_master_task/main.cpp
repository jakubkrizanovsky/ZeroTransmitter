#include <stdstring.h>
#include <stdfile.h>
#include <drivers/bridges/i2c_defs.h>

/**
 * Master task - receives data from slaves using I2C and logs them to screen
 **/

//Whether extra data will be logged
constexpr bool debug = true;

constexpr uint32_t sleep_time = 0x1000;

void log(uint32_t log_fd, const char* message) {
	write(log_fd, message, 32);
}

void log_value(uint32_t log_fd, float value, char* log_buffer, char* float_buffer) {
	bzero(log_buffer, 32);
	strncpy(log_buffer, "[M] Value: ", 11);
	strncpy(log_buffer + 11, ftoa(value, float_buffer, 2), strlen(float_buffer));
	log(log_fd, log_buffer);
}

uint32_t open_connection() {
	//Open file
	uint32_t i2c_file = open("DEV:i2c/1", NFile_Open_Mode::Read_Only);

	// Set addreses 
	TI2C_IOCtl_Params params;
	params.address = 2;
	params.targetAddress = 1;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	return i2c_file;
}

bool receive(uint32_t i2c_fd, char& type, float& value, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	uint32_t num_read = read(i2c_fd, msg_buffer, 5);
	if (num_read) {
		type = msg_buffer[0];
		value = *(float*) (msg_buffer+1);

		if(debug) {
			bzero(log_buffer, 32);
			strncpy(log_buffer, "[MD] Received: ", 16);
			log_buffer[15] = type;
			strncpy(log_buffer + 16, ftoa(value, float_buffer, 2), strlen(float_buffer));
			log(log_fd, log_buffer);
		}

		return true;
	} else if (debug) {
		log(log_fd, "[MD] Nothing to receive");
	}

	return false;
}

int main(int argc, char** argv)
{
	// Alloc buffers, variables
	char log_buffer[32];
	bzero(log_buffer, 32);

	char msg_buffer[5];
	bzero(msg_buffer, 5);

	char float_buffer[10];
	bzero(float_buffer, 10);

	char type;
	float value;

	// Get log pipe reference
	uint32_t logpipe = pipe("log", 128);
	log(logpipe, "[M] Master starting");

	// Open i2c connection
	uint32_t i2c_file = open_connection();

	// Wait a bit
	sleep(sleep_time);

	while (true)
	{
		// Receive data and log results in loop
		bool received = receive(i2c_file, type, value, msg_buffer, logpipe, log_buffer, float_buffer);
		if(received) {
			log_value(logpipe, value, log_buffer, float_buffer);
			if (type == 'd') {
				log(logpipe, "[M] Dangerous value");
			} else if (type == 't') {
				log(logpipe, "[M] Dangerous trend");
			} else {
				log(logpipe, "[M] OK");
			}
		}

		// Wait a bit
		sleep(sleep_time);
	}

	close(i2c_file);
	close(logpipe);

    return 0;
}
