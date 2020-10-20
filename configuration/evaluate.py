import glob
import pickle
import pandas as pd
import matplotlib.pyplot as plt

def plot(xlabel: str, ylabel: str, filename: str, legend=True):
    xpoints = ypoints = plt.xlim()
    plt.plot(xpoints, ypoints, linestyle='-', zorder=0)
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.axis('square')
    plt.gca().yaxis.grid(True)
    plt.gca().xaxis.grid(True)
    if plt.gca().get_legend_handles_labels()[0]:
        plt.legend(framealpha=1)
    plt.savefig(filename, dpi=300, bbox_inches = 'tight')
    print("Plot saved to file {}.png".format(filename))
    plt.close()

params   = ['g', 'b', 'a', 'c']
instance = params + ['seed']
static   = instance + ['makespan']

needed = static + ['config', 'result', 't_total']

pickle_jar = 'data.p'

encodings = [(4, 0), (3, 1), (5, 2), (6, 3)]

def load(encoding=False) -> dict:

    if encoding:
        ds = []
        for i, j in encodings:
            d = pd.concat(map(pd.read_csv, glob.glob(str(i) + '/*.csv')))
            d['config'] = j
            ds.append(d)
        df = pd.concat(ds)[needed]
    else:
        df = pd.concat(map(pd.read_csv, glob.glob('[0-3]/*.csv')))[needed]

    cs = dict(tuple(df.groupby(['config'])))

    p = pd.DataFrame(columns=static)
    for c, d in cs.items():
        p = p.merge(d[d['result'].isna()][static], on=static, how='outer')
        assert(not any(p[instance].duplicated()))

    result = dict()

    for c, d in cs.items():
        ps = dict(tuple(d.groupby(params)))
        for i, s in ps.items():
            s = s[s['result'].isna() | (s['result'] == 'Timeout')]
            t = s.apply(lambda x: pd.NA if x['result'] == 'Timeout' else max(x['t_total'], 1), axis=1, result_type='reduce')
            result.setdefault(i, dict())
            result[i][c] = (t.mean(), t.std())

    return result

def do(data: dict, cf1, cf2, label=None):

    if label is not None:
        cf1 += '|600|4|1|1'
        cf2 += '|600|4|1|1'

    x = []
    y = []

    fs = []

    s1 = 0
    s2 = 0

    for i, r in data.items():
        v1 = r[cf1][0]
        v2 = r[cf2][0]

        if not pd.isna(v1):
            s1 += 1
        if not pd.isna(v2):
            s2 += 1
        if v1 < 5e2 or v2 < 5e2:
            continue
        if pd.isna(v1) or pd.isna(v2):
            continue

        fs.append(v2 / v1)
        x.append(v1 / 1e3)
        y.append(v2 / 1e3)

    plt.plot(x, y, '.', label=label, markersize=4)

    return sum(fs) / len(fs), len(fs), s1, s2

if __name__ == '__main__':

    result = enc = None

    try:
        with open(pickle_jar, 'rb') as fp:
            print("Loading from jar")
            result, enc = pickle.load(fp)
    except FileNotFoundError:
        print("Loading from CSVs")
        result = load()
        enc = load(True)
        print("Saving to jar")
        with open(pickle_jar, 'wb') as fp:
            pickle.dump([result, enc], fp)

    other = '|600|4|1|1'

    plt.rcParams['axes.xmargin'] = 1e-2
    plt.rcParams['axes.ymargin'] = 1e-2

    out = "{} vs. {}: {:4.2f}% decrease (n = {}), {} vs. {} solved ({:+})"

    i, n, s1, s2 = do(result, '1|1.5', '0|1.5', label='f = 1.5')
    print(out.format("f = 1.5: With", "without preprocessing", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    i, n, s1, s2 = do(result, '1|2', '0|2', label='f = 2')
    print(out.format("f = 2:   With", "without preprocessing", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    plot('preprocessed', 'not preprocessed', 'p')

    i, n, s1, s2 = do(result, '1|1.5', '1|2', label='preprocessed')
    print(out.format("With    preprocessing: f = 1.5", "f = 2", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    i, n, s1, s2 = do(result, '0|1.5', '0|2', label='not preprocessed')
    print(out.format("Without preprocessing: f = 1.5", "f = 2", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    plot('f = 1.5', 'f = 2', 'f')

    i, n, s1, s2 = do(enc, 1, 0)
    print(out.format("Sequential AMO (1)", "binomial AMO (0)", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    plot('sequential AMO', 'binomial AMO', 'amo', False)

    i, n, s1, s2 = do(enc, 2, 1)
    print(out.format("With edge variables (2)", "without edge variables (1)", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    plot('with edge variables', 'without edge variables', 'ev', False)

    i, n, s1, s2 = do(enc, 3, 2)
    print(out.format("With movement variables (3)", "without movement variables (2)", (1 - 1 / i) * 100, n, s1, s2, s1 - s2))
    plot('with movement variables', 'without movement variables', 'mv', False)

    for i, j in encodings:
        d = pd.concat(map(pd.read_csv, glob.glob(str(i) + '/*.csv')))
        out = "Encoding {} needs {:9.1f} clauses, {:9.1f} literals and {:7.1f} variables on average"
        print(out.format(j, d['n_clauses'].mean(), d['n_literals'].mean(), d['n_variables'].mean()))
