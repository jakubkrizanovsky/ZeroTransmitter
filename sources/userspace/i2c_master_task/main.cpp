#include <stdstring.h>
#include <stdfile.h>
#include <drivers/bridges/i2c_defs.h>

/**
 * Master task - receives data from slaves using I2C and logs them to screen
 **/

//Whether extra data will be logged
const bool debug = true;
//Whether task wants to be the application master ("mst") or slave ("slv")
const char* desired_role = "mst";

const uint32_t sleep_time = 0x1000;

void log(uint32_t log_fd, const char* message) {
	write(log_fd, message, 32);
}

void log_value(uint32_t log_fd, float value, char* log_buffer, char* float_buffer) {
	bzero(log_buffer, 32);
	strncpy(log_buffer, "[M] Value: ", 11);
	strncpy(log_buffer + 11, ftoa(value, float_buffer, 2), strlen(float_buffer));
	log(log_fd, log_buffer);
}

uint32_t open_connection(uint32_t log_fd) {
	//Open file
	uint32_t i2c_file = open("DEV:i2c/1", NFile_Open_Mode::Read_Write);
	if(debug) log(log_fd, "[MD] I2C master");
	// if (i2c_file == Invalid_Handle) {
	// 	i2c_file = open("DEV:i2c/3", NFile_Open_Mode::Read_Write);
	// 	if(i2c_file == Invalid_Handle) {
	// 		log(log_fd, "[M] Failed opening i2c file");
	// 		terminate(1);
	// 	}

	// 	if (debug) {
	// 		log(log_fd, "[MD] I2C slave");
	// 	}
	// } else if (debug) {
	// 	log(log_fd, "[MD] I2C master");
	// }

	// Set addreses 
	TI2C_IOCtl_Params params;
	params.address = 1;
	params.targetAddress = 2;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	return i2c_file;
}

bool select_master(uint32_t i2c_fd, char* msg_buffer, uint32_t log_fd) {
	bzero(msg_buffer, 5);

	//Send desired role to other process
	write(i2c_fd, desired_role, 4);

	//Read other process' desired role
	uint32_t num_read = read(i2c_fd, msg_buffer, 4);
	while(num_read == 0) {
		sleep(sleep_time);
		num_read = read(i2c_fd, msg_buffer, 4);
	}

	if(strncmp(desired_role, msg_buffer, 3) == 0) {
		//Both want to be the same role - pick master by lowest address
		TI2C_IOCtl_Params params;
		ioctl(i2c_fd, NIOCtl_Operation::Get_Params, &params);
		if (debug) log(log_fd, "[MD] Both want to be same");

		return params.address < params.targetAddress;
	}

	return strncmp(desired_role, "mst", 3) == 0;
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
	uint32_t i2c_file = open_connection(logpipe);

	if(debug) log(logpipe, "[MD] Connected");

	bool app_m = select_master(i2c_file, msg_buffer, logpipe);
	if(debug) {
		if(app_m) {
			log(logpipe, "[MD] Application master");
		} else {
			log(logpipe, "[MD] Application slave");
		}
	}

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
