# Default CC
CC = gcc

# Uncomment for below 3 lines compilation on Irix,
# and omment out all other lines in this file.
#CC = cc
#upclient: upclient.c config.h
#	$(CC) -fullwarn -o upclient upclient.c

# Uncomment for Solaris
#OPTS = -lsocket -lnsl

#Uncomment these 2 lines for OS/2
#OPTS = -Zomf
#OPTS2 = upclient.def -lsocket -lsi2
#EXT = .exe

upclient: upclient.c config.h
	$(CC) $(OPTS) -Wall -o upclient$(EXT) upclient.c $(OPTS2)
