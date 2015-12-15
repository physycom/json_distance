EXE=json_distance.exe

all:
	$(CXX) -std=c++11 -I. -o $(EXE) json_distance.cpp 
