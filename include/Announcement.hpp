#pragma once
#include <string>
#include <vector>
#include <sstream>

enum class Relationship {
    ORIGIN = 3,
    CUSTOMER = 2,
    PEER = 1,
    PROVIDER = 0
};

struct Announcement {
    std::string prefix;
    std::vector<int> as_path;
    int next_hop_asn;
    Relationship rel;
    bool rov_invalid;

std::string as_path_string() const {
    std::ostringstream oss;
    oss << "(";

    if (as_path.size() == 1) {
        // Professor format requires a trailing comma for single-AS paths
        oss << as_path[0] << ",";
    } else {
        for (size_t i = 0; i < as_path.size(); ++i) {
            oss << as_path[i];
            if (i + 1 < as_path.size()) oss << ", ";
        }
    }

    oss << ")";
    return oss.str();
}

        
};

