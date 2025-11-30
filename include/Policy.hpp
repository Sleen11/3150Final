#pragma once
#include <string>
#include <vector>
#include <tuple>
#include "Announcement.hpp"

class ASNode;

class Policy {
public:
    virtual ~Policy() = default;

    virtual void receive(const std::string &prefix,
                         const Announcement &ann,
                         Relationship rel) = 0;

    virtual void process_received() = 0;

    virtual void send_up() = 0;
    virtual void send_across() = 0;
    virtual void send_down() = 0;

    virtual void dump_rib_to_csv_rows(
        std::vector<std::tuple<int,std::string,std::string>> &rows
    ) const = 0;

    virtual ASNode* get_owner() const = 0;
};

