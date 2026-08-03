CXX ?= c++
CPPFLAGS := -I./mocks -I../..
CXXFLAGS := -std=gnu++11 -Wall -Wextra -Werror -pedantic
CONTROLLER_SOURCES := \
	../../protocol.cpp \
	../../controller_state.cpp \
	../../serial_transport.cpp \
	../../can_transport.cpp \
	../../debounced_button.cpp \
	../../controller_ui.cpp
CONTROLLER_HEADERS := ../../controller.ino $(wildcard ../../*.h)
BUILD_DIR := build
TEST_BINARY := $(BUILD_DIR)/controller_tests
SANITIZED_BINARY := $(BUILD_DIR)/controller_tests_sanitized

.PHONY: test sanitize

test: $(TEST_BINARY)
	./$(TEST_BINARY)

sanitize: $(SANITIZED_BINARY)
	./$(SANITIZED_BINARY)

$(TEST_BINARY): controller_tests.cpp $(CONTROLLER_SOURCES) $(CONTROLLER_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) controller_tests.cpp $(CONTROLLER_SOURCES) -o $@

$(SANITIZED_BINARY): controller_tests.cpp $(CONTROLLER_SOURCES) $(CONTROLLER_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer \
		controller_tests.cpp $(CONTROLLER_SOURCES) -o $@

$(BUILD_DIR):
	mkdir -p $@
