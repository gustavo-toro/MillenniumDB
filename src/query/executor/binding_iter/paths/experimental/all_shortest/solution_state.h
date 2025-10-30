#pragma once

#include "graph_models/object_id.h"
#include <cstdint>
#include <functional>
#include <vector>

#include "endpoint_solution.h"

namespace Paths { namespace AllShortest {

class SolutionState {
public:
    SolutionState(uint64_t first_idx, EndpointSolution endpoint_solution);

    // check if the indices are valid
    bool has_next();

    // increases the next index
    void advance();
    void print(
        std::ostream& os,
        std::function<void(std::ostream& os, ObjectId)> print_node,
        std::function<void(std::ostream& os, ObjectId, bool)> print_edge,
        bool begin_at_left
    ) const;

    const MultiSourceSearchState* state;
    uint64_t start_idx;

    std::vector<uint64_t> idxs;

    // TODO: do i need this? consider using the next function to extract the nodes and edges
    // immediately
    // std::vector<ObjectId> oids;
};

}} // namespace Paths::AllShortest
