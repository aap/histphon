CFLAGS += -Wall -Wextra -g -O0
sc_test: sc_test.o legscript.o tree.o phon.o
lx_test: lx_test.o phon.o legex.o subst.o
phon.o: legex.h
legex.o: legex.h
subst.o: legex.h
tree.o: legex.h
legscript.o: legex.h
lx_test.o: legex.h
sc_test.o: legex.h

clean:
	rm *.o
