#include "solution_state.h"

#include <stack>

#include "search_state.h"

using namespace Paths::AllShortest;

SolutionState::SolutionState(uint64_t first_idx, EndpointSolution endpoint_solution) :
    state(endpoint_solution.end_state),
    start_idx(endpoint_solution.start_index)
{
    auto distance = endpoint_solution.end_state->previous[start_idx].distance;
    if (distance > 0) {
        idxs.reserve(distance);
        idxs[0] = first_idx;
    }
}

bool SolutionState::has_next()
{
    std::stack<const MultiSourceSearchState*> state_stack;
    state_stack.push(state);

    uint64_t i = 0;

    bool reach_end = false;

    while (state_stack.empty() && i < idxs.size()) {
        const MultiSourceSearchState* current_state = state_stack.top();

        if (current_state->previous[start_idx].previous.empty()) {
            reach_end = true;
            break;
        }

        if (idxs[i] < current_state->previous[start_idx].previous.size()) {
            const MultiSourceSearchState*
                next_state = current_state->previous[start_idx].previous[idxs[i]].previous;
            state_stack.push(next_state);
            i++;
        } else {
            state_stack.pop();
            i--;
        }
    }

    return reach_end;
}

void SolutionState::advance()
{
    std::stack<const MultiSourceSearchState*> state_stack;

    uint64_t i = 0;

    // find the second to last state
    for (uint64_t i = 0; i < idxs.size() - 1; ++i) {
        const MultiSourceSearchState* current_state = state_stack.top();

        const MultiSourceSearchState*
            next_state = current_state->previous[start_idx].previous[idxs[i]].previous;

        state_stack.push(next_state);
    }

    i = idxs.size() - 1;

    while (!state_stack.empty()) {
        const auto current_state = state_stack.top();

        // if the index is in range, we increase it
        if (idxs[i] + 1 < current_state->previous[start_idx].previous.size()) {
            idxs[i]++;
            break;
        }

        // otherwise, we set idxs[i] to 0 and remove the state from the stack
        idxs[i] = 0;
        i--;
        state_stack.pop();
    }
}

void SolutionState::print(
    std::ostream& os,
    std::function<void(std::ostream& os, ObjectId)> print_node,
    std::function<void(std::ostream& os, ObjectId, bool)> print_edge,
    bool begin_at_left
) const
{
    if (begin_at_left) {
        const MultiSourceSearchState* current_state = state;

        uint64_t i = 0;
        while (i < idxs.size()) {
            Transition transition = current_state->previous[start_idx].previous[idxs[i]];
            print_node(os, current_state->node_id);
            print_edge(os, transition.type_id, transition.inverse_direction);
            current_state = current_state->previous[start_idx].previous[idxs[i]].previous;
            i++;
        }

        print_node(os, current_state->node_id);
    } else {
        const MultiSourceSearchState* current_state = state;

        std::vector<ObjectId> nodes;
        std::vector<ObjectId> edges;
        std::vector<bool> inverse_directions;

        for (uint64_t i = 0; i < idxs.size(); ++i) {
            Transition transition = current_state->previous[start_idx].previous[idxs[i]];
            nodes.push_back(current_state->node_id);
            edges.push_back(transition.type_id);
            inverse_directions.push_back(transition.inverse_direction);
            current_state = transition.previous;
        }

        print_node(os, current_state->node_id);

        for (int64_t i = idxs.size() - 1; i >= 0; --i) {
            print_edge(os, edges[i], inverse_directions[i]);
            print_node(os, nodes[i]);
        }
    }
}
