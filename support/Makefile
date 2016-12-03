all:
	vcs -full64 +vpi +cli+3 -use_vpiobj vpi.o -load libvcshsim.so:cs_vpi_startup -LDFLAGS "-L/usr/lib -L${HOME}/lib" -lprsim -lvlsilib -ledit -lm test.v
	./simv +nsda+cosim.cfg

clean:
	rm -rf simv*
	rm -rf csrc
	rm -rf hsim*
	rm -f nsda_cosim.sp
	rm -f ucli.key
	rm -f *.log
