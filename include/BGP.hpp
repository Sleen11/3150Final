#pragma once
#include <unordered_map>
#include <vector>
#include <tuple>
#include "Policy.hpp"

class BGP : public Policy {
protected:
    ASNode* owner;

    std::unordered_map<std::string, Announcement> local_rib;
    std::unordered_map<std::string,
        std::vector<std::pair<Announcement, Relationship>>> received_queue;

    bool is_better(const Announcement &a, const Announcement &b) const;

public:
    explicit BGP(ASNode* o) : owner(o) {}
    ASNode* get_owner() const override { return owner; }
    const std::unordered_map<std::string, Announcement>& get_local_rib() const {
    return local_rib;
}


    void receive(const std::string&, const Announcement&, Relationship) override;
    void process_received() override;

    void send_up() override;
    void send_across() override;
    void send_down() override;

    void dump_rib_to_csv_rows(std::vector<std::tuple<int,std::string,std::string>>&) const override;
};

