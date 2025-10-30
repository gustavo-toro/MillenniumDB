#include "endpoint_solution.h"

#include "search_state.h"

using namespace Paths::AllShortest;

EndpointSolution::EndpointSolution(int start_index, const MultiSourceSearchState* end_state) :
    start_index(start_index),
    end_state(end_state)
{ }

bool EndpointSolution::operator==(const EndpointSolution& other) const
{
    return int(this->start_index == other.start_index) & int(this->end_state == other.end_state);
}

bool EndpointSolution::operator<(const EndpointSolution& other) const
{
    if (start_index != other.start_index) {
        return start_index < other.start_index;
    } else {
        const MultiSourceSearchState& other_end = *other.end_state;
        return end_state->operator<(other_end);
    }
}

std::size_t EndpointSolution::Hasher::operator()(const EndpointSolution& s) const
{
    return s.end_state->node_id.id;
}
