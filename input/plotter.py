import csv
import os
import argparse

import matplotlib.pyplot as plt

G_RANGE = range(4, 13)
S_RANGE = range(0, 20)

def setup_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("data1", type=str, help="folder of first data set")
    parser.add_argument("name1", type=str, help="name of first data set")
    parser.add_argument("data2", type=str, help="folder of second data set")
    parser.add_argument("name2", type=str, help="name of second data set")
    return parser.parse_args()


def plot_solved_agents(configs, solved_dict, num_list):
    assert(len(solved_dict) == 2)
    algo1, algo2 = list(solved_dict.keys())[0], list(solved_dict.keys())[1]

    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    total = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[2]))] += int(solved_dict[algo1][c])
        comp_vals[num_list.index(int(c[2]))] += int(solved_dict[algo2][c])
        total[num_list.index(int(c[2]))] += 1

    plt.xlabel("# agents")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(cbs_vals, total)], label=algo1)
    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(comp_vals, total)], label=algo2)

    plt.legend()
    plt.savefig("solved_agents_{}__{}.png".format(algo1.lower(), algo2.lower()))
    plt.cla()


def plot_solved_containers(configs, solved_dict, num_list):
    assert(len(solved_dict) == 2)
    algo1, algo2 = list(solved_dict.keys())[0], list(solved_dict.keys())[1]

    cbs_vals = [0] * len(num_list)
    comp_vals = [0] * len(num_list)
    total = [0] * len(num_list)
    for c in configs:
        cbs_vals[num_list.index(int(c[3]))] += int(solved_dict[algo1][c])
        comp_vals[num_list.index(int(c[3]))] += int(solved_dict[algo2][c])
        total[num_list.index(int(c[3]))] += 1

    plt.xlabel("# containers")
    plt.ylabel("% solved")

    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(cbs_vals, total)], label=algo1)
    plt.plot(num_list, [float(v) / float(n) * 100 for v, n in zip(comp_vals, total)], label=algo2)

    plt.legend()
    plt.savefig("solved_containers_{}__{}.png".format(algo1.lower(), algo2.lower()))
    plt.cla()


def scatter_dict(configs, data_dict, stat_name, data_type=int, fail_val="", log_scale=False, val_range=[]):
    cbs_vals = []
    comp_vals = []
    assert(len(data_dict) == 2)
    algo1, algo2 = list(data_dict.keys())[0], list(data_dict.keys())[1]
    for c in configs:
        if fail_val:
            cbs_vals.append(data_type(fail_val if c not in data_dict[algo1] else data_dict[algo1][c]))
            comp_vals.append(data_type(fail_val if c not in data_dict[algo2] else data_dict[algo2][c]))
        else:
            if c in data_dict[algo1] and c in data_dict[algo2]:
                cbs_vals.append(data_type(data_dict[algo1][c]))
                comp_vals.append(data_type(data_dict[algo2][c]))

    plt.xlabel(algo1)
    plt.ylabel(algo2)

    if log_scale:
        plt.xscale('log')
        plt.yscale('log')

    if val_range:
        assert(len(val_range) == 2)
        plt.xlim(val_range[0], val_range[1])
        plt.ylim(val_range[0], val_range[1])
        plt.plot([val_range[0], val_range[1]], [val_range[0], val_range[1]])

    plt.scatter(cbs_vals, comp_vals, alpha=0.05)

    plt.savefig("scatter_{}_{}__{}.png".format(stat_name, algo1.lower(), algo2.lower()))
    plt.cla()


def main():
    args = setup_args()

    configs = []
    solved_dict = {args.name1: {}, args.name2: {}}
    makespan_dict = {args.name1: {}, args.name2: {}}
    runtime_dict = {args.name1: {}, args.name2: {}}
    agent_nums = set()
    container_nums = set()
    num_dif_makespans = 0
    for g in G_RANGE:
        for s in S_RANGE:
            with open(os.path.join(args.data1, str(g), str(s) + ".csv")) as file:
                reader = csv.DictReader(file)
                for row in reader:
                    agent_nums.add(int(row["a"]))
                    container_nums.add(int(row["c"]))
                    config = (g, s, row["a"], row["c"], row["b"])
                    configs.append(config)
                    solved_dict[args.name1][config] = not row["result"]
                    if not row["result"]:
                        makespan_dict[args.name1][config] = row["makespan"]
                        runtime_dict[args.name1][config] = float(row["t_total"])
            with open(os.path.join(args.data2, str(g), str(s) + ".csv")) as file:
                reader = csv.DictReader(file)
                for row in reader:
                    a = row["a"]
                    c = row["c"]
                    config = (g, s, a, c, row["b"])
                    if config not in configs:
                        continue
                    solved_dict[args.name2][config] = not row["result"]
                    if not row["result"]:
                        makespan_dict[args.name2][config] = row["makespan"]
                        if solved_dict[args.name1][config] and row["makespan"] != makespan_dict[args.name1][config]:
                            # print(config)
                            num_dif_makespans += 1
                        runtime_dict[args.name2][config] = float(row["t_total"])
            assert (all(c in solved_dict[args.name2] for c in configs))

    plot_solved_agents(configs, solved_dict, sorted(list(agent_nums)))
    plot_solved_containers(configs, solved_dict, sorted(list(container_nums)))
    scatter_dict(configs, makespan_dict, "makespan")
    scatter_dict(configs, runtime_dict, "t_total",
                 data_type=float, fail_val='600000', log_scale=True, val_range=[1, 1e6])
    print("total solved:")
    print("{}: {} / {}".format(args.name1, len([v for v in solved_dict[args.name1].values() if v]), len(configs)))
    print("{}: {} / {}".format(args.name2, len([v for v in solved_dict[args.name2].values() if v]), len(configs)))
    print("instances with diverging makespans: {}".format(num_dif_makespans))


if __name__ == '__main__':
    main()
