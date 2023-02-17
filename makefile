main.out: delta_stepping.cpp
	mpic++ -o main.out delta_stepping.cpp

clean:
	rm -f main.out