#include "ms_search_state.h"

using namespace Paths::Any;

void MSSearchState::print(
    std::ostream& os,
    std::function<void(std::ostream& os, ObjectId)> print_node,
    std::function<void(std::ostream& os, ObjectId, bool)> print_edge,
    bool begin_at_left
) const
{
    if (begin_at_left) {
        auto cur_state = this;
        auto start = this->start_node;

        std::vector<ObjectId> nodes;
        std::vector<ObjectId> edges;
        std::vector<bool> inverse_directions;

        while (cur_state->previous.count(start)) {
            Transition transition = cur_state->previous[start];
            nodes.push_back(cur_state->node_id);
            edges.push_back(transition.type_id);
            inverse_directions.push_back(transition.inverse_direction);

            cur_state = transition.state;
        }

        print_node(os, start);
        for (int_fast32_t i = nodes.size() - 1; i >= 0; --i) {
            print_edge(os, edges[i], !inverse_directions[i]);
            print_node(os, nodes[i]);
        }
    } else {
        auto cur_state = this;
        auto start = this->start_node;

        while (cur_state->previous.count(start)) {
            Transition transition = cur_state->previous[start];
            print_node(os, cur_state->node_id);
            print_edge(os, transition.type_id, !transition.inverse_direction);

            cur_state = transition.state;
        }

        print_node(os, start);
    }
}
