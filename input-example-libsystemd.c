// libsystemd-based example for handling input change events

#include "sysfs-gpio.h"
#include <stdio.h>
#include <systemd/sd-event.h>

static int gpio_changed_handler( sd_event_source *s, int fd, uint32_t revents, void *userdata )
{
	struct Gpio const *gpio = (struct Gpio const *)userdata;

	printf( "%d\n", gpio_read( gpio ) );

	return 0;
}

int main()
{
	// FIXME ought to check return values of the sd_event calls

	// NOTE: cleanup-attribute is used to automatically release resources at end of scope.

	// get default event loop.
	sd_event *evloop __attribute__(( cleanup( sd_event_unrefp ) )) = NULL;
	sd_event_default( &evloop );

	// open input gpio and add event handler for edge events
	struct Gpio gpio __attribute__(( cleanup( gpio_close ) )) = GPIO_INITIALIZER;
	gpio_open_input( &gpio, "/sys/class/gpio/gpio60", GPIO_EDGE_BOTH, GPIO_RDONLY );
	sd_event_add_io( evloop, NULL, gpio.fd, EPOLLPRI, gpio_changed_handler, &gpio );

	// print current input value
	printf( "%d\n", gpio_read( &gpio ) );

	// run main event loop
	return sd_event_loop( evloop );
}
