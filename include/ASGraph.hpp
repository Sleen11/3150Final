#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "ASNode.hpp"

class ASGraph {
public:
    std::unordered_map<int, std::unique_ptr<ASNode>> nodes;
    std::vector<std::vector<ASNode*>> ranks;

    ASNode* get_or_create(int);
    void load_caida_relationships(const std::string&);
    bool detect_cycles_and_print_if_any() const;
    void compute_ranks();
    void make_rov_nodes_from_file(const std::string&);
    void seed_announcements_from_csv(const std::string&);
    void propagate_up();
    void propagate_across();
    void propagate_down();
    void write_ribs_csv(const std::string&) const;
};

