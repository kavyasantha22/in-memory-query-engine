#include "aggregation.hpp"
#include "query.hpp"
#include "table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>

int main(){
    Table table = generateTable(20);

    assert(std::get<double>(aggregate(table.rows, Aggregation{AggregationType::SUM, ColumnName::PRICE})) == 5063);
    assert(std::get<std::uint64_t>(aggregate(table.rows, Aggregation{AggregationType::COUNT, ColumnName::PRICE})) == 20);
    assert(std::get<double>(aggregate(table.rows, Aggregation{AggregationType::AVG, ColumnName::PRICE})) == 253.15);
    assert(std::get<double>(aggregate(table.rows, Aggregation{AggregationType::MIN, ColumnName::PRICE})) == 0);
    assert(std::get<double>(aggregate(table.rows, Aggregation{AggregationType::MAX, ColumnName::PRICE})) == 479);

    std::vector<Row> empty_rows;
    assert(std::get<double>(aggregate(empty_rows, Aggregation{AggregationType::SUM, ColumnName::PRICE})) == 0);
    assert(std::get<std::uint64_t>(aggregate(empty_rows, Aggregation{AggregationType::COUNT, ColumnName::PRICE})) == 0);
    assert(std::get<double>(aggregate(empty_rows, Aggregation{AggregationType::MIN, ColumnName::PRICE})) == 0);
    assert(std::get<double>(aggregate(empty_rows, Aggregation{AggregationType::MAX, ColumnName::PRICE})) == 0);

    bool threw = false;
    try {
        aggregate(table.rows, Aggregation{AggregationType::NONE, ColumnName::PRICE});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    std::cout << "All aggregation tests passed\n";
}
