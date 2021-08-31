// basic example for handling input change events

#include "sysfs-gpio.h"
#include <stdio.h>

int main()
{
	struct Gpio gpio = GPIO_INITIALIZER;
	gpio_open_input( &gpio, "/sys/class/gpio/gpio60", GPIO_EDGE_BOTH, false );

	int const timeout_ms = 3000;  // -1 = no timeout

	do {
		printf( "%d\n", gpio_read( &gpio ) );
	} while( gpio_wait_event( &gpio, timeout_ms ) );

	printf( "no edge event detected in %d milliseconds.\n", timeout_ms );

	gpio_close( &gpio );
	return 0;
}
