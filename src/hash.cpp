#include <vector>
#include "query_util.hpp"

namespace query_engine {

struct GroupKeyHash {
    size_t operator()(const std::vector<ResultValue>& x) const {

    }
}

}

