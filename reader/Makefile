CXX = g++
EXEC = gbcr

all: $(EXEC)

$(EXEC): reader.cpp
	$(CXX) -o $(EXEC) -O2 -std=c++11 -lboost_system reader.cpp

clean:
	rm -v $(EXEC)

