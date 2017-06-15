CFLAGS += -Wall -Wextra -g -O0
legex: main.o legex.o subst.o test.o
main.o: legex.h
legex.o: legex.h
subst.o: legex.h
test.o: legex.h
