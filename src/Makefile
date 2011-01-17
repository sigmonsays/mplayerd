CC=gcc
BINARY=mplayerd

CFLAGS_NORMAL=-Iinc -g -Wall
CFLAGS_DEBUG=-Iinc -g -DDEBUG -Wall

#CFLAGS=${CFLAGS_DEBUG}
#CFLAGS=${CFLAGS_NORMAL}

MPLAYERD_OBJS=client_request.o ll.o mplayer_command.o mplayer_slave.o fileops.o client.o parse_config.o history.o xmemory.o debug.o tab_comp.o instance.o
MPLAYERD_CFLAGS=${CFLAGS_NORMAL}
MPLAYERD_LIBS=-lpthread

all: mplayerd

mplayerd: mplayerd.o ${MPLAYERD_OBJS} user_message_mplayerd
	gcc ${MPLAYERD_CFLAGS} ${MPLAYERD_LIBS} -o ${BINARY} ${MPLAYERD_OBJS} mplayerd.o

clean:
	rm -f ${MPLAYERC_OBJS} ${MPLAYERD_OBJS} ${BINARY} mplayerc mplayerd.o mplayerc.o test.o

install: all install_complete_message
	mkdir -p ${DESTDIR}${PREFIX}/man/man1/  ${DESTDIR}${PREFIX}/bin/
	install -m 0555 ${BINARY} ${DESTDIR}${PREFIX}/bin/
	install -m 0555 ../mplayerd.1 ${DESTDIR}${PREFIX}/man/man1/mplayerd.1

build: clean all

user_message_mplayerd:
	@echo -e "!!\r\n!!\r\n!!\tPlease report bugs to mplayerd-bugs@signuts.net"
	@echo -e "!!\r\n!!\tLinking mplayerd..."
	@echo -e "!!\r\n"
	@echo -e "!!Do a 'make install_config' or just copy the config file"
	@echo -e "!!\r\n"

install_complete_message:
	@echo -e "!!\r\n!!\tBe sure to edit /etc/mplayerd.conf, consult the README"
	@echo -e "!!\tand don't forget to report bugs to mplayerd-bugs@signuts.net!"
	@echo -e "!!\r\n"

uninstall:
	rm -f /usr/local/bin/mplayerd

install_config:
	install -m 0555 ../mplayerd.conf ${DESTDIR}${PREFIX}/etc/mplayerd.conf
