CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
TARGET = tema1

SRC = main.cpp WordProcessor.cpp SortingUtility.cpp QueueManager.cpp Mapper.cpp Reducer.cpp TaskManager.cpp
OBJ = $(SRC:.cpp=.o)

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: build clean
