#pragma once

#include "query_engine/query.hpp"

namespace query_engine {

class RowComparator {
public:
    const std::vector<OrderByItem>& order_by;
    const std::vector<int>& order_by_column_idx;

    bool operator()(const ResultRow& left, const ResultRow& right) const {
        for (size_t i = 0; i < order_by.size(); i++){
            const OrderByItem& order = order_by[i];
            int idxSort = order_by_column_idx[i];
    
            if (idxSort == -1) continue;
    
            ResultValue left_val = left.data[idxSort];
            ResultValue right_val = right.data[idxSort];
            if (
                (order.ascending && left_val < right_val) ||
                (!order.ascending && left_val > right_val)
            ) return true;
    
            if (
                (order.ascending && left_val > right_val) ||
                (!order.ascending && left_val < right_val)
            ) return false;
        }
        return false;
    }
};


struct GroupKeyHash {
    size_t operator()(const std::vector<ResultValue>& key) const {
        
    }
};

std::string orderExpressionToString(const OrderExpression& expression);

std::vector<int> convertToColumnIdx(
    const std::vector<std::string>& table_columns,
    const std::vector<std::string>& query_columns
);

std::vector<int> convertToColumnIdx(
    const std::vector<std::string>& table_columns,
    const std::vector<ColumnName>& query_columns
);

} // namespace query_engine
