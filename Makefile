CC 	=	gcc
CFLAGS	=	-O2 -fPIE -fstack-protector --param=ssp-buffer-size=4 \
	-Wall -W -Wshadow -Wformat-security \
	-D_FORTIFY_SOURCE=2 \

LINK	=	-Wl,-s
LDFLAGS	=	-fPIE -pie -Wl,-z,relro -Wl,-z,now

OBJS	=	main.o picoev_epoll.o

.c.o:
	$(CC) -c $*.c $(CFLAGS)

main: $(OBJS)
	$(CC) -o main $(OBJS) $(LINK) $(LDFLAGS)

clean:
	rm -f *.o *.swp main
