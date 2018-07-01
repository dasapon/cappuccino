SOURCES := $(shell ls *.cpp)
HEADERS := $(shell ls *.hpp)
LIBRARIES := -lpthread
OPTIONS := -std=c++14 -Wall -DNDEBUG -O3 -march=native
BIN_DIR := ../bin
CC := clang++
EXE := cappuccino
LEXE := cappuccino_learn
OBJ_DIR := ../obj
LOBJ_DIR := ../learnobj
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
LOBJECTS = $(SOURCES:%.cpp=$(LOBJ_DIR)/%.o)

release: $(BIN_DIR)/$(EXE)

-include $(OBJECTS:%.o=%.d)

$(BIN_DIR)/$(EXE): $(OBJECTS)
	if [ ! -d $(BIN_DIR) ] ; then mkdir -p $(BIN_DIR); fi
	$(CC) $(OPTIONS) -o $(BIN_DIR)/$(EXE) $(OBJECTS) $(LIBRARIES)
	
learn: $(BIN_DIR)/$(LEXE)

-include $(LOBJECTS:%.o=%.d)

$(BIN_DIR)/$(LEXE): $(LOBJECTS)
	if [ ! -d $(BIN_DIR) ] ; then mkdir -p $(BIN_DIR); fi
	$(CC) $(OPTIONS) -DLEARN -o $(BIN_DIR)/$(LEXE) $(LOBJECTS) $(LIBRARIES)
	
.PHONY:test
test: release learn
	$(BIN_DIR)/$(EXE) unit_test

.PHONY:clean
clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(OBJ_DIR)/*.d
	rm -f $(LOBJ_DIR)/*.o
	rm -f $(LOBJ_DIR)/*.d

$(OBJ_DIR)/%.o:%.cpp
	if [ ! -d $(OBJ_DIR) ] ; then mkdir -p $(OBJ_DIR); fi
	$(CC) $(OPTIONS) -c -MMD -MP -MF $(@:%.o=%.d) -o $@ $<

$(LOBJ_DIR)/%.o:%.cpp
	if [ ! -d $(LOBJ_DIR) ] ; then mkdir -p $(LOBJ_DIR); fi
	$(CC) $(OPTIONS) -DLEARN -c -MMD -MP -MF $(@:%.o=%.d) -o $@ $<
