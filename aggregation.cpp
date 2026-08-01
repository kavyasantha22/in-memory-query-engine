#include "table.hpp"
#include <vector>
#include "query.hpp"
#include <stdexcept>
#include <variant>


double calculate_sum(std::vector<Row> rows, ColumnName column){
    double sum = 0;
    for (auto row: rows){
        sum += std::visit(
            [](auto x){
                return static_cast<double>(x);
            },
            getColumnValue(row, column)
        );
    }
    return sum;
}


uint64_t calculate_count(std::vector<Row> rows, ColumnName column){
    (void)column;
    int64_t c = 0;
    for (auto row: rows){
        (void)row;
        c++;
    }
    return c;
}


double calculate_avg(std::vector<Row> rows, ColumnName column){
    double sum = calculate_sum(rows, column);
    int64_t count = calculate_count(rows, column);
    return sum / count;
}


double calculate_max(std::vector<Row> rows, ColumnName column){
    if (rows.size() == 0) return 0;
    double mx = std::visit(
        [](auto x){
            return static_cast<double>(x);
        },
        getColumnValue(rows[0], column)
    );

    for (auto row: rows){
        double curValue = std::visit(
            [](auto x){
                return static_cast<double>(x);
            },
            getColumnValue(row, column)
        );
        if (mx < curValue){
            mx = curValue;
        }
    }
    return mx;
}


double calculate_min(std::vector<Row> rows, ColumnName column){
    if (rows.size() == 0) return 0;
    double mn = std::visit(
        [](auto x){
            return static_cast<double>(x);
        },
        getColumnValue(rows[0], column)
    );
    for (auto row: rows){
        double curValue = std::visit(
            [](auto x){
                return static_cast<double>(x);
            },
            getColumnValue(row, column)
        );
        if (curValue < mn){
            mn = curValue;
        }
    }
    return mn;
}


ResultValue aggregate(std::vector<Row> rows, Aggregation aggr){
    switch (aggr.type){
        case AggregationType::AVG:
            return calculate_avg(rows, aggr.column);
        case AggregationType::COUNT:
            return calculate_count(rows, aggr.column);
        case AggregationType::MAX:
            return calculate_max(rows, aggr.column);
        case AggregationType::MIN:
            return calculate_min(rows, aggr.column);
        case AggregationType::SUM:
            return calculate_sum(rows, aggr.column);
        default:
            throw std::invalid_argument("Unknown aggregation type");
    }
}

std::vector<ResultValue> aggregate_groups(std::vector<Group> groups, Aggregation aggr){
    std::vector<ResultValue> aggregated_values;
    for (Group g: groups){
        ResultValue val = aggregate(g.rows, aggr);
        aggregated_values.push_back(val);
    }
    return aggregated_values;
}


std::string aggregatationTypeToString(AggregationType type){
    switch (type) {
        case AggregationType::NONE:
            return "none";
        case AggregationType::COUNT:
            return "count";
        case AggregationType::SUM:
            return "sum";
        case AggregationType::AVG:
            return "avg";
        case AggregationType::MIN:
            return "min";
        case AggregationType::MAX:
            return "max";
    }

    throw std::invalid_argument("Unknown aggregation type");
}