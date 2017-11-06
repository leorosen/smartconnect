CFLAGS=-Wall -ansi -O4 -g
CFLAGS+=-D_UNITEST 
# CFLAHS+= -D_DEBUG
# CFLAGS+=-DSMART_CONN_CACHE_SIZE=0

all:	smart_connect

clean:
	rm smart_connect 
