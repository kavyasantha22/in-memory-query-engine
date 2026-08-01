CXX := g++
CXXFLAGS := -Wall -Wextra -I.
QUERY_TEST := query_test_runner
QUERY_AGGREGATION_TEST := query_aggregation_test_runner
AGGREGATION_TEST := aggregation_test_runner
ORDER_BY_TEST := order_by_test_runner
LIMIT_TEST := limit_test_runner
TEST_TARGETS := $(QUERY_TEST) $(QUERY_AGGREGATION_TEST) $(AGGREGATION_TEST) $(ORDER_BY_TEST) $(LIMIT_TEST)
CORE_SOURCES := query.cpp table.cpp aggregation.cpp formatter.cpp
HEADERS := query.hpp table.hpp aggregation.hpp formatter.hpp

.PHONY: all run clean

all: $(TEST_TARGETS)

random: random.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) random.cpp $(CORE_SOURCES) -o random

$(QUERY_TEST): correctness_test/query_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/query_test.cpp $(CORE_SOURCES) -o $(QUERY_TEST)

$(QUERY_AGGREGATION_TEST): correctness_test/query_aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/query_aggregation_test.cpp $(CORE_SOURCES) -o $(QUERY_AGGREGATION_TEST)

$(AGGREGATION_TEST): correctness_test/aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/aggregation_test.cpp $(CORE_SOURCES) -o $(AGGREGATION_TEST)

$(ORDER_BY_TEST): correctness_test/order_by_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/order_by_test.cpp $(CORE_SOURCES) -o $(ORDER_BY_TEST)

$(LIMIT_TEST): correctness_test/limit_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) correctness_test/limit_test.cpp $(CORE_SOURCES) -o $(LIMIT_TEST)

run: $(TEST_TARGETS)
	./$(QUERY_TEST)
	./$(QUERY_AGGREGATION_TEST)
	./$(AGGREGATION_TEST)
	./$(ORDER_BY_TEST)
	./$(LIMIT_TEST)

clean:
	rm -f $(TEST_TARGETS) random
