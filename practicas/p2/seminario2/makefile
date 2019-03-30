.SUFFIXES:
.PHONY: x1, x2, clean

compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp

x5: prodcons1_sc_exe
	./$<

x6: prodcons2_sc_exe
	./$<

x7: prodcons2_su_exe
	./$<


prodcons1_sc_exe: prodcons1_sc.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $<

prodcons2_sc_exe: prodcons2_sc.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $<

prodcons2_su_exe: prodcons2_su.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)  -o $@ $<  HoareMonitor.cpp Semaphore.cpp

clean:
	rm -f *_exe
