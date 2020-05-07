CXX      := -c++
CXXFLAGS := -std=c++17 -Wno-literal-suffix -Wreturn-type -Wno-deprecated-declarations -Wno-invalid-offsetof -Wparentheses -g
LDFLAGS  := -L/usr/lib -lstdc++ -lm
OBJ_DIR  := obj
APP_DIR  := ..
DEP_DIR  := dep
TARGET   := epidemia_fast
INCLUDE  := -I.
LIBS     := -lboost_thread -lboost_filesystem -lboost_regex -lboost_system -lboost_date_time -lboost_program_options -lpthread
DEBUG_FLAG := $(strip $(wildcard DEBUG))

ifdef DEBUG_FLAG
OPT    := -O0 -DDEBUG
else
OPT     := -O3
endif

SRC      := \
	agent.cpp \
	agent_manager.cpp \
	city.cpp \
	cluster.cpp \
	command.cpp \
	enum_helper.cpp \
	geometry.cpp \
	infection_counter.cpp \
	log_output.cpp \
	main.cpp \
	person.cpp \
	properties.cpp \
	random.cpp \
	utility.cpp \
	world.cpp \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPS     := $(SRC:%.cpp=$(DEP_DIR)/%.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: $(DEP_DIR)/%.d

$(DEP_DIR)/%.d: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(OPT) $(INCLUDE) -MM -MF $@ $<
	sed -i 's#.*\.o:#$(OBJ_DIR)/&#' $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(OPT) $(INCLUDE) -c $< -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS) $(LIBS)

.PHONY: all build clean superclean debug release touch_debug rm_debug

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug:	OPT := -O0 -DDEBUG
debug:  touch_debug all

touch_debug:
	-@touch DEBUG

release: OPT := -O3
release: rm_debug all

rm_debug:
	-@rm -f DEBUG

superclean:	clean
	-@rm -rvf $(DEP_DIR)/*

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -vf $(APP_DIR)/$(TARGET)

-include $(DEPS)

