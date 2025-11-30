#pragma once
#include <vector>
#include <memory>
#include "Policy.hpp"

class ASNode {
public:
    int asn;
    int rank = -1;

    std::vector<ASNode*> customers;
    std::vector<ASNode*> providers;
    std::vector<ASNode*> peers;

    std::unique_ptr<Policy> policy;

    explicit ASNode(int a) : asn(a) {}

    void add_customer(ASNode* n) { customers.push_back(n); }
    void add_provider(ASNode* n) { providers.push_back(n); }
    void add_peer(ASNode* n) { peers.push_back(n); }
};

