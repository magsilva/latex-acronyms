CC=gcc
CFLAGS=-std=c99 -Wall -O2 `mysql_config --cflags`
LIBS=`mysql_config --libs`

default:
	$(CC) $(CFLAGS)  make-acronyms.c -o make-acronyms $(LIBS)

clean:
	rm -f make-acronyms acronyms.tex
