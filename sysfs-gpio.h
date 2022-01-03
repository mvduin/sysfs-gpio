#pragma once
#include <stdbool.h>

// use one of the gpio_open* functions to initialize this structure and use gpio_close() to clean it up.
struct Gpio {
	int fd;		// of value attribute (gets POLLPRI event when configured edge is detected)
	int pathfd;	// of gpio directory
	char *path;	// of gpio directory
};

#define GPIO_INITIALIZER ((struct Gpio){ -1, -1, NULL })

enum GpioEdge {
	GPIO_EDGE_NONE,
	GPIO_EDGE_RISING,
	GPIO_EDGE_FALLING,
	GPIO_EDGE_BOTH,
};


// open gpio, do not change current direction (or value, if output).
// if input_only is true, gpio_write() will not be supported.
void gpio_open( struct Gpio *gpio, char const *path, bool input_only );

// open gpio and initialize as input.
// if bidirectional is false, gpio will be opened as input-only.
void gpio_open_input( struct Gpio *gpio, char const *path, enum GpioEdge event_edge, bool bidirectional );

// open gpio and initialize as output.
void gpio_open_output( struct Gpio *gpio, char const *path, bool init_value );

// close gpio.  it is safe to call this on a zero-filled struct Gpio.  it is safe to call this more than once.
void gpio_close( struct Gpio *gpio );


// check if gpio is configured as active-low (meaning value is opposite of signal level).
bool gpio_is_active_low( struct Gpio const *gpio );


// get current input value or output value (depending on direction).
bool gpio_read( struct Gpio const *gpio );

// set output value.  will fail if direction is not set to output or gpio was opened as input-only.
void gpio_write( struct Gpio const *gpio, bool value );


// check gpio direction.
bool gpio_is_output( struct Gpio *const gpio );

// change direction to input.  will fail if gpio is not bidirectional.
void gpio_set_direction_input( struct Gpio const *gpio );

// change direction to output.  will fail if gpio is not bidirectional.
// will also fail if event edge is configured to anything other than GPIO_EDGE_NONE.
void gpio_set_direction_output( struct Gpio const *gpio, bool init_value );


// change which edge(s) trigger an event (POLLPRI on gpio->fd).
// will fail if gpio direction is output and event_edge != GPIO_EDGE_NONE.
void gpio_set_event_edge( struct Gpio const *gpio, enum GpioEdge event_edge );

// wait for edge event configured using gpio_set_event_edge.
// timeout has same meaning as for poll().
// returns true if edge event was detected, false on timeout.
//
// this is just a simple wrapper for poll() for the simple use-case of waiting on a single gpio.  for more
// complex use-cases, just use poll(), epoll, or your favorite event loop to receive POLLPRI events on
// gpio->fd for each gpio you want to monitor.
bool gpio_wait_event( struct Gpio const *gpio, int timeout );
