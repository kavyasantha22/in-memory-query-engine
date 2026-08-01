#include "formatter.hpp"
#include "query.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <variant>

std::string formatValue(ResultValue value){
    return std::visit([](auto x) {
        std::ostringstream out;
        if constexpr (std::is_same_v<decltype(x), double>) {
            out << std::fixed << std::setprecision(2) << x;
        } else {
            out << x;
        }
        return out.str();
    }, value);
}

static void printSeparator(const std::vector<std::size_t>& widths){
    std::cout << "+";
    for (std::size_t width: widths){
        std::cout << std::string(width + 2, '-') << "+";
    }
    std::cout << "\n";
}

void printSqlTable(
    const std::string& title,
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows
){
    std::vector<std::string> normalizedHeaders = headers;
    std::vector<std::size_t> widths;
    for (std::string header: normalizedHeaders){
        widths.push_back(header.size());
    }

    for (const auto& row: rows){
        for (std::size_t i = 0; i < row.size(); i++){
            if (i >= widths.size()){
                normalizedHeaders.push_back("");
                widths.push_back(0);
            }
            if (row[i].size() > widths[i]){
                widths[i] = row[i].size();
            }
        }
    }

    std::cout << title << "\n";
    printSeparator(widths);
    std::cout << "|";
    for (std::size_t i = 0; i < normalizedHeaders.size(); i++){
        std::cout << " " << std::left << std::setw(widths[i]) << normalizedHeaders[i] << " |";
    }
    std::cout << "\n";
    printSeparator(widths);

    for (const auto& row: rows){
        std::cout << "|";
        for (std::size_t i = 0; i < row.size(); i++){
            std::cout << " " << std::left << std::setw(widths[i]) << row[i] << " |";
        }
        std::cout << "\n";
    }

    printSeparator(widths);
    std::cout << rows.size() << " rows\n\n";
}

void printSqlTable(const std::string& title, const Table& table){
    std::vector<std::string> headers;
    for (ColumnName column: table.column_names){
        headers.push_back(columnNameToString(column));
    }

    std::vector<std::vector<std::string>> rows;
    for (Row row: table.rows){
        std::vector<std::string> formattedRow;
        for (ColumnName column: table.column_names){
            formattedRow.push_back(formatValue(getColumnValue(row, column)));
        }
        rows.push_back(formattedRow);
    }

    printSqlTable(title, headers, rows);
}

void printSqlTable(const std::string& title, const ResultTable& table){
    std::vector<std::vector<std::string>> rows;
    for (ResultRow row: table.rows){
        std::vector<std::string> formattedRow;
        for (ResultValue value: row.data){
            formattedRow.push_back(formatValue(value));
        }
        rows.push_back(formattedRow);
    }

    printSqlTable(title, table.column_names, rows);
}


