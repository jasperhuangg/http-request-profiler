profiler: main.o get.o
	g++ main.o get.o -o profiler

main.o: main.cpp
	g++ -c main.cpp

get.o: get.cpp
	g++ -c get.cpp

clean:
	rm *.o profiler