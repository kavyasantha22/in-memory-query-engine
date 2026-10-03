CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude
QUERY_TEST := query_test_runner
QUERY_AGGREGATION_TEST := query_aggregation_test_runner
AGGREGATION_TEST := aggregation_test_runner
ORDER_BY_TEST := order_by_test_runner
HIDDEN_ORDER_BY_TEST := hidden_order_by_test_runner
LIMIT_TEST := limit_test_runner
GROUP_BY_TEST := group_by_test_runner
TEST_TARGETS := $(QUERY_TEST) $(QUERY_AGGREGATION_TEST) $(AGGREGATION_TEST) $(GROUP_BY_TEST) $(ORDER_BY_TEST) $(HIDDEN_ORDER_BY_TEST) $(LIMIT_TEST)
CORE_SOURCES := src/query.cpp src/table.cpp src/aggregation.cpp src/formatter.cpp
HEADERS := $(wildcard include/query_engine/*.hpp)

.PHONY: all run clean

all: $(TEST_TARGETS)

random: examples/random.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) examples/random.cpp $(CORE_SOURCES) -o random

$(QUERY_TEST): tests/query_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/query_test.cpp $(CORE_SOURCES) -o $(QUERY_TEST)

$(QUERY_AGGREGATION_TEST): tests/query_aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/query_aggregation_test.cpp $(CORE_SOURCES) -o $(QUERY_AGGREGATION_TEST)

$(AGGREGATION_TEST): tests/aggregation_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/aggregation_test.cpp $(CORE_SOURCES) -o $(AGGREGATION_TEST)

$(GROUP_BY_TEST): tests/group_by_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/group_by_test.cpp $(CORE_SOURCES) -o $(GROUP_BY_TEST)

$(ORDER_BY_TEST): tests/order_by_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/order_by_test.cpp $(CORE_SOURCES) -o $(ORDER_BY_TEST)

$(HIDDEN_ORDER_BY_TEST): tests/hidden_order_by_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/hidden_order_by_test.cpp $(CORE_SOURCES) -o $(HIDDEN_ORDER_BY_TEST)

$(LIMIT_TEST): tests/limit_test.cpp $(CORE_SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) tests/limit_test.cpp $(CORE_SOURCES) -o $(LIMIT_TEST)

run: $(TEST_TARGETS)
	./$(QUERY_TEST)
	./$(QUERY_AGGREGATION_TEST)
	./$(AGGREGATION_TEST)
	./$(GROUP_BY_TEST)
	./$(ORDER_BY_TEST)
	./$(HIDDEN_ORDER_BY_TEST)
	./$(LIMIT_TEST)

clean:
	rm -f $(TEST_TARGETS) random
