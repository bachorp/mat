# Create all possible extensions of T_0' cycle pairs by an open ear with up to 3 nodes
# and mark all those that only have T_0'-sub-graphs.
# We can ignore all cycles longer than 4 since they are either unusable (if only 4 agents can move)
# or include a basic subgraph other than T_0'.
# If an extension induces only cycles longer than 4, we ignore it completely.


def sharednodes(c1, c2):
    # computes number of shared nodes of two simple cycles
    return len(set(c1) & set(c2))


def pairtype(c1, c2):
    # computes the type of a pair of cycles that share some nodes
    # print("  c1:", c1, "c2:", c2)
    if len(c1) > len(c2):
        c1, c2 = c2, c1
    if sharednodes(c1, c2) < 1 or len(c1) > 4 or len(c2) > 4:
        return None
    else:
        # print ("  type:", len(c1) - sharednodes(c1,c2), sharednodes(c1,c2), len(c2) - sharednodes(c1,c2))
        return (
            len(c1) - sharednodes(c1, c2),
            sharednodes(c1, c2),
            len(c2) - sharednodes(c1, c2),
        )


def all_t0_pairs(allcycs):
    # iterate over all cycle pairs in a graph
    # and try to identify a non-T0-pair
    # print(" Cycles:", allcycs)
    check = False
    for c in allcycs[2:]:
        if len(c) <= 4:
            check = True
    if not check:
        return False
    for c1 in allcycs:
        for c2 in allcycs:
            if c1 != c2:
                ctype = pairtype(c1, c2)
                if ctype and ctype != (2, 2, 2):
                    return False
    return True


t0dict = {
    "a1": ["a2"],
    "a2": ["b2"],
    "b2": ["b1"],
    "b1": ["a1", "c1"],
    "c1": ["c2"],
    "c2": ["b2"],
}
ears = [["e1"], ["e1", "e2"]]
tested = []

for head in t0dict.keys():
    global tested
    for tail in t0dict.keys():
        if tail == head:
            continue
        for ear in ears:
            done = False
            t0 = Graph(t0dict)
            allpaths = t0.all_paths(tail, head)
            t0.add_path([head] + ear + [tail])
            for gold in tested:
                if gold.is_isomorphic(t0):
                    print("Isomorphic:", [head] + ear + [tail])
                    done = True
            if not done:
                tested += [t0]
                # print("Check:", [ head ] + ear + [ tail ])
                if all_t0_pairs(
                    [["a1", "a2", "b2", "b1"], ["b1", "b2", "c2", "c1"]]
                    + [x + ear for x in allpaths]
                ):
                    print("******Only T0s for ear:", [head] + ear + [tail])
