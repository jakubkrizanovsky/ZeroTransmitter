#include <stdstring.h>
#include <stdfile.h>
#include <drivers/bridges/i2c_defs.h>
#include <mathf.h>

/**
 * Universal task that can act as both master and slave during i2c communication
 * Master - receives data from slaves using I2C and logs them to screen
 * Slave - reads data, determines whether value or trend is dangerous, logs value and trend and sends it to master via i2c
 **/

#pragma region configuration

//Which interface of I2C will be used for communication ('0', '1' or '3')
const char i2c_interface = '3';
//I2C address of this device (or task)
const uint32_t address = 2;
//I2C address of target device (or task) to comunicate with
const uint32_t target_address = 1;
//Whether task wants to be the application master ("mst") or slave ("slv")
const char* desired_role = "slv";

//Which letter logs will be identified with
const char log_identifier = 'B';

//Whether extra debug information will be logged
const bool debug = false;

//Delay between reads, writes
const uint32_t sleep_time = 0x100;

//Thresholds determining dangerous values and trends
const float low_threshold = 3.5f;
const float high_threshold = 11.0f;

#pragma endregion

#pragma region data

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

#pragma endregion

#pragma region common

// Logs a message to log file (which is then output to UART by logger task)
void log(uint32_t log_fd, const char* message, char* log_buffer) {
	strncpy(log_buffer, message, strlen(message) + 1);
	log_buffer[1] = log_identifier;
	write(log_fd, log_buffer, 32);
}

// Logs current value, either on slave before sending it or on master after receiving it
void log_value(uint32_t log_fd, float value, char* log_buffer, char* float_buffer) {
	bzero(log_buffer, 32);
	strncpy(log_buffer, "[X] Value: ", 11);
	strncpy(log_buffer + 11, ftoa(value, float_buffer, 2), strlen(float_buffer));
	log(log_fd, log_buffer, log_buffer);
}

// Opens the i2c file and sets adresses, which opens the connection
uint32_t open_connection(uint32_t log_fd, char* log_buffer) {
	//Open file
	bzero(log_buffer, 32);
	strncpy(log_buffer, "DEV:i2c/X", 10);
	log_buffer[8] = i2c_interface;
	uint32_t i2c_fd = open(log_buffer, NFile_Open_Mode::Read_Write);

	//Check file
	if(i2c_fd == Invalid_Handle) {
		log(log_fd, "[X] Failed opening i2c file", log_buffer);
		terminate(1);
	}

	// Set addreses 
	TI2C_IOCtl_Params params;
	params.address = address;
	params.targetAddress = target_address;
	ioctl(i2c_fd, NIOCtl_Operation::Set_Params, &params);

	return i2c_fd;
}

// Determines the role of this task based on its and others prefered role, and adresses
// returns true for master, false for slave
bool select_role(uint32_t i2c_fd, char* msg_buffer, uint32_t log_fd, char* log_buffer) {
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
		if (debug) log(log_fd, "[XD] Both want the same role", log_buffer);

		return params.address < params.targetAddress;
	}

	return strncmp(desired_role, "mst", 3) == 0;
}

#pragma endregion

#pragma region master

// Receives data from slave
bool master_receive(uint32_t i2c_fd, char& type, float& value, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	uint32_t num_read = read(i2c_fd, msg_buffer, 6);
	if(num_read) {
		type = msg_buffer[0];
		value = *(float*) (msg_buffer+1);

		if(debug) {
			bzero(log_buffer, 32);
			strncpy(log_buffer, "[XD] Received: ", 16);
			log_buffer[15] = type;
			strncpy(log_buffer + 16, ftoa(value, float_buffer, 2), strlen(float_buffer));
			log(log_fd, log_buffer, log_buffer);
		}

		return true;
	} else if(debug) {
		log(log_fd, "[XD] Nothing to receive", log_buffer);
	}

	return false;
}

// Master logic
void master(uint32_t i2c_fd, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	char type;
	float value;

	while(true) {
		// Receive data and log results in loop
		bool received = master_receive(i2c_fd, type, value, msg_buffer, log_fd, log_buffer, float_buffer);
		if(received) {
			log_value(log_fd, value, log_buffer, float_buffer);

			if (type == 'd') {
				log(log_fd, "[X] Dangerous value", log_buffer);
			} else if (type == 't') {
				log(log_fd, "[X] Dangerous trend", log_buffer);
			} else {
				log(log_fd, "[X] OK", log_buffer);
			}
		}

		// Wait a bit
		sleep(sleep_time);
	}
}

#pragma endregion

#pragma region slave

// Predicts the next value using linear extrapolation
float slave_calculate_trend(float last_value, float current_value) {
	return lerp(last_value, current_value, 2);
}

// Checks whether value or trend is dangerous (not between the thresholds)
bool slave_is_dangerous(float value) {
	return value < low_threshold || value > high_threshold;
}

// Logs calculated trend on slave before sending it to master
void slave_log_trend(uint32_t log_fd, float trend, char* log_buffer, char* float_buffer) {
	bzero(log_buffer, 32);
	strncpy(log_buffer, "[X] Trend: ", 11);
	strncpy(log_buffer + 11, ftoa(trend, float_buffer, 2), strlen(float_buffer));
	log(log_fd, log_buffer, log_buffer);
}

// Sends data from slave to master
void slave_send(uint32_t i2c_fd, char type, float value, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	msg_buffer[0] = type;
	memcpy(&value, msg_buffer+1, 4);
	msg_buffer[5] = 0;
	write(i2c_fd, msg_buffer, 6);

	if(debug) {
		bzero(log_buffer, 32);
		strncpy(log_buffer, "[XD] Sent: ", 12);
		log_buffer[11] = type;
		strncpy(log_buffer + 12, ftoa(value, float_buffer, 2), strlen(float_buffer));
		log(log_fd, log_buffer, log_buffer);
	}
}

// Slave logic
void slave(uint32_t i2c_fd, char* msg_buffer, uint32_t log_fd, char* log_buffer, char* float_buffer) {
	float last_value = -1; // -1 ... not initialized
	float value;
	bool trend_set = false;
	float trend;
	char type;

	float* f_ptr = (float*) data;

	while(f_ptr < (data + data_len))
	{
		// Read next value
		value = *f_ptr;

		// Calculate trend if applicable
		if(last_value > 0) {
			trend = slave_calculate_trend(last_value, value);
			trend_set = true;
		} else {
			trend_set = false;
		}

		// Log current value and trend if applicable
		log_value(log_fd, value, log_buffer, float_buffer);
		if(trend_set) {
			slave_log_trend(log_fd, trend, log_buffer, float_buffer);
		} else {
			log(log_fd, "[X] Trend: -", log_buffer);
		}

		// Determine type of message
		if(slave_is_dangerous(value)) {
			type = 'd';
			log(log_fd, "[X] Dangerous value", log_buffer);
		} else if(trend_set && slave_is_dangerous(trend)) {
			type = 't';
			log(log_fd, "[X] Dangerous trend", log_buffer);
		} else {
			type = 'v';
			log(log_fd, "[X] OK", log_buffer);
		}

		// Send data via i2c
		slave_send(i2c_fd, type, value, msg_buffer, log_fd, log_buffer, float_buffer);

		// Set last value and move to next value
		last_value = value;
		f_ptr++;
		
		// Wait a bit
		sleep(sleep_time);
	}
}

#pragma endregion

int main(int argc, char** argv) {
	// Alloc buffers, variables
	char log_buffer[32];
	bzero(log_buffer, 32);

	char msg_buffer[6];
	bzero(msg_buffer, 6);

	char float_buffer[10];
	bzero(float_buffer, 10);

	// Get log pipe reference
	uint32_t log_fd = pipe("log", 128);
	log(log_fd, "[X] Task starting", log_buffer);

	// Open i2c connection
	uint32_t i2c_fd = open_connection(log_fd, log_buffer);

	log(log_fd, "[X] Connected", log_buffer);

	// Select role
	bool app_master = select_role(i2c_fd, msg_buffer, log_fd, log_buffer);
	if(debug) {
		if(app_master) {
			log(log_fd, "[XD] Application master", log_buffer);
		} else {
			log(log_fd, "[XD] Application slave", log_buffer);
		}
	}

	// Wait a bit
	sleep(sleep_time);

	//Start task based on selected role
	if(app_master) {
		master(i2c_fd, msg_buffer, log_fd, log_buffer, float_buffer);
	} else {
		slave(i2c_fd, msg_buffer, log_fd, log_buffer, float_buffer);
	}

	//Close files
	close(i2c_fd);
	close(log_fd);

    return 0;
}
