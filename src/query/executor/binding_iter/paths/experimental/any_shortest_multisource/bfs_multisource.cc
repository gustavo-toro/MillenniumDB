#include "bfs_multisource.h"
#include "system/path_manager.h"

using namespace Paths::Any;

template<bool MULTIPLE_FINAL>
void BFSMultiSource<MULTIPLE_FINAL>::_begin(Binding& _parent_binding)
{
    parent_binding = &_parent_binding;

    lhs->begin(_parent_binding);

    lhs_at_end = false;
    fill_next_lhs_batch();
}

template<bool MULTIPLE_FINAL>
void BFSMultiSource<MULTIPLE_FINAL>::_reset()
{
    // Empty open and visited
    std::queue<const MSSearchState*> empty;
    open.swap(empty);

    search_states.clear();

    lhs->reset();
    lhs_at_end = false;
    ready_solutions.clear();

    fill_next_lhs_batch();
}

template<bool MULTIPLE_FINAL>
void BFSMultiSource<MULTIPLE_FINAL>::fill_next_lhs_batch()
{
    start_batch.clear();
    reached_final.clear();
    if (lhs_at_end) {
        return;
    }

    uint64_t total_start_nodes = 0;
    while (total_start_nodes < 64 && lhs->next()) {
        ObjectId start_node = (*parent_binding)[start];
        if (!start_node.is_null()) {
            auto state_inserted = search_states.emplace(automaton.start_state, start_node);
            start_batch.push_back(start_node);
            open.push(state_inserted.first.operator->());
            total_start_nodes++;
        }
    }
    lhs_at_end = total_start_nodes == 0;

    // Starting state is solution
    if (automaton.is_final_state[automaton.start_state]) {
        for (uint64_t node_idx = 0; node_idx < total_start_nodes; node_idx++) {
            MSSearchState state(automaton.start_state, start_batch[node_idx]);
            auto insert_status = search_states.insert(state);
            ready_solutions.emplace_back(insert_status.first.operator->());
        }
    }
    iter = std::make_unique<NullIndexIterator>();
}

template<bool MULTIPLE_FINAL>
bool BFSMultiSource<MULTIPLE_FINAL>::_next()
{
next_begin:
    while (ready_solutions.size() > 0) {
        auto solution = ready_solutions.back();

        // should i handle MULTIPLE FINAL here?

        for (auto&& [start_node, _] : solution->previous) {
            EndpointSolution boundaries { static_cast<int>(start_node), solution->node_id };

            // we skip if this path was returned before
            if (!returned.insert(boundaries).second) {
                continue;
            }

            solution->start_node = start_batch[start_node];
            solution->start_node_idx = start_node;

            auto path_id = path_manager.set_path(solution, path_var);
            parent_binding->add(start, solution->start_node);
            parent_binding->add(end, solution->node_id);
            parent_binding->add(path_var, path_id);

            return true;
        }
        ready_solutions.pop_back();
    }

    while (open.size() > 0) {
        auto current_state = open.front();

        if (expand_neighbors(*current_state)) {
            // Enumerate reached solutions
            goto next_begin;
        } else {
            // Pop and visit next state
            current_state->in_queue = false;
            open.pop();
        }
    }
    fill_next_lhs_batch();
    if (!start_batch.empty()) {
        goto next_begin;
    }

    return false;
}

template<bool MULTIPLE_FINAL>
bool BFSMultiSource<MULTIPLE_FINAL>::expand_neighbors(const MSSearchState& current_state)
{
    // Check if this is the first time that current_state is explored
    if (iter->at_end()) {
        current_transition = 0;
        // Check if automaton state has transitions
        if (automaton.from_to_connections[current_state.automaton_state].size() == 0) {
            return false;
        }
        set_iter(current_state);
    }

    auto it = std::find(start_batch.begin(), start_batch.end(), current_state.node_id);
    uint32_t current_state_node_idx = it - start_batch.begin();

    // Iterate over the remaining transitions of current_state
    // Don't start from the beginning, resume where it left thanks to current_transition and iter (pipeline)
    while (current_transition < automaton.from_to_connections[current_state.automaton_state].size()) {
        auto& transition = automaton.from_to_connections[current_state.automaton_state][current_transition];

        // Iterate over records until a final state is reached
        while (iter->next()) {
            auto reached_node = ObjectId(iter->get_reached_node());
            MSSearchState search_state(transition.to, reached_node);

            auto [it, inserted] = search_states.insert(search_state);
            auto* reached_state = &*it;

            // we visit the state for the first time
            if (inserted) {
                open.push(reached_state);

                if (current_state.previous.empty()) { // TODO: should this be here
                    reached_state->set_previous(
                        current_state_node_idx,
                        &current_state,
                        transition.type_id,
                        transition.inverse
                    );
                }

                // iterate over the starting nodes that reached the previous state
                for (auto&& [start_node, _] : current_state.previous) {
                    reached_state
                        ->set_previous(start_node, &current_state, transition.type_id, transition.inverse);
                }

                if (automaton.is_final_state[reached_state->automaton_state]) {
                    ready_solutions.push_back(reached_state);
                    return true;
                }
            } else {
                bool state_updated = false;

                if (current_state.previous.empty()) { // TODO: should this be here
                    reached_state->set_previous(
                        current_state_node_idx,
                        &current_state,
                        transition.type_id,
                        transition.inverse
                    );
                    state_updated = true;
                }

                for (auto&& [start_node, _] : current_state.previous) {
                    if (reached_state->previous.count(start_node)) {
                        continue;
                    }
                    reached_state
                        ->set_previous(start_node, &current_state, transition.type_id, transition.inverse);
                    state_updated = true;
                }

                // add to open if there are new paths that reach this state
                if (state_updated && !reached_state->in_queue) {
                    open.push(reached_state);
                    reached_state->in_queue = true;
                }

                // add the state as a solution if it was not already added
                if (state_updated && automaton.is_final_state[reached_state->automaton_state]) {
                    ready_solutions.push_back(reached_state);
                    return true;
                }
            }
        }

        // Construct new iter with the next transition (if there exists one)
        current_transition++;

        if (current_transition < automaton.from_to_connections[current_state.automaton_state].size()) {
            set_iter(current_state);
        }
    }
    return false;
}

template<bool MULTIPLE_FINAL>
void BFSMultiSource<MULTIPLE_FINAL>::print(std::ostream& os, int indent, bool stats) const
{
    if (stats) {
        os << std::string(indent, ' ') << "[begin: " << stat_begin << " next: " << stat_next
           << " reset: " << stat_reset << " results: " << results << " idx_searches: " << idx_searches
           << "]\n";
    }
    os << std::string(indent, ' ') << "Paths::Any::BFSMultiSource(start: " << start << ", end: " << end
       << ")";
}

template class Paths::Any::BFSMultiSource<true>;
template class Paths::Any::BFSMultiSource<false>;
