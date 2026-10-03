#include "query_engine/aggregation.hpp"
#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>

int main(){
    query_engine::Table table = query_engine::generateTable(20);

    assert(std::get<double>(query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE})) == 5063);
    assert(std::get<std::uint64_t>(query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::COUNT, query_engine::ColumnName::PRICE})) == 20);
    assert(std::get<double>(query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::AVG, query_engine::ColumnName::PRICE})) == 253.15);
    assert(std::get<double>(query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::MIN, query_engine::ColumnName::PRICE})) == 0);
    assert(std::get<double>(query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::MAX, query_engine::ColumnName::PRICE})) == 479);

    std::vector<query_engine::Row> empty_rows;
    assert(std::get<double>(query_engine::aggregate(empty_rows, query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE})) == 0);
    assert(std::get<std::uint64_t>(query_engine::aggregate(empty_rows, query_engine::Aggregation{query_engine::AggregationType::COUNT, query_engine::ColumnName::PRICE})) == 0);
    assert(std::get<double>(query_engine::aggregate(empty_rows, query_engine::Aggregation{query_engine::AggregationType::MIN, query_engine::ColumnName::PRICE})) == 0);
    assert(std::get<double>(query_engine::aggregate(empty_rows, query_engine::Aggregation{query_engine::AggregationType::MAX, query_engine::ColumnName::PRICE})) == 0);

    bool threw = false;
    try {
        query_engine::aggregate(table.rows, query_engine::Aggregation{query_engine::AggregationType::NONE, query_engine::ColumnName::PRICE});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    assert(threw);

    std::cout << "All aggregation tests passed\n";
}
