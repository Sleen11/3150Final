#include "ASGraph.hpp"
#include "ASNode.hpp"
#include "BGP.hpp"
#include "ROV.hpp"
#include "Announcement.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <tuple>

// ---------------- SAFE PARSING HELPERS ----------------

static bool is_number(const std::string &s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

static int safe_stoi(const std::string &s) {
    if (!is_number(s)) return 0;
    return std::stoi(s);
}

// ---------------- NODE CREATION ----------------

ASNode* ASGraph::get_or_create(int asn) {
    auto it = nodes.find(asn);
    if (it != nodes.end()) return it->second.get();

    auto n = std::make_unique<ASNode>(asn);
    n->policy = std::make_unique<BGP>(n.get());
    ASNode* ptr = n.get();
    nodes[asn] = std::move(n);
    return ptr;
}

// ---------------- LOAD RELATIONSHIPS ----------------
// CAIDA format: as1|as2|rel
// rel = -1 -> provider/customer
// rel =  0 -> peer/peer

void ASGraph::load_caida_relationships(const std::string &file) {
    std::ifstream fin(file);
    if (!fin) {
        std::cerr << "Could not open relationships file: " << file << "\n";
        std::exit(1);
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string a, b, r;
        if (!std::getline(ss, a, '|')) continue;
        if (!std::getline(ss, b, '|')) continue;
        if (!std::getline(ss, r, '|')) continue;

        int as1 = safe_stoi(a);
        int as2 = safe_stoi(b);
        int rel = safe_stoi(r);

        ASNode* n1 = get_or_create(as1);
        ASNode* n2 = get_or_create(as2);

        if (rel == -1) {
            // as1 is provider, as2 is customer
            n1->add_customer(n2);
            n2->add_provider(n1);
        } else if (rel == 0) {
            // peer/peer
            n1->add_peer(n2);
            n2->add_peer(n1);
        }
    }
}

// ---------------- CYCLE DETECTION ----------------
// Follow provider links upward. If a cycle is found, print a message and return true.

bool ASGraph::detect_cycles_and_print_if_any() const {
    std::unordered_set<int> temp, perm;

    std::function<bool(ASNode*)> dfs = [&](ASNode* n) {
        if (perm.count(n->asn)) return false;
        if (temp.count(n->asn)) return true;  // back-edge -> cycle

        temp.insert(n->asn);
        for (ASNode* p : n->providers) {
            if (dfs(p)) return true;
        }
        temp.erase(n->asn);
        perm.insert(n->asn);
        return false;
    };

    for (const auto &p : nodes) {
        if (dfs(p.second.get())) {
            std::cerr << "Cycle detected in AS graph" << std::endl;
            return true;
        }
    }
    return false;
}

// ---------------- RANK COMPUTATION ----------------
// Spec: rank 0 = ASes with NO customers (edges).
// Providers get higher ranks (rank increases as you go "up" the graph).

void ASGraph::compute_ranks() {
    // Start with ASes that have NO CUSTOMERS — these are rank 0
    std::queue<ASNode*> q;
    std::unordered_map<int, int> remaining_customers;

    for (auto &p : nodes) {
        remaining_customers[p.first] = p.second->customers.size();
    }

    for (auto &p : nodes) {
        if (p.second->customers.empty()) {
            p.second->rank = 0;
            q.push(p.second.get());
        }
    }

    // Now walk UP through providers
    while (!q.empty()) {
        ASNode* cur = q.front();
        q.pop();

        for (ASNode* prov : cur->providers) {
            remaining_customers[prov->asn]--;
            if (remaining_customers[prov->asn] == 0) {
                prov->rank = cur->rank + 1;
                q.push(prov);
            }
        }
    }

    int max_rank = 0;
    for (auto &p : nodes) {
        max_rank = std::max(max_rank, p.second->rank);
    }

    ranks.clear();
    ranks.resize(max_rank + 1);

    for (auto &p : nodes) {
        ranks[p.second->rank].push_back(p.second.get());
    }
}

// ---------------- ROV NODES ----------------

void ASGraph::make_rov_nodes_from_file(const std::string &file) {
    std::ifstream fin(file);
    if (!fin) {
        std::cerr << "Could not open ROV ASN file: " << file << "\n";
        std::exit(1);
    }

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        if (!is_number(line)) continue;

        int asn = std::stoi(line);
        ASNode* node = get_or_create(asn);
        // Replace its policy with an ROV-aware policy
        node->policy = std::make_unique<ROV>(node);
    }
}

// ---------------- SEED ANNOUNCEMENTS ----------------
// anns.csv format (given by prof):
// seed_asn,prefix,rov_invalid

void ASGraph::seed_announcements_from_csv(const std::string &file) {
    std::ifstream fin(file);
    if (!fin) {
        std::cerr << "Could not open announcement file: " << file << "\n";
        std::exit(1);
    }

    std::string line;
    // skip header
    if (!std::getline(fin, line)) {
        return;
    }

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string asn_s, prefix, rov_s;

        if (!std::getline(ss, asn_s, ',')) continue;
        if (!std::getline(ss, prefix, ',')) continue;
        if (!std::getline(ss, rov_s)) rov_s.clear();

        int seed_asn = safe_stoi(asn_s);
        ASNode* node = get_or_create(seed_asn);

        Announcement a;
        a.prefix = prefix;
        a.as_path.clear();
        a.as_path.push_back(seed_asn);       // origin path starts with itself
        a.next_hop_asn = seed_asn;
        a.rel = Relationship::ORIGIN;

        // rov_invalid field: expect "True" or "False"
        for (auto &c : rov_s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        a.rov_invalid = (rov_s == "true");

        // Let Policy/BGP/ROV handle whether it is stored or dropped.
        node->policy->receive(prefix, a, Relationship::ORIGIN);
    }

    // Process all queued origin announcements once
    for (auto &p : nodes) {
        p.second->policy->process_received();
    }
}

// ---------------- PROPAGATION ----------------
// Up: rank 0 (leaves) -> providers -> ... -> highest rank
// Across: one hop between peers (all ASes send, then all process)
// Down: highest rank -> customers -> ... -> rank 0
// ---------------- PROPAGATION ----------------

void ASGraph::propagate_up() {
    // Pass routes from customers up to providers
    for (auto &lvl : ranks) {
        for (ASNode* n : lvl) {
            n->policy->send_up();
        }
    }

    // After sending, every AS processes what it received
    for (auto &p : nodes) {
        p.second->policy->process_received();
    }
}

void ASGraph::propagate_across() {
    // Pass routes across peers at the same rank
    for (auto &lvl : ranks) {
        for (ASNode* n : lvl) {
            n->policy->send_across();
        }
    }

    // Then install the newly received routes
    for (auto &p : nodes) {
        p.second->policy->process_received();
    }
}

void ASGraph::propagate_down() {
    // Pass routes from providers down to customers.
    // We go from highest rank down to lowest.
    for (auto it = ranks.rbegin(); it != ranks.rend(); ++it) {
        for (ASNode* n : *it) {
            n->policy->send_down();
        }
    }

    // Finally, install what was learned
    for (auto &p : nodes) {
        p.second->policy->process_received();
    }
}


// ---------------- WRITE RIBS CSV ----------------

void ASGraph::write_ribs_csv(const std::string &output_file) const {
    std::vector<std::tuple<int, std::string, std::string>> rows;

    // Collect rows from every AS policy
    for (const auto &p : nodes) {
        p.second->policy->dump_rib_to_csv_rows(rows);
    }

    // Sort by asn, then prefix lexicographically.
    std::sort(rows.begin(), rows.end(),
              [](const auto &a, const auto &b) {
                  if (std::get<0>(a) != s
d::get<0>(b))
                      return std::get<0>(a) < std::get<0>(b);
                  return std::get<1>(a) < std::get<1>(b);
              });

    std::ofstream fout(output_file);
    if (!fout) {
        std::cerr << "Could not open output ribs file: " << output_file << "\n";
        std::exit(1);
    }

    fout << "asn,prefix,as_path\n";
    for (const auto &row : rows) {
        fout << std::get<0>(row) << ",";
        fout << std::get<1>(row) << ",";
        fout << "\"" << std::get<2>(row) << "\"\n";
    }
}

