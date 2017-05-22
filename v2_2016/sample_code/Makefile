#
# Scale-Out NUMA Open Source License
#
# Copyright (c) 2017, Parallel Systems Architecture Lab, EPFL
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:

# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of the Parallel Systems Architecture Lab, EPFL,
#   nor the names of its contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LAB,
# EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

SHELL = /bin/sh
CC    = g++

CFLAGS	= -I$(LIBSONUMA_PATH)
LDFLAGS	= -L../libsonuma -lsonuma -lrt

TARGET  = bench_sync

SOURCES = \
	./bench_sync.c

OBJECTS = $(SOURCES:.c=.o)

TARGET1  = bench_server

SOURCES1 = \
	./bench_server.c

OBJECTS1 = $(SOURCES1:.c=.o)

TARGET2  = bench_async

SOURCES2 = \
	./bench_async.c

OBJECTS2 = $(SOURCES2:.c=.o)

bench_sync: $(TARGET) $(TARGET1) $(TARGET2)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJECTS) 
	cp ./bench_sync ../bin/
#rread_server: $(TARGET1)

$(TARGET1): $(OBJECTS1)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET1) $(OBJECTS1)
	cp ./bench_server ../bin/

$(TARGET2): $(OBJECTS2)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET2) $(OBJECTS2)
	cp ./bench_async ../bin/

clean:
	-${RM} ${TARGET} ${TARGET1} ${TARGET2} ${OBJECTS} ${OBJECTS1} ${OBJECTS2} $(SRCS:.c=.o)
