#include <cstdint>
#include <vector>
#include <variant>

using ResultValue = std::variant<std::uint64_t, std::uint32_t, double>;

struct Row {
    std::uint64_t transaction_id;
    std::uint64_t product_id;
    std::uint64_t category_id;
    double price;
    std::uint32_t quantity;
    std::int64_t timestamp;
};

struct Table {
    std::vector<Row> rows;
};


struct ResultRow {
    std::vector<ResultValue> data;
};


struct ResultTable {
    std::vector<ResultRow> rows;
};




