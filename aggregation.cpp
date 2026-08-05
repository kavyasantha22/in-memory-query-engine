#include "table.hpp"
#include <vector>
#include "query.hpp"
#include <stdexcept>
#include <variant>


double calculateSum(std::vector<Row> rows, ColumnName column){
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


uint64_t calculateCount(std::vector<Row> rows, ColumnName column){
    (void)column;
    int64_t c = 0;
    for (auto row: rows){
        (void)row;
        c++;
    }
    return c;
}


double calculateAvg(std::vector<Row> rows, ColumnName column){
    double sum = calculateSum(rows, column);
    int64_t count = calculateCount(rows, column);
    return sum / count;
}


double calculateMax(std::vector<Row> rows, ColumnName column){
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


double calculateMin(std::vector<Row> rows, ColumnName column){
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
            return calculateAvg(rows, aggr.column);
        case AggregationType::COUNT:
            return calculateCount(rows, aggr.column);
        case AggregationType::MAX:
            return calculateMax(rows, aggr.column);
        case AggregationType::MIN:
            return calculateMin(rows, aggr.column);
        case AggregationType::SUM:
            return calculateSum(rows, aggr.column);
        default:
            throw std::invalid_argument("Unknown aggregation type");
    }
}

std::vector<ResultValue> aggregateGroups(std::vector<Group> groups, Aggregation aggr){
    std::vector<ResultValue> aggregated_values;
    for (Group g: groups){
        ResultValue val = aggregate(g.rows, aggr);
        aggregated_values.push_back(val);
    }
    return aggregated_values;
}


std::string aggregationTypeToString(AggregationType type){
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

std::string aggregationToString(Aggregation aggregation){
    return (
        aggregationTypeToString(aggregation.type) + 
        "(" + columnNameToString(aggregation.column) + ")"
    );
}
