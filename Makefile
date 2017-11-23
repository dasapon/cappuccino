SOURCES := $(shell ls *.cpp)
HEADERS := $(shell ls *.hpp)
LIBRARIES := -lpthread
OPTIONS := -std=c++14 -Wall -DNDEBUG -O3 -march=native
BIN_DIR := ../bin
CC := clang++
EXE := cappuccino
OBJ_DIR := ../obj/
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

all: $(BIN_DIR)/$(EXE)

-include $(OBJECTS:%.o=%.d)

$(BIN_DIR)/$(EXE): $(OBJECTS)
	if [ ! -d $(BIN_DIR) ] ; then mkdir -p $(BIN_DIR); fi
	$(CC) $(OPTIONS) -o $(BIN_DIR)/$(EXE) $(OBJECTS) $(LIBRARIES)
.PHONY:clean
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(OBJ_DIR)/*.d

$(OBJ_DIR)/%.o:%.cpp
	if [ ! -d $(OBJ_DIR) ] ; then mkdir -p $(OBJ_DIR); fi
	$(CC) $(OPTIONS) -c -MMD -MP -MF $(@:%.o=%.d) -o $@ $<


