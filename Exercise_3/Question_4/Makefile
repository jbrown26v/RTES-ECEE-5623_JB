NCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O3 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

PRODUCT=question_4_a

HFILES=
CFILES= question_4_a.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

question_4_a:	question_4_a.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ question_4_a.o $(LIBS)

depend:

.c.o:
	$(CC) -MD $(CFLAGS) -c $<
