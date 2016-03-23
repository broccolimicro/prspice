all: prspice prdbase

prspice: src/common.cpp src/hash.cpp src/dbase.cpp src/prspice.cpp
	g++ -g src/common.cpp src/hash.cpp src/dbase.cpp src/prspice.cpp -o prspice

prdbase: src/common.cpp src/hash.cpp src/dbase.cpp src/prdbase.cpp
	g++ -g src/common.cpp src/hash.cpp src/dbase.cpp src/prdbase.cpp -o prdbase

clean:
	rm prspice
	rm prdbase
