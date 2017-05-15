#@author snovakov
SHELL = /bin/sh
CC    = g++

CFLAGS	= -I$(LIBSONUMA_PATH)
LDFLAGS	= -L../libsonuma -lsonuma -lrt

TARGET  = rread_sync

SOURCES = \
	./rread_sync.c

OBJECTS = $(SOURCES:.c=.o)

TARGET1  = rread_server

SOURCES1 = \
	./rread_server.c

OBJECTS1 = $(SOURCES1:.c=.o)

rread_sync: $(TARGET) $(TARGET1)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJECTS) 
	cp ./rread_sync ../bin/
#rread_server: $(TARGET1)

$(TARGET1): $(OBJECTS1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET1) $(OBJECTS1)
	cp ./rread_server ../bin/

clean:
	-${RM} ${TARGET} ${TARGET1} ${OBJECTS} ${OBJECTS1} $(SRCS:.c=.o)
