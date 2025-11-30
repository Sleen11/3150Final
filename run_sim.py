import sys
import os
import bgp_sim

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 run_sim.py <dataset_dir>")
        print("Example: python3 run_sim.py ../bench/prefix")
        sys.exit(1)

    d = sys.argv[1]

    rel1 = os.path.join(d, "CAIDAASGraphCollector_2025.10.15.txt")
    rel2 = os.path.join(d, "CAIDAASGraphCollector_2025.10.16.txt")
    anns = os.path.join(d, "anns.csv")
    rov  = os.path.join(d, "rov_asns.csv")
    out  = os.path.join(d, "my_ribs.csv")

    print("Running simulator on:", d)
    bgp_sim.run(rel1, rel2, anns, rov, out)
    print("Done. Wrote:", out)

if __name__ == "__main__":
    main()

