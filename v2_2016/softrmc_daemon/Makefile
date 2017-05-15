#@author snovakov
SHELL = /bin/sh
CC    = g++

CFLAGS	= -I$(LIBSONUMA_PATH)

TARGET  = rmcd

SOURCES = \
	./rmcd.c

HEADERS = \
	$(LIBSONUMA_PATH)/RMCdefines.h

OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	cp ./rmcd ../bin/

clean:
	-${RM} ${TARGET} ${OBJECTS} $(SRCS:.c=.o)
