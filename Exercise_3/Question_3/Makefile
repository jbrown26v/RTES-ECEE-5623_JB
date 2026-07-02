ImNCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt

HFILES=

CFILES1= question_3_b.c 


SRCS1= ${HFILES} ${CFILES1}


OBJS1= ${CFILES1:.c=.o}


all: question_3_b 

clean:
	-rm -f *.o *.d *.exe question_3_b

question_3_b: question_3_b.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<
