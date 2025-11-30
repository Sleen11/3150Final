#include "ROV.hpp"

void ROV::receive(const std::string &prefix,
                  const Announcement &ann,
                  Relationship rel) {
    // Drop any invalid announcement immediately
    if (ann.rov_invalid) {
        return;
    }
    BGP::receive(prefix, ann, rel);
}

