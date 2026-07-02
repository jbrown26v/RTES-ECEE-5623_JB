ImNCLUDE_DIRS = 
LIB_DIRS = 

CDEFS=
CFLAGS= -O -g $(INCLUDE_DIRS) $(CDEFS) -DLINUX
LIBS=-lpthread -lrt -lm

HFILES=

CFILES1= question_2_c.c 


SRCS1= ${HFILES} ${CFILES1}


OBJS1= ${CFILES1:.c=.o}


all: question_2_c 

clean:
	-rm -f *.o *.d *.exe question_2_c

question_2_c: question_2_c.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS1) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<