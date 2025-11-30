#include <pybind11/pybind11.h>
#include <string>
#include <stdexcept>
#include "ASGraph.hpp"

namespace py = pybind11;

void run_sim(const std::string &rel_path_1,
             const std::string &rel_path_2,
             const std::string &anns_path,
             const std::string &rov_path,
             const std::string &out_path)
{
    ASGraph graph;

    // Load relationships
    graph.load_caida_relationships(rel_path_1);
    graph.load_caida_relationships(rel_path_2);

    // HARD FAIL on cycles
    graph.detect_cycles_and_print_if_any();

    // Compute ranks
    graph.compute_ranks();

    // Load ROV ASNs
    graph.make_rov_nodes_from_file(rov_path);

    // Seed announcements
    graph.seed_announcements_from_csv(anns_path);

    // ✅✅✅ FULL BGP CONVERGENCE WITH PROCESSING ✅✅✅
    int rounds = graph.ranks.size() * 2;
    for (int i = 0; i < rounds; i++) {

        graph.propagate_up();
        for (auto &p : graph.nodes)
            p.second->policy->process_received();

        graph.propagate_across();
        for (auto &p : graph.nodes)
            p.second->policy->process_received();

        graph.propagate_down();
        for (auto &p : graph.nodes)
            p.second->policy->process_received();
    }

    // Write output
    graph.write_ribs_csv(out_path);
}

PYBIND11_MODULE(bgp_sim, m) {
    m.doc() = "BGP + ROV simulator";
    m.def("run", &run_sim,
          py::arg("rel_path_1"),
          py::arg("rel_path_2"),
          py::arg("anns_path"),
          py::arg("rov_path"),
          py::arg("out_path"));
}

