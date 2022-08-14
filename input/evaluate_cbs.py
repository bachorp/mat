import csv
import os

import matplotlib.pyplot as plt

CBS_PATH = "./cbs"
COMPARE_PATH = "./mapd"
COMPARE_NAME = "mapd"
G_RANGE = range(4, 13)
S_RANGE = range(0, 20)


def plot_solved_agents(configs, solved_dict, num_list):
    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    total = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[2]))] += int(solved_dict[0][c])
        comp_vals[num_list.index(int(c[2]))] += int(solved_dict[1][c])
        total[num_list.index(int(c[2]))] += 1

    plt.xlabel("# agents")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(cbs_vals, total)], label="cbs")
    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(comp_vals, total)], label=COMPARE_NAME)

    plt.legend()
    plt.savefig("solved_agents_cbs__{}.png".format(COMPARE_NAME))
    plt.cla()


def plot_solved_containers(configs, solved_dict, num_list):
    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    total = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[3]))] += int(solved_dict[0][c])
        comp_vals[num_list.index(int(c[3]))] += int(solved_dict[1][c])
        total[num_list.index(int(c[3]))] += 1

    plt.xlabel("# containers")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(cbs_vals, total)], label="cbs")
    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(comp_vals, total)], label=COMPARE_NAME)

    plt.legend()
    plt.savefig("solved_containers_cbs__{}.png".format(COMPARE_NAME))
    plt.cla()


def scatter_dict(configs, data_dict, stat_name, data_type=int, fail_val=0, log_scale=False, val_range=[]):
    cbs_vals = []
    comp_vals = []
    for c in configs:
        if fail_val:
            cbs_vals.append(data_type(fail_val if c not in data_dict[0] else data_dict[0][c]))
            comp_vals.append(data_type(fail_val if c not in data_dict[1] else data_dict[1][c]))
        else:
            if c in data_dict[0] and c in data_dict[1]:
                cbs_vals.append(data_type(data_dict[0][c]))
                comp_vals.append(data_type(data_dict[1][c]))

    plt.xlabel("CBS")
    plt.ylabel(COMPARE_NAME)

    if log_scale:
        plt.xscale('log')
        plt.yscale('log')

    if val_range:
        assert(len(val_range) == 2)
        plt.xlim(val_range[0], val_range[1])
        plt.ylim(val_range[0], val_range[1])
        plt.plot([val_range[0], val_range[1]], [val_range[0], val_range[1]])

    plt.scatter(cbs_vals, comp_vals, alpha=0.1)

    plt.savefig("scatter_{}_cbs__{}.png".format(stat_name, COMPARE_NAME))
    plt.cla()


def main():
    configs = []
    solved_dict = ({}, {})
    makespan_dict = ({}, {})
    runtime_dict = ({}, {})
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
                        runtime_dict[0][config] = float(row["t_total"]) * 1000
            with open(os.path.join(COMPARE_PATH, str(g), str(s) + ".csv")) as file:
                reader = csv.DictReader(file)
                for row in reader:
                    a = row["a"]
                    c = row["c"]
                    config = (g, s, a, c, row["b"])
                    if int(c) > int(a):
                        continue
                    agent_nums.add(int(a))
                    container_nums.add(int(c))
                    if config not in configs:
                        configs.append(config)
                        solved_dict[0][config] = False
                    solved_dict[1][config] = not row["result"]
                    if not row["result"]:
                        makespan_dict[1][config] = row["makespan"]
                        runtime_dict[1][config] = float(row["t_total"])
            assert (all(c in solved_dict[1] for c in configs))

    plot_solved_agents(configs, solved_dict, sorted(list(agent_nums)))
    plot_solved_containers(configs, solved_dict, sorted(list(container_nums)))
    scatter_dict(configs, makespan_dict, "makespan")
    scatter_dict(configs, runtime_dict, "t_total", data_type=float, fail_val='600000', log_scale=True, val_range=[1, 1e6])
    print("total solved:")
    print("CBS: {} / {}".format(len([v for v in solved_dict[0].values() if v]), len(configs)))
    print("{}: {} / {}".format(COMPARE_NAME, len([v for v in solved_dict[1].values() if v]), len(configs)))


if __name__ == '__main__':
    main()
