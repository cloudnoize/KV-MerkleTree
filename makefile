# Compiler and Flags
CXX = g++-11
CXXFLAGS = -std=c++23 -I/usr/local/include -Wall -g -MMD -MP
LDFLAGS = -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lgtest -lgtest_main -pthread -lcrypto

# Directories
ROOT_SRC_DIR   = .
DETAIL_SRC_DIR = detail
TEST_SRC_DIR   = tests
BUILD_DIR      = build
OBJ_DIR        = $(BUILD_DIR)/obj
ROOT_OBJ_DIR   = $(OBJ_DIR)
DETAIL_OBJ_DIR = $(OBJ_DIR)/detail
TEST_OBJ_DIR   = $(OBJ_DIR)/tests

# Output executables
KEY_TEST_EXECUTABLE = $(BUILD_DIR)/key_tests
TREE_TEST_EXECUTABLE = $(BUILD_DIR)/tree_tests
NODES_TEST_EXECUTABLE = $(BUILD_DIR)/nodes_tests

# Source and Object Files
DETAIL_SOURCES = $(wildcard $(DETAIL_SRC_DIR)/*.cpp)
DETAIL_OBJECTS = $(patsubst $(DETAIL_SRC_DIR)/%.cpp, $(DETAIL_OBJ_DIR)/%.o, $(DETAIL_SOURCES))

ROOT_SOURCES = $(wildcard $(ROOT_SRC_DIR)/*.cpp)
ROOT_OBJECTS = $(patsubst $(ROOT_SRC_DIR)/%.cpp, $(ROOT_OBJ_DIR)/%.o, $(ROOT_SOURCES))

KEY_TEST_SOURCE = $(TEST_SRC_DIR)/key_utils_tests.cpp
KEY_TEST_OBJECT = $(TEST_OBJ_DIR)/key_utils_tests.o

TREE_TEST_SOURCE = $(TEST_SRC_DIR)/tree_tests.cpp
TREE_TEST_OBJECT = $(TEST_OBJ_DIR)/tree_tests.o

NODES_TEST_SOURCE = $(TEST_SRC_DIR)/nodes_tests.cpp
NODES_TEST_OBJECT = $(TEST_OBJ_DIR)/nodes_tests.o

# Dependencies
DEPS = $(DETAIL_OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

# All build target
all: $(KEY_TEST_EXECUTABLE) $(TREE_TEST_EXECUTABLE) $(NODES_TEST_EXECUTABLE)

# Link all object files into the final executable
$(KEY_TEST_EXECUTABLE): $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(KEY_TEST_OBJECT)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(KEY_TEST_OBJECT) $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(LDFLAGS) -o $@

# Link tree test object file into a dedicated executable
$(TREE_TEST_EXECUTABLE): $(TREE_TEST_OBJECT) $(DETAIL_OBJECTS) $(ROOT_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TREE_TEST_OBJECT) $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(LDFLAGS) -o $@

	# Link tree test object file into a dedicated executable
$(NODES_TEST_EXECUTABLE): $(NODES_TEST_OBJECT) $(DETAIL_OBJECTS) $(ROOT_OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(NODES_TEST_OBJECT) $(DETAIL_OBJECTS) $(ROOT_OBJECTS) $(LDFLAGS) -o $@

# Compile detail directory
$(DETAIL_OBJ_DIR)/%.o: $(DETAIL_SRC_DIR)/%.cpp
	@mkdir -p $(DETAIL_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile root directory
$(ROOT_OBJ_DIR)/%.o: $(ROOT_SRC_DIR)/%.cpp
	@mkdir -p $(ROOT_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Compile the tree test file
$(KEY_TEST_OBJECT): $(KEY_TEST_SOURCE)
	@mkdir -p $(TEST_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile the tree test file
$(TREE_TEST_OBJECT): $(TREE_TEST_SOURCE)
	@mkdir -p $(TEST_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile the tree test file
$(NODES_TEST_OBJECT): $(NODES_TEST_SOURCE)
	@mkdir -p $(TEST_OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean all generated files
clean:
	rm -rf $(BUILD_DIR)

# Include dependencies for incremental builds
-include $(DEPS)
