import glob

import pandas as pd

if __name__ == "__main__":

    params = ["g", "b", "a", "c"]
    instance = params + ["seed"]
    static = instance + ["makespan"]

    needed = static + ["config", "result", "t_total", "t_extend"]

    N = 20

    df = pd.concat(map(pd.read_csv, glob.glob("data/*.csv")))[needed]

    cs = dict(tuple(df.groupby(("config"))))

    p = None
    for c, d in cs.items():
        d = cs[c] = d[d["result"].isna()].reset_index(drop=True)
        if p is None:
            p = d
            continue
        assert d[static].equals(p[static])

    data = dict()

    for c, d in cs.items():
        ps = dict(tuple(d.groupby(params)))
        for i, s in ps.items():
            s = s.sort_values("seed")
            assert len(s) >= N
            s = s.head(N)
            data.setdefault(i, dict())
            data[i][c] = (
                s["t_total"].mean(),
                s["t_total"].std(),
                s["t_extend"].mean(),
                s["t_extend"].std(),
            )

    print(" g   b   c |    Without preprocessing*   |     With preprocessing*")
    for k, v in data.items():
        p = v["1|2|600|4|0|0"]
        nop = v["0|2|600|4|0|0"]
        print(
            "{:2}  {:2}  {:2} | {:5.2f} [{:4.2f}] - {:5.2f} [{:4.2f}] | {:5.2f} [{:4.2f}] - {:5.2f} [{:4.2f}]".format(
                k[0],
                k[1],
                k[3],
                nop[0] * 1e-3,
                nop[1] * 1e-3,
                nop[2] * 1e-3,
                nop[3] * 1e-3,
                p[0] * 1e-3,
                p[1] * 1e-3,
                p[2] * 1e-3,
                p[3] * 1e-3,
            )
        )
    print("*Total time [σ] - Construction time [σ]")
    print()
    print(" g   b   c |  nop* |    p*")
    for k, v in data.items():
        p = v["1|2|600|4|0|0"]
        nop = v["0|2|600|4|0|0"]
        print(
            "{:2}  {:2}  {:2} | {:5.2f} | {:5.2f} ".format(
                k[0],
                k[1],
                k[3],
                nop[0] * 1e-3 - nop[2] * 1e-3,
                p[0] * 1e-3 - p[2] * 1e-3,
            )
        )
    print("*Solver time")
