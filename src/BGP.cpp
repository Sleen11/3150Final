#include "BGP.hpp"
#include "ASNode.hpp"
#include <algorithm>

// Prefer better relationship, then shorter path, then lower next hop ASN
bool BGP::is_better(const Announcement &a, const Announcement &b) const {
    // 1) Relationship: CUSTOMER > PEER > PROVIDER (ORIGIN is treated as best)
    if (static_cast<int>(a.rel) != static_cast<int>(b.rel)) {
        return static_cast<int>(a.rel) > static_cast<int>(b.rel);
    }
    // 2) Shorter AS path
    if (a.as_path.size() != b.as_path.size()) {
        return a.as_path.size() < b.as_path.size();
    }
    // 3) Lower next-hop ASN
    return a.next_hop_asn < b.next_hop_asn;
}

void BGP::receive(const std::string &prefix,
                  const Announcement &ann,
                  Relationship rel) {
    // Just enqueue; owner ASN and rel get applied in process_received()
    received_queue[prefix].push_back({ann, rel});
}

void BGP::process_received() {
    for (auto &entry : received_queue) {
        const std::string &prefix = entry.first;
        auto &vec = entry.second;

        for (auto &p : vec) {
            Announcement cand = p.first;
            Relationship rel = p.second;

            // Prepend this ASN to AS path for non-origin routes
            if (rel != Relationship::ORIGIN) {
                cand.as_path.insert(cand.as_path.begin(), owner->asn);
            }

            // This is the relationship under which *we* learned this route
            cand.rel = rel;
            cand.next_hop_asn = owner->asn;

            auto it = local_rib.find(prefix);
            if (it == local_rib.end()) {
                local_rib[prefix] = cand;
            } else if (is_better(cand, it->second)) {
                it->second = cand;
            }
        }
    }
    received_queue.clear();
}

// Helper: should we export this route in a given direction?
// dir: the relationship of the neighbor we are sending TO
static bool export_allowed(Relationship learned_from, Relationship dir_to_neigh) {
    // Standard Gao–Rexford-ish export rules:
    //
    // - Customer routes (CUSTOMER / ORIGIN) can go to everyone.
    // - Peer or Provider routes can only go to customers.
    //
    // So:
    //   - if neighbor is a customer: we can export anything.
    //   - if neighbor is a peer or provider: only export routes we
    //     learned from customers / origin.
    if (dir_to_neigh == Relationship::CUSTOMER) {
        return true;  // export all to customers
    }

    // Neighbor is PEER or PROVIDER:
    // only export routes learned from customers (or we are origin)
    return (learned_from == Relationship::CUSTOMER ||
            learned_from == Relationship::ORIGIN);
}

void BGP::send_up() {
    // Sending from us -> providers  (neighbor relation: PROVIDER)
    for (const auto &entry : local_rib) {
        const std::string &prefix = entry.first;
        const Announcement &best = entry.second;

        if (!export_allowed(best.rel, Relationship::PROVIDER)) {
            continue;
        }

        Announcement ann = best;
        ann.next_hop_asn = owner->asn;

        for (ASNode* prov : owner->providers) {
            prov->policy->receive(prefix, ann, Relationship::CUSTOMER);
        }
    }
}

void BGP::send_across() {
    // Sending from us -> peers (neighbor relation: PEER)
    for (const auto &entry : local_rib) {
        const std::string &prefix = entry.first;
        const Announcement &best = entry.second;

        if (!export_allowed(best.rel, Relationship::PEER)) {
            continue;
        }

        Announcement ann = best;
        ann.next_hop_asn = owner->asn;

        for (ASNode* peer : owner->peers) {
            peer->policy->receive(prefix, ann, Relationship::PEER);
        }
    }
}

void BGP::send_down() {
    // Sending from us -> customers (neighbor relation: CUSTOMER)
    for (const auto &entry : local_rib) {
        const std::string &prefix = entry.first;
        const Announcement &best = entry.second;

        if (!export_allowed(best.rel, Relationship::CUSTOMER)) {
            continue;
        }

        Announcement ann = best;
        ann.next_hop_asn = owner->asn;

        for (ASNode* cust : owner->customers) {
            cust->policy->receive(prefix, ann, Relationship::PROVIDER);
        }
    }
}

void BGP::dump_rib_to_csv_rows(
    std::vector<std::tuple<int,std::string,std::string>> &rows) const
{
    for (const auto &entry : local_rib) {
        const std::string &prefix = entry.first;
        const Announcement &ann = entry.second;
        rows.emplace_back(owner->asn, prefix, ann.as_path_string());
    }
}

