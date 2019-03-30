.SUFFIXES:
.PHONY: pc
.PRECIOUS: %_exe

compilador:=g++

pc: prodcons_exe fumadores_exe estancos2_exe
	./$<	# si quiero que al hacer make se ejecuten   

%_exe: %.cpp
		$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp 

clean:
	rm -f *_exe
