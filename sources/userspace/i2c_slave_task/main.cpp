#include <stdstring.h>
#include <stdfile.h>
#include <mathf.h>
#include <drivers/bridges/i2c_defs.h>


/**
 * Slave task - reads data, determines whether value or trend is dangerous, logs value and trend and sends it to master via i2c
 **/

//Whether extra data will be logged
const bool debug = true;
//Whether task wants to be the application master ("mst") or slave ("slv")
const char* desired_role = "mst";

const uint32_t sleep_time = 0x1000;

const float low_threshold = 3.5f;
const float high_threshold = 11.0f;

//Data to transmit
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

void log_value(uint32_t log_fd, float value, char* log_buffer, char* float_buffer) {
	strncpy(log_buffer, "[S] Value: ", 11);
	strncpy(log_buffer + 11, ftoa(value, float_buffer, 2), 10);
	log_buffer[21] = 0;
	log(log_fd, log_buffer);
}

void log_trend(uint32_t log_fd, float trend, char* log_buffer, char* float_buffer) {
	strncpy(log_buffer, "[S] Trend: ", 11);
	strncpy(log_buffer + 11, ftoa(trend, float_buffer, 2), 10);
	log_buffer[21] = 0;
	log(log_fd, log_buffer);
}

uint32_t open_connection(uint32_t log_fd) {
	//Open file
	uint32_t i2c_file = open("DEV:i2c/3", NFile_Open_Mode::Write_Only);
	if(debug) log(log_fd, "[SD] I2C slave");
	// if (i2c_file == Invalid_Handle) {
	// 	i2c_file = open("DEV:i2c/3", NFile_Open_Mode::Write_Only);
	// 	if (debug) {
	// 		log(log_fd, "[SD] I2C slave");
	// 	}
	// } else if (debug) {
	// 	log(log_fd, "[SD] I2C master");
	// }

	// Set addreses 
	TI2C_IOCtl_Params params;
	params.address = 2;
	params.targetAddress = 1;
	ioctl(i2c_file, NIOCtl_Operation::Set_Params, &params);

	return i2c_file;
}

bool select_master(uint32_t i2c_fd, char* msg_buffer, uint32_t log_fd, char* float_buffer) {
	bzero(msg_buffer, 5);

	//Send desired role to other process
	write(i2c_fd, desired_role, 3);

	//Read other process' desired role
	uint32_t num_read = read(i2c_fd, msg_buffer, 3);
	while(num_read == 0) {
		sleep(sleep_time);
		num_read = read(i2c_fd, msg_buffer, 3);
	}

	if(strncmp(desired_role, msg_buffer, 3) == 0) {
		//Both want to be the same role - pick master by lowest address
		TI2C_IOCtl_Params params;
		ioctl(i2c_fd, NIOCtl_Operation::Get_Params, &params);
		if (debug) log(log_fd, "[SD] Both want to be same");

		return params.address < params.targetAddress;
	}

	return strncmp(desired_role, "mst", 3) == 0;
}

float calculate_trend(float last_value, float current_value) {
	return lerp(last_value, current_value, 2);
}

bool is_dangerous(float value) {
	return value < low_threshold || value > high_threshold;
}

void send(uint32_t i2c_fd, char type, float value, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	msg_buffer[0] = type;
	memcpy(&value, msg_buffer+1, 4);
	write(i2c_fd, msg_buffer, 5);

	if(debug) {
		bzero(log_buffer, 32);
		strncpy(log_buffer, "[SD] Sent: ", 12);
		log_buffer[11] = type;
		strncpy(log_buffer + 12, ftoa(value, float_buffer, 2), strlen(float_buffer));
		log(log_fd, log_buffer);
	}
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

	float last_value = -1; // -1 ... not initialized
	float value;
	bool trend_set = false;
	float trend;
	char type;

	// Get log pipe reference
	uint32_t logpipe = pipe("log", 128);
	log(logpipe, "[S] Slave starting");

	// Open i2c connection
	uint32_t i2c_file = open_connection(logpipe);

	if(debug) log(logpipe, "[SD] Connected");

	bool app_m = select_master(i2c_file, msg_buffer, logpipe, float_buffer);
	if(debug) {
		if(app_m) {
			log(logpipe, "[SD] Application master");
		} else {
			log(logpipe, "[SD] Application slave");
		}
	}

	// Wait a bit
	sleep(10 * sleep_time);

	float* f_ptr = (float*) data;
	while (f_ptr < (data + data_len))
	{
		// Read next value
		value = *f_ptr;

		// Calculate trend if applicable
		if(last_value > 0) {
			trend = calculate_trend(last_value, value);
			trend_set = true;
		} else {
			trend_set = false;
		}

		// Log current value and trend if applicable
		log_value(logpipe, value, log_buffer, float_buffer);
		if(trend_set) {
			log_trend(logpipe, trend, log_buffer, float_buffer);
		} else {
			log(logpipe, "[S] Trend: -");
		}

		// Determine type of message
		if(is_dangerous(value)) {
			type = 'd';
			log(logpipe, "[S] Dangerous value");
		} else if(trend_set && is_dangerous(trend)) {
			type = 't';
			log(logpipe, "[S] Dangerous trend");
		} else {
			type = 'v';
			log(logpipe, "[S] OK");
		}

		// Send data via i2c
		send(i2c_file, type, value, msg_buffer, logpipe, log_buffer, float_buffer);

		// Set last value and move to next value
		last_value = value;
		f_ptr++;
		
		// Wait a bit
		sleep(sleep_time);
	}

	// Close i2c file
	log(logpipe, "[S] Slave done");

	close(i2c_file);
	close(logpipe);

    return 0;
}
