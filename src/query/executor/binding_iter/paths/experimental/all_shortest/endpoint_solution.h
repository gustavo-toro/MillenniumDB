#pragma once

#include <cstddef>

namespace Paths::AllShortest {

struct MultiSourceSearchState;

struct EndpointSolution {
    int start_index;
    const MultiSourceSearchState* end_state;

    EndpointSolution(int start_index, const MultiSourceSearchState* end_state);

    bool operator==(const EndpointSolution& other) const;

    bool operator<(const EndpointSolution& other) const;

    struct Hasher {
        std::size_t operator()(const EndpointSolution& s) const;
    };
};

} // namespace Paths::AllShortest
