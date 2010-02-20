CC=gcc
CFLAGS=-std=c99 -Wall -O2
LIBS=`mysql_config --cflags --libs`

default:
	$(CC) $(CFLAGS) $(LIBS) make-acronyms.c -o make-acronyms

