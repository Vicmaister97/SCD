.SUFFIXES:
.PHONY: x1, x2, clean

compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp

x1: fumadores_su_exe
	./$<

x2: barberia_exe
	./$<


fumadores_su_exe: fumadores_su.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $< HoareMonitor.cpp Semaphore.cpp

barberia_exe: barberia.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $<  HoareMonitor.cpp Semaphore.cpp

clean:
	rm -f *_exe
