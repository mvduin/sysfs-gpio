libsystemd_CFLAGS != pkg-config --cflags libsystemd
libsystemd_LDLIBS != pkg-config --libs libsystemd

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g -Og
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -g -Og -std=gnu++17

programs := input-example-basic input-example-libsystemd

all :: ${programs}

input-example-libsystemd.o:  private CFLAGS+=${libsystemd_CFLAGS}
input-example-libsystemd:  private LDLIBS+=${libsystemd_LDLIBS}

${programs}: sysfs-gpio.o

clean ::
	${RM} ${programs} *.o

.DELETE_ON_ERROR:
