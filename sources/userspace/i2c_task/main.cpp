#include <stdstring.h>
#include <stdfile.h>
#include <stdmutex.h>

#include <drivers/gpio.h>
#include <process/process_manager.h>

/**
 * Task testujici I2C
 **/

constexpr uint32_t sleep_time = 0x10000;

int main(int argc, char** argv)
{
	uint32_t logpipe = pipe("log", 32);

	while (true)
	{
		write(logpipe, "I2C", 4);

		sleep(sleep_time);
	}

    return 0;
}
