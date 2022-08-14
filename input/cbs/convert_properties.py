import json
import csv
import os

PROP_PATH = "./data/experiments-eval/properties"
OUT_PATH = "."

CSV_HEADER = ["g", "b", "a", "c", "seed", "timeout", "memout", "result", "makespan", "t_total", "t_solver",
              "h_expanded", "l_expanded"]


def sort_data(data):
    data_dict = {}
    for key, value in data.items():
        g, b, a, c, s = [v[1:] for v in key.split("-")]
        if g not in data_dict:
            data_dict[g] = {}
        if s not in data_dict[g]:
            data_dict[g][s] = []
        line = [g, b, a, c, s, str(value["time_limit"]), str(value["memory_limit"])]
        if value["solved"]:
            line += ["", str(value["makespan"]), str(value["t_total"]*1000), str(value["t_solver"]*1000),
                     str(value["high_level_expansion"]), str(value["low_level_expansion"])]
        else:
            if value["unsolvable"]:
                line += ["Unsolvable"]
            else:
                line += ["Timeout"]
            line += [""] * 5
        data_dict[g][s].append(line)
    return data_dict


def main():
    with open(PROP_PATH, "r") as file:
        props = file.read()
    data = json.loads(props)
    data_dict = sort_data(data)

    for g, s_dict in data_dict.items():
        g_path = os.path.join(OUT_PATH, g)
        if not os.path.exists(g_path):
            os.mkdir(g_path)
        for s, lines in s_dict.items():
            csv_path = os.path.join(g_path, s + ".csv")
            with open(csv_path, "w") as f:
                writer = csv.writer(f)
                writer.writerow(CSV_HEADER)
                writer.writerows(lines)


if __name__ == '__main__':
    main()