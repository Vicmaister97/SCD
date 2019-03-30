.SUFFIXES:
.PHONY:    pc,clean

compilador := mpicxx -std=c++11
flagsc     := -g -Wall

pc: prodcons_exe
	mpirun -np 6 ./$<

%_exe: %.cpp
	$(compilador) $(flagsc) -o $@ $<

clean:
	rm -rf *_exe *.dSYM
