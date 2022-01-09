ifneq ($(shell pkg-config --exists libsystemd && echo 1),)
libsystemd_CFLAGS != pkg-config --cflags libsystemd
libsystemd_LDLIBS != pkg-config --libs libsystemd
endif

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g -Og
CXXFLAGS = -Wall -Wextra -Wno-unused-parameter -g -Og -std=gnu++17

programs := input-example-basic input-example-libsystemd

all :: ${programs}

ifdef libsystemd_LDLIBS
input-example-libsystemd.o:  private CFLAGS+=${libsystemd_CFLAGS}
input-example-libsystemd:  private LDLIBS+=${libsystemd_LDLIBS}
else
input-example-libsystemd:
	$(warning libsystemd-dev not installed, not building input-example-libsystemd)
endif

${programs}: sysfs-gpio.o

clean ::
	${RM} ${programs} *.o

.DELETE_ON_ERROR:
