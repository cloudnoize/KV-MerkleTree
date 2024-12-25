# Compiler and Flags
CXX = g++-11
CXXFLAGS = -std=c++2a -I/usr/local/include -Wall -g -MMD -MP
LDFLAGS = -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lgtest -lgtest_main -pthread -lcrypto

# Directories
ROOT_SRC_DIR   = .
DETAIL_SRC_DIR = detail
TEST_SRC_DIR   = tests
BUILD_DIR      = build
OBJ_DIR        = $(BUILD_DIR)/obj
ROOT_OBJ_DIR = $(OBJ_DIR)
DETAIL_OBJ_DIR = $(OBJ_DIR)/detail
TEST_OBJ_DIR   = $(OBJ_DIR)/tests

# Output executable
EXECUTABLE = $(BUILD_DIR)/run_tests

# Source and Object Files
DETAIL_SOURCES = $(wildcard $(DETAIL_SRC_DIR)/*.cpp)
DETAIL_OBJECTS = $(patsubst $(DETAIL_SRC_DIR)/%.cpp, $(DETAIL_OBJ_DIR)/%.o, $(DETAIL_SOURCES))

ROOT_SOURCES = $(wildcard $(ROOT_SRC_DIR)/*.cpp)
ROOT_OBJECTS = $(patsubst $(ROOT_SRC_DIR)/%.cpp, $(ROOT_OBJ_DIR)/%.o, $(ROOT_SOURCES))

TEST_SOURCES = $(wildcard $(TEST_SRC_DIR)/*.cpp)
TEST_OBJECTS = $(patsubst $(TEST_SRC_DIR)/%.cpp, $(TEST_OBJ_DIR)/%.o, $(TEST_SOURCES))

# Dependencies
DEPS = $(DETAIL_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

# All build target
all: $(EXECUTABLE)

# Link all object files into the final executable
$(EXECUTABLE): $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(TEST_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(TEST_OBJECTS) $(LDFLAGS) -o $@

# Compile detail directory
$(DETAIL_OBJ_DIR)/%.o: $(DETAIL_SRC_DIR)/%.cpp
	@mkdir -p $(DETAIL_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile root directory
$(ROOT_OBJ_DIR)/%.o: $(ROOT_SRC_DIR)/%.cpp
	@mkdir -p $(ROOT_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test directory
$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	@mkdir -p $(TEST_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean all generated files
clean:
	rm -rf $(BUILD_DIR)

# Include dependencies for incremental builds
-include $(DEPS)
