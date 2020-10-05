#pragma once

#include <set>
#include <queue>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>

#include "util.hpp"

struct Problem
{
    typedef vector<vector<int>> AdjacencyContainer;
    typedef vector<pair<int, int>> EdgeContainer;

    int grid;

    range V;
    EdgeContainer E;
    range C;
    range A;
    range C_u_A;
    vector<int> s;
    vector<int> g;

    AdjacencyContainer adj;

    void init_adj(int n_vertices, EdgeContainer &edges, bool make_symmetric)
    {
        adj.resize(n_vertices);
        for (auto e : edges) {
            E.push_back(e);
            adj[e.first].push_back(e.second);
            if (make_symmetric) {
                E.push_back(swap(e));
                adj[e.second].push_back(e.first);
            }
        }
    }

    void make_grid(std::set<int> &&blockades)
    {
        EdgeContainer edges;
        for (int r : range(grid)) {
            for (int c : range(grid)) {
                if (c < grid - 1)
                    edges.push_back({grid * r + c, grid * r + c + 1});
                if (r < grid - 1)
                    edges.push_back({grid * r + c, grid * (r + 1) + c});
            }
        }
        EdgeContainer filtered;
        for (auto e : edges)
            if (!blockades.count(e.first) && !blockades.count(e.second))
                filtered.push_back(e);
        init_adj(grid * grid, filtered, true);
    }

    // Quadratic grid with custom start, goal, blockades
    Problem(int grid, vector<int> start, vector<int> goal, vector<int> blockades = {})
        : grid(grid), V(grid * grid), C(goal.size()), A(goal.size(), start.size())
        , C_u_A(start.size()), s(start), g(goal)
    {
        make_grid(std::set<int>(blockades.begin(), blockades.end()));
    }

    // Quadratic grid with random start and goal, blockades of given number
    template <typename T = string>
    Problem(int grid, int b = 0, int a = 0, int c = 0, T seed = "")
        : grid(grid), V(grid * grid), C(c), A(c, c + a), C_u_A(c + a)
    {
        vector<int> nodes;
        for (auto v : V)
            nodes.push_back(v);
        std::stringstream ss;
        static const auto sep = ",";
        ss << grid << sep << b << sep << a << sep << c << sep << seed;
        std::mt19937 r(hashCode(ss.str()));
        shuffle_(nodes.begin(), nodes.end(), r);
        s.resize(c + a);
        std::copy(nodes.begin(), nodes.begin() + c, s.begin());
        shuffle_(nodes.begin(), nodes.end() - b, r);
        std::copy(nodes.begin(), nodes.begin() + a, s.begin() + c);
        g.resize(c);
        shuffle_(nodes.begin(), nodes.end() - b, r);
        std::copy(nodes.begin(), nodes.begin() + c, g.begin());
        make_grid(std::set<int>(nodes.end() - b, nodes.end()));
    }

    vector<vector<optional<int>>> dist;

    optional<int> bound(bool pickup = true)
    {
        dist.resize(C_u_A.size(), vector<optional<int>>(V.size()));
        vector<optional<int>> to_agent(C.size());
        vector<optional<int>> to_goal(C.size());
        for (auto e : C_u_A) {
            std::deque<pair<int, int>> q;
            dist[e][s[e]] = 0;
            q.push_back({s[e], 0});
            while (!q.empty()) {
                int u, d;
                std::tie(u, d) = q.front();
                q.pop_front();
                if (C.contains(e)) {
                    if (!to_goal[e] && u == g[e])
                        to_goal[e] = d;
                    if (!to_agent[e])
                        for (auto a : A) if (s[a] == u)
                            to_agent[e] = d;
                }
                for (auto v : adj[u]) {
                    if (dist[e][v])
                        continue;
                    dist[e][v] = d + 1;
                    q.emplace_back(v, d + 1);
                }
            }
        }
        if (!pickup)
            std::fill(to_agent.begin(), to_agent.end(), 0);
        optional<int> lower_bound = 0;
        for (auto c : C) {
            if (s[c] == g[c])
                continue;
            if (!to_goal[c] || (pickup && !to_agent[c]))
                return std::nullopt;
            lower_bound = std::max(lower_bound, to_agent[c] + to_goal[c]);
            for (auto v : V) if (v != s[c])
                dist[c][v] += to_agent[c];
        }
        return lower_bound;
    }

    void print_edges() const
    {
        for (auto e : E)
            std::cout << e.first << " - " << e.second << std::endl;
    }

    void print_adj() const
    {
        for (auto v : V) {
            std::cout << v << ": ";
            for (auto w : adj[v])
                std::cout << w << " ";
            std::cout << std::endl;
        }
    }

    static string fix(int v, int max)
    {
        std::stringstream ss;
        ss << std::setw(num_digits(max)) << std::setfill(' ') << v;
        return ss.str();
    }

    string format_vertex(int v, bool fixed = false) const
    {
        if (grid) {
            int x = v % grid;
            int y = (v - x) / grid;
            int max = V.size() / grid;
            if (fixed)
                return "(" + fix(x, max) + ", " + fix(y, max) + ")";
            return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
        }
        if (fixed)
            return fix(v, V.size());
        return std::to_string(v);
    }

    void print_objective() const
    {
        for (int c : C)
            std::cout << format_vertex(s[c]) << " -> " << format_vertex(g[c]) << std::endl;
        for (int a : A)
            std::cout << format_vertex(s[a]) << std::endl;
    }

    struct Occupation
    {
        int container;
        int goal;
        bool agent;

        Occupation() : container(-1), goal(-1), agent(false) {}
    };

    void print_grid(const vector<int> *position = nullptr) const
    {
        if (!grid) {
            std::cerr << "Cannot print non-grid graph" << std::endl;
            return;
        }
        if (!position)
            position = &s;
        vector<Occupation> occ(V.size());
        for (auto c : C) {
            occ[position->at(c)].container = c;
            occ[g[c]].goal = c;
        }
        for (auto a : A)
            occ[position->at(a)].agent = true;
        for (_ : range(2 * grid + 1))
            std::cout << "-";
        std::cout << std::endl;
        for (auto v : V) {
            std::cout << "|";

            Occupation &o = occ[v];
            std::cout << BG[o.goal % NUM_COLORS + 1];
            std::cout << FG[o.container % NUM_COLORS + 1];

            if (o.container + 1)
                if (o.agent)
                    if (o.container == o.goal)
                        std::cout << FG[0] << "A";
                    else
                        std::cout << "ā";
                else
                    std::cout << "☐";
            else if (o.agent)
                std::cout << "A";
            else if (adj[v].empty())
                std::cout << "·";
            else
                std::cout << " ";
            std::cout << BG[0] << FG[0];
            if ((v + 1) % grid == 0)
                std::cout << "|" << std::endl;
        }
        for (_ : range(2 * grid + 1))
            std::cout << "-";
        std::cout << std::endl;
    }

    void print_distances(int e) const
    {
        if (!grid) {
            std::cerr << "Cannot print non-grid graph" << std::endl;
            return;
        }
        for (_ : range(2 * grid + 1))
            std::cout << "-";
        std::cout << std::endl;
        for (auto v : V) {
            std::cout << "|";
            if (v == s[e]) {
                if (C.contains(e)) {
                    std::cout << FG[e % NUM_COLORS + 1];
                    std::cout << "☐";
                }
                else
                    std::cout << "A";
            }
            else if (!dist[e][v])
                std::cout << "·";
            else
                std::cout << dist[e][v].value();
            std::cout << BG[0] << FG[0];
            if ((v + 1) % grid == 0)
                std::cout << "|" << std::endl;
        }
        for (_ : range(2 * grid + 1))
            std::cout << "-";
        std::cout << std::endl;
    }
};

typedef vector<vector<int>> Paths;

struct Stats
{
    long long t_bound = 0, t_extend = 0, t_solver = 0, t_total = 0;
    int n_clauses = 0, n_variables = 0, n_literals = 0;
    int initial_bound = 0, lower_bound = 0, upper_bound = -1;

    static inline const std::function<int(long long)> f = [](long long t) {
        return t / static_cast<long long>(1e6);
    };

    void print()
    {
        std::printf("BFS: %dms, Formula: %dms, SAT: %dms\n", f(t_bound), f(t_extend), f(t_solver));
        std::printf("Clauses: %d, Variables: %d, Literals: %d\n", n_clauses, n_variables, n_literals);
        std::printf("Initial bound: %d, Lower bound: %d, Upper bound: %d\n", initial_bound, lower_bound, upper_bound);
    }

    static inline const vector<string> fields =
        {"t_bound", "t_extend", "t_solver", "t_total"
       , "n_clauses",  "n_variables",  "n_literals"
       , "initial_bound", "lower_bound", "upper_bound"};

    vector<pair<string, int>> get_all()
    {
        return {{"t_bound", f(t_bound)}, {"t_extend", f(t_extend)}
             , {"t_solver", f(t_solver)}, {"t_total", f(t_total)}
             , {"n_clauses", n_clauses}, {"n_variables", n_variables}
             , {"n_literals", n_literals}
             , {"initial_bound", initial_bound}, {"lower_bound", lower_bound}, {"upper_bound", upper_bound}};
    }
};

struct Solution : public Problem
{
    int makespan;
    Paths paths;

    Stats stats;

    Solution(Problem &p, int makespan, Paths paths, Stats stats)
        : Problem(p), makespan(makespan), paths(paths), stats(stats) {}

    void print()
    {
        for (auto t : range(makespan + 1)) {
            for (auto c : C)
                std::cout << format_vertex(paths[c][t], true) << " ";
            std::cout << "| ";
            for (auto a : A)
                std::cout << format_vertex(paths[a][t], true) << " ";
            std::cout << std::endl;
        }
    }

    void print_at(int t)
    {
        vector<int> position(C_u_A.size());
        for (auto e : C_u_A)
            position[e] = paths[e][t];
        std::cout << "t = " << t << std::endl;
        print_grid(&position);
    }

    void visualize()
    {
        if (!grid) {
            std::cerr << "Cannot visualize non-grid graph" << std::endl;
            return;
        }
        char c;
        int t = 0;
        std::cout << "\0337"; // VT100
        do {
            std::cout << "\0338\033[K";
            print_at(t);
            std::cout << "\033[K";
            std::cin >> c;
            if (c == 'f' && t < makespan)
                ++t;
            if (c == 'd' && t > 0)
                --t;
        } while (c != 'c');
    }
};
