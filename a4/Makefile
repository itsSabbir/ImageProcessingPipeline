# You should change the value of PORT
PORT = 54740
CC = gcc
CFLAGS =  -DPORT=${PORT} -g -Wall -std=gnu99 


# Note that this Makefile populates the images/ and filters/ directories
# for the server.
all: image_server images filters

image_server: image_server.o response.o request.o socket.o
	${CC} ${CFLAGS} -o $@ $^


.c.o: response.h request.h socket.h
	${CC} ${CFLAGS}  -c $<

images:
	mkdir images
	cp dog.bmp images

filters:
	mkdir filters
	cp copy filters

clean:
	rm *.o image_server
