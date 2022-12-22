# This script verifies that any extension of T_0' with a cycle
# of length less than 6 induces the permutation group S_n.


class Node:
    def __init__(self, id):
        self.id = id
        self.neighbours = set()

    def connect(self, other):
        self.neighbours.add(other)
        other.neighbours.add(self)

    def paths(self, g, l, v=[]):
        if self.id in v:
            return []
        v = v.copy()
        v.append(self.id)
        if self.id == g:
            return [v]
        elif l == 0:
            return []
        res = []
        for n in self.neighbours:
            if n.id in v:
                continue
            res += n.paths(g, l - 1, v)
        return res


# T_0'
g = list(map(Node, range(n)))
g[0].connect(g[1])
g[1].connect(g[2])
g[2].connect(g[3])
g[3].connect(g[0])
g[1].connect(g[4])
g[4].connect(g[5])
g[5].connect(g[0])

a = 4
b = 2
c = 2
n = a + b

S = SymmetricGroup(n)
A = S(tuple(k for k in range(1, a + 1)))
B = S(tuple(k for k in range(1, c + 1)) + tuple(k for k in range(a + 1, a + b + 1)))
assert S.subgroup([A, B]).is_isomorphic(PGL(2, 5))

# We add any possible 'ear' or 'handle' such that
# the resulting new cycle has length less than 6
for i in range(n):
    for j in range(i, n):
        for l in range(3, 5 + 1):
            for p in g[i].paths(j, l - 2):
                h = l - len(p)
                S = SymmetricGroup(n + h)
                p = list(map(lambda x: x + 1, p))
                C = S(tuple(p + list(k for k in range(n + 1, n + h + 1))))
                assert S.subgroup([A, B, C]).is_isomorphic(S)
