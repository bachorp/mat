# Note that opposed to the figure of Yu and Rus,
# here a denotes the length of α,
# c the number of nodes α and β have in common,
# and b the size of the ear corresponding to β.

def search(n):
    S = SymmetricGroup(n)
    for a in range(3, n):
        for b in range(1, n):
            if a + b != n:
                continue
            for c in range(1, a):
                if not b + (a - c) + 2 > max(a, b + c): # The outer cycle must be longer than α and β
                    continue
                α = S(tuple(i for i in range(1, a + 1)))
                β = S(tuple(i for i in range(1, c + 1)) + tuple(i for i in range(a + 1, a + b + 1)))
                G = S.subgroup([α, β])
                if factorial(n) / G.order() not in [1, 2]: # i.e. G ≥ A_n does not hold
                    assert(G.is_isomorphic(PGL(2, 5))) # T_0'
                if c != 1: # If we can move the outer cycle, G = S_n
                    l = list(i for i in range(c, a + 1))
                    l.reverse()
                    C = S(tuple([1]) + tuple(l) + tuple(i for i in range(a + 1, a + b + 1)))
                    G = S.subgroup([α, β, C])
                    assert(factorial(n) / G.order() == 1)


for n in range(1, 6 + 1):
    print("Searching n={}".format(n))
    search(n)
