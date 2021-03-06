#include "sysfs-gpio.h"

#define _GNU_SOURCE 1
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


//-------- error handling --------------------------------------------------------------------------------------

__attribute__(( noreturn, always_inline, format( printf, 1, 2 ) ))
static inline void die( char const *fmt, ... )
{
	fprintf( stderr, fmt, __builtin_va_arg_pack() );
	exit( EXIT_FAILURE );
}


//-------- internal helper functions ---------------------------------------------------------------------------

static int _gpio_open_attribute( struct Gpio const *gpio, char const *attr, int mode )
{
	int afd = openat( gpio->pathfd, attr, mode | O_CLOEXEC );
	if( afd < 0 )
		die( "open %s/%s: %m\n", gpio->path, attr );
	return afd;
}

static char _gpio_read_attribute( struct Gpio const *gpio, char const *attr )
{
	int afd = _gpio_open_attribute( gpio, attr, O_RDONLY );
	char c = 0;
	if( read( afd, &c, 1 ) < 0 )
		die( "read %s/%s: %m\n", gpio->path, attr );
	close( afd );
	return c;
}

static void _gpio_write_attribute( struct Gpio const *gpio, char const *attr, char const *value )
{
	int afd = _gpio_open_attribute( gpio, attr, O_WRONLY );
	if( write( afd, value, strlen( value ) ) < 0 )
		die( "write %s/%s: %m\n", gpio->path, attr );
	close( afd );
}


//-------- public library functions ----------------------------------------------------------------------------

// open gpio, do not change current direction (or value, if output).
// for backwards compatibility, mode 0 and 1 are interpreted as GPIO_RDWR and GPIO_RDONLY respectively.
void gpio_open( struct Gpio *gpio, char const *path, enum GpioOpenMode mode )
{
	if( mode == (enum GpioOpenMode)0 )
		mode = GPIO_RDWR;
	else if( mode == (enum GpioOpenMode)1 )
		mode = GPIO_RDONLY;
	assert( mode == GPIO_RDONLY || mode == GPIO_RDWR );

	assert( path != NULL );
	gpio->path = strdup( path );
	if( gpio->path == NULL )
		die( "strdup: %m\n" );

	gpio->pathfd = open( path, O_PATH | O_CLOEXEC );
	if( gpio->pathfd < 0 )
		die( "open %s: %m\n", path );

	gpio->fd = _gpio_open_attribute( gpio, "value", mode == GPIO_RDONLY ? O_RDONLY : O_RDWR );
}

// open gpio and initialize as input.
// for backwards compatibility, mode 0 and 1 are interpreted as GPIO_RDONLY and GPIO_RDWR respectively.
void gpio_open_input( struct Gpio *gpio, char const *path, enum GpioEdge event_edge, enum GpioOpenMode mode )
{
	if( mode == (enum GpioOpenMode)0 )
		mode = GPIO_RDONLY;
	else if( mode == (enum GpioOpenMode)1 )
		mode = GPIO_RDWR;
	gpio_open( gpio, path, mode );

	if( gpio_is_output( gpio ) )
		gpio_set_direction_input( gpio );

	gpio_set_event_edge( gpio, event_edge );
}

// open gpio and initialize as output.
void gpio_open_output( struct Gpio *gpio, char const *path, enum GpioValue init_value )
{
	gpio_open( gpio, path, GPIO_RDWR );

	if( gpio_is_output( gpio ) )
		gpio_write( gpio, init_value );
	else
		gpio_set_direction_output( gpio, init_value );
}

// close gpio.  it is safe to call this on a zero-filled struct Gpio.  it is safe to call this more than once.
void gpio_close( struct Gpio *gpio )
{
	if( gpio->path == NULL )
		return;

	if( gpio->fd >= 0 ) {
		close( gpio->fd );
		gpio->fd = -1;
	}

	if( gpio->pathfd >= 0 ) {
		close( gpio->pathfd );
		gpio->pathfd = -1;
	}

	free( gpio->path );
	gpio->path = NULL;
}

// check if gpio is configured as active-low (meaning value is opposite of signal level).
bool gpio_is_active_low( struct Gpio const *gpio )
{
	char c = _gpio_read_attribute( gpio, "active_low" );
	assert( c == '0' || c == '1' );
	return c & 1;
}

// get current input value or output value (depending on direction).
enum GpioValue gpio_read( struct Gpio const *gpio )
{
	char c = 0;
	ssize_t res = pread( gpio->fd, &c, 1, 0 );
	if( res < 0 )
		die( "pread %s/value: %m\n", gpio->path );
	assert( c == '0' || c == '1' );
	return (enum GpioValue)( c & 1 );
}

// set output value.  will fail if direction is not set to output or gpio was opened as input-only.
void gpio_write( struct Gpio const *gpio, enum GpioValue value )
{
	assert( value == 0 || value == 1 );
	char c = '0' + value;
	if( pwrite( gpio->fd, &c, 1, 0 ) < 0 )
		die( "pwrite %s/value: %m\n", gpio->path );
}

// check gpio direction.
bool gpio_is_output( struct Gpio *const gpio )
{
	char c = _gpio_read_attribute( gpio, "direction" );
	assert( c == 'i' || c == 'o' );
	return c == 'o';
}

// change direction to input.  will fail if gpio is not bidirectional.
void gpio_set_direction_input( struct Gpio const *gpio )
{
	_gpio_write_attribute( gpio, "direction", "in" );
}

// change direction to output.  will fail if gpio is not bidirectional.
// will also fail if event edge is configured to anything other than GPIO_EDGE_NONE.
void gpio_set_direction_output( struct Gpio const *gpio, enum GpioValue init_value )
{
	assert( init_value == 0 || init_value == 1 );
	bool init_level = gpio_is_active_low( gpio ) ^ init_value;
	_gpio_write_attribute( gpio, "direction", init_level ? "high" : "low" );
}

// change which edge(s) trigger an event (POLLPRI on gpio->fd).
// will fail if gpio direction is output and event_edge != GPIO_EDGE_NONE.
void gpio_set_event_edge( struct Gpio const *gpio, enum GpioEdge event_edge )
{
	static char const *const edge_names[] = { "none", "rising", "falling", "both" };
	assert( (unsigned)event_edge < sizeof(edge_names)/sizeof(edge_names[0]) );
	_gpio_write_attribute( gpio, "edge", edge_names[ event_edge ] );
}

// wait for configured edge event (POLLPRI on gpio->fd) or timeout.
// timeout has same meaning as for poll().
// returns true if edge event was detected, false on timeout.
bool gpio_wait_event( struct Gpio const *gpio, int timeout )
{
	struct pollfd pfd = { gpio->fd, POLLPRI, 0 };
	int result = poll( &pfd, 1, timeout );
	if( result < 0 )
		die( "poll %s/value: %m\n", gpio->path );
	return result;
}
