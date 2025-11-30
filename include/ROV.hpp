#pragma once
#include "BGP.hpp"

class ROV : public BGP {
public:
    explicit ROV(ASNode* o) : BGP(o) {}
    void receive(const std::string&, const Announcement&, Relationship) override;
};

