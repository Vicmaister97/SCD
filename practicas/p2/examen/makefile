.SUFFIXES:
.PHONY: x1, x2, clean

compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp

x1: fuma_ex_exe
	./$<

x2: ejercicio2_exe
	./$<


fuma_ex_exe: fuma_ex.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $< HoareMonitor.cpp Semaphore.cpp

ejercicio2_exe: ejercicio2.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $<  HoareMonitor.cpp Semaphore.cpp

clean:
	rm -f *_exe
