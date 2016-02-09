all: prspice prdbase

prspice:
	g++ src/common.cpp src/dbase.cpp src/prspice.cpp -o prspice

prdbase:
	g++ src/common.cpp src/dbase.cpp src/prdbase.cpp -o prdbase

