# FOME Integration Test Makefile

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -I..
PYTHON = python3

# Test executables
TESTS = test_xcp_protocol test_can_encoding test_xcp_flash

.PHONY: all clean test test-cpp test-python

all: $(TESTS)

test: test-cpp test-python
	@echo ""
	@echo "================================"
	@echo "All tests completed!"
	@echo "================================"

test-cpp: $(TESTS)
	@echo "Running C++ tests..."
	@echo ""
	@for test in $(TESTS); do \
		echo ">>> Running $$test"; \
		./$$test || exit 1; \
		echo ""; \
	done

test-python:
	@echo "Running Python tests..."
	@echo ""
	$(PYTHON) -m pytest test_a2l_generator.py -v || $(PYTHON) test_a2l_generator.py

test_xcp_protocol: test_xcp_protocol.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_can_encoding: test_can_encoding.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

test_xcp_flash: test_xcp_flash.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Note: test_vehicle_profile requires the full vehicle_profile implementation
# which includes platform-specific code. It's designed for embedded testing.

clean:
	rm -f $(TESTS) *.o

# Individual test targets
run-xcp: test_xcp_protocol
	./test_xcp_protocol

run-can: test_can_encoding
	./test_can_encoding

run-a2l:
	$(PYTHON) test_a2l_generator.py

run-flash: test_xcp_flash
	./test_xcp_flash

# Quick test (just CAN encoding)
quick: test_can_encoding
	./test_can_encoding
