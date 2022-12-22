# This script generates 3-cycles for every extension of T_0 I - III to consider.

# The graph with two 4-cycles
alpha = (1, 2, 4, 3)
beta = (3, 4, 6, 5)

# possible extension that are not covered
# by the basic graphs
delta = (5, 6, 8, 7)
zeta = (1, 3, 5, 7)
eta = (3, 4, 8, 7)

G0 = SymmetricGroup(8)

alpha = G0(alpha)
alpha_inv = alpha.inverse()
beta = G0(beta)
beta_inv = beta.inverse()
delta = G0(delta)
delta_inv = delta.inverse()
zeta = G0(zeta)
zeta_inv = zeta.inverse()
eta = G0(eta)
eta_inv = eta.inverse()

id = G0([])

gen1 = [alpha, alpha_inv, beta, beta_inv, delta, delta_inv]
gen2 = [alpha, alpha_inv, beta, beta_inv, zeta, zeta_inv]
gen3 = [alpha, alpha_inv, beta, beta_inv, eta, eta_inv]


def threecycle(p):
    if len(p.cycle_tuples()) == 1 and len(p.cycle_tuples()[0]) == 3:
        return True
    return False


def tryout(pl, gen):
    newpl = []
    for x in gen:
        for p in pl:
            if threecycle(p[0] * x):
                return p[0] * x, p[1] + list(x)
            else:
                newpl += [(p[0] * x, p[1] + list(x))]
    return tryout(newpl, gen)


print(tryout([(id, [])], gen1))
print(tryout([(id, [])], gen2))
print(tryout([(id, [])], gen3))
