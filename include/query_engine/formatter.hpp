#pragma once

#include "query_engine/table.hpp"
#include <string>
#include <vector>

namespace query_engine {

std::string formatValue(ResultValue value);

void printSqlTable(
    const std::string& title,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
);

void printSqlTable(const std::string& title, const Table& table);
void printSqlTable(const std::string& title, const ResultTable& table);

void printTable(const std::string& title, const Table& table);
void printResultTable(const std::string& title, const ResultTable& table);

} // namespace query_engine
