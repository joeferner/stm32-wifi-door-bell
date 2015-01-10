CC = gcc
CXX = g++
CFLAGS ?= -Wall -g3 -O0
CXXFLAGS ?= -Wall
LDFLAGS ?= -static

all: test-all

test-all: test-rsa.c
	$(CC) -I../ $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) \
	../polarssl/base64.c \
	../polarssl/bignum.c \
	../polarssl/asn1parse.c \
	../polarssl/rsa.c \
	../polarssl/pem.c \
	../polarssl/md.c \
	../polarssl/md5.c \
	../polarssl/oid.c \
	../polarssl/error.c \
	../polarssl/havege.c \
	../polarssl/memory_buffer_alloc.c \
	../polarssl/timing.c \
	../polarssl/pk.c \
	../polarssl/pkcs5.c \
	../polarssl/pkcs12.c \
	../polarssl/pk_wrap.c \
	../polarssl/pkparse.c \
	../polarssl/platform.c \
	$< -o $@
