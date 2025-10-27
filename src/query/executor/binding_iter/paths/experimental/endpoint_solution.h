#pragma once

#include "graph_models/object_id.h"

struct EndpointSolution {
    int start_index;
    ObjectId end;

    EndpointSolution(int start_index, ObjectId end) :
        start_index(start_index),
        end(end)
    { }

    bool operator==(const EndpointSolution& other) const
    {
        return int(this->start_index == other.start_index) & int(this->end == other.end);
    }

    struct Hasher {
        std::size_t operator()(const EndpointSolution& s) const
        {
            return s.end.id;
        }
    };
};

// Dummy structure for template usage
class DummyEndpointSet {
public:
    static inline void clear() { }

    static inline std::pair<bool, bool> insert(const EndpointSolution&)
    {
        return { true, true };
    }
};
