CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I.
QUERY_TEST := query_test_runner
QUERY_AGGREGATION_TEST := query_aggregation_test_runner
AGGREGATION_TEST := aggregation_test_runner
TEST_TARGETS := $(QUERY_TEST) $(QUERY_AGGREGATION_TEST) $(AGGREGATION_TEST)
CORE_SOURCES := query.cpp table.cpp aggregation.cpp
HEADERS := query.hpp table.hpp aggregation.hpp

.PHONY: all run clean

all: $(TEST_TARGETS)

$(QUERY_TEST): correctness_test/query_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/query_test.cpp $(CORE_SOURCES) -o $(QUERY_TEST)

$(QUERY_AGGREGATION_TEST): correctness_test/query_aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/query_aggregation_test.cpp $(CORE_SOURCES) -o $(QUERY_AGGREGATION_TEST)

$(AGGREGATION_TEST): correctness_test/aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/aggregation_test.cpp $(CORE_SOURCES) -o $(AGGREGATION_TEST)

run: $(TEST_TARGETS)
	./$(QUERY_TEST)
	./$(QUERY_AGGREGATION_TEST)
	./$(AGGREGATION_TEST)

clean:
	rm -f $(TEST_TARGETS)
