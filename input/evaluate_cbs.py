import csv
import os

import matplotlib.pyplot as plt

CBS_PATH = "./cbs"
COMPARE_PATH = "./mapd"
COMPARE_NAME = "mapd"
G_RANGE = range(4, 12)
S_RANGE = range(0, 19)


def plot_solved_agents(configs, solved_dict, num_list):
    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[2]))] += int(solved_dict[0][c])
        comp_vals[num_list.index(int(c[2]))] += int(solved_dict[1][c])

    plt.xlabel("# agents")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(len(solved_dict[0])) * 100 for v in cbs_vals], label="cbs")
    plt.plot(num_list, [float(v) / float(len(solved_dict[1])) * 100 for v in comp_vals], label=COMPARE_NAME)

    plt.legend()
    plt.savefig("solved_agents_cbs__{}.png".format(COMPARE_NAME))
    plt.cla()


def plot_solved_containers(configs, solved_dict, num_list):
    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[3]))] += int(solved_dict[0][c])
        comp_vals[num_list.index(int(c[3]))] += int(solved_dict[1][c])

    plt.xlabel("# containers")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(len(solved_dict[0])) * 100 for v in cbs_vals], label="cbs")
    plt.plot(num_list, [float(v) / float(len(solved_dict[1])) * 100 for v in comp_vals], label=COMPARE_NAME)

    plt.legend()
    plt.savefig("solved_containers_cbs__{}.png".format(COMPARE_NAME))
    plt.cla()


def scatter_makespan(configs, makespan_dict):
    cbs_vals = []
    comp_vals = []
    for c in configs:
        if c in makespan_dict[0] and c in makespan_dict[1]:
            cbs_vals.append(int(makespan_dict[0][c]))
            comp_vals.append(int(makespan_dict[1][c]))

    plt.xlabel("CBS")
    plt.ylabel(COMPARE_NAME)

    plt.scatter(cbs_vals, comp_vals)

    plt.savefig("scatter_makespan_cbs__{}.png".format(COMPARE_NAME))
    plt.cla()


def main():
    configs = []
    solved_dict = ({}, {})
    makespan_dict = ({}, {})
    agent_nums = set()
    container_nums = set()
    for g in G_RANGE:
        for s in S_RANGE:
            with open(os.path.join(CBS_PATH, str(g), str(s) + ".csv")) as file:
                reader = csv.DictReader(file)
                for row in reader:
                    agent_nums.add(int(row["a"]))
                    container_nums.add(int(row["c"]))
                    config = (g, s, row["a"], row["c"], row["b"])
                    configs.append(config)
                    solved_dict[0][config] = not row["result"]
                    if not row["result"]:
                        makespan_dict[0][config] = row["makespan"]
            with open(os.path.join(COMPARE_PATH, str(g), str(s) + ".csv")) as file:
                reader = csv.DictReader(file)
                for row in reader:
                    config = (g, s, row["a"], row["c"], row["b"])
                    if config not in configs:
                        continue
                    solved_dict[1][config] = not row["result"]
                    if not row["result"]:
                        makespan_dict[1][config] = row["makespan"]
            assert (all(c in solved_dict[1] for c in configs))

    plot_solved_agents(configs, solved_dict, sorted(list(agent_nums)))
    plot_solved_containers(configs, solved_dict, sorted(list(container_nums)))
    scatter_makespan(configs, makespan_dict)


if __name__ == '__main__':
    main()
