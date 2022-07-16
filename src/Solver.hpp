#pragma once

#include <initializer_list>
#include <tuple>
#include <cassert>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <cmath>

#include <cryptominisat5/cryptominisat.h>

#include "Problem.hpp"

#define VER 2

struct unsolvable_e
{
} unsolvable;

struct partially_solved
{
    Stats stats;

    partially_solved(Stats stats) : stats(stats) {}

    virtual string what() = 0;
};

struct timeout_e : partially_solved
{
    using partially_solved::partially_solved;
    string what() override { return "Timeout"; }
};

constexpr int max_T = 1 << 8;
struct maximum_makespan_e : partially_solved
{
    using partially_solved::partially_solved;
    string what() override { return "Maximum (maximum) makespan exceeded"; }
};

constexpr int max_literals = 1e9;
struct formula_size_e : partially_solved
{
    using partially_solved::partially_solved;
    string what() override { return "Maximum formula size exceeded"; }
};

struct Logger
{
    bool log;

    vector<std::chrono::steady_clock::time_point> begin;

    Logger(bool log = true) : log(log) {}

    std::chrono::steady_clock::time_point start_sequence(string message = "")
    {
        if (log && message.size())
            std::cout << message << std::flush;
        auto start = std::chrono::steady_clock::now();
        begin.push_back(start);
        return start;
    }

    void put(string message) const
    {
        if (log && message.size())
            std::cout << message << std::flush;
    }

    long long end_sequence(string message = "")
    {
        using namespace std::chrono;
        auto elapsed = steady_clock::now() - begin.back();
        begin.pop_back();
        if (log && message.size())
        {
            std::cout << message + "(" << duration_cast<milliseconds>(elapsed).count()
                      << "ms)\n"
                      << std::flush;
        }
        return elapsed.count();
    }
};

struct Config
{
    bool prep = true; // Calculate distances and add singleton clauses
    double f = 2.0;   // Exponential search parameter

    bool amo = true;        // Use sequential instead of binomial encoding
    bool edge_vars = false; // Use designated agent transition variables
    bool move_vars = false; // Use designated move events

#if VER >= 2
    bool fixed_agent = false;     // A container is transported by at most one agent
    bool fixed_container = false; // An agent transports at most one container
#endif

    unsigned n_threads = 4;
    int timeout_s = 600;

    bool edge_reservation = true;
    bool transport = true;

    bool log = true;

    constexpr Config() = default;

    constexpr Config(bool prep, double f) : prep(prep), f(f) {}

    constexpr Config(int encoding)
    {
        switch (encoding)
        {
        case 0:
            amo = false;
            break;
        case 3:
            move_vars = true;
        case 2:
            edge_vars = true;
        }
    }

    string fingerprint()
    {
        static const auto sep = "|";
        std::stringstream ss;
        ss << prep << sep << f;
        ss << sep << timeout_s << sep << n_threads;
        ss << sep << edge_reservation << sep << transport;
        return ss.str();
    }
};

class Solver : public Problem
{
    int T = -1;

    CMSat::SATSolver solver;

    Config config;

    Logger logger;

    Stats stats;

public:
    template <typename... Args>
    Solver(Config config, Args... args) : Problem(args...), config(config) {}

private:
    struct Var
    {
        enum
        {
            VERTEX,
            EDGE,
            AUXILIARY,
#if VER >= 2
            ASSIGNMENT,
#endif
        } type;

#if VER >= 2
        int e, v, w, t, c, a;
#else
        int e, v, w, t, a;
#endif

        static inline int aux = 0;

        Var(int e, int v, int t) : type(VERTEX), e(e), v(v), t(t) {}
        Var(int e, int v, int w, int t) : type(EDGE), e(e), v(v), w(w), t(t) {}
        explicit Var(int a) : type(AUXILIARY), a(a) { aux = std::max(aux, a + 1); }
#if VER >= 2
        Var(int c, int a) : type(ASSIGNMENT), c(c), a(a)
        {
        }
#endif

        string to_string() const
        {
            std::stringstream ss;
            static const auto sep = ",";
            if (type == AUXILIARY)
                ss << a;
#if VER >= 2
            else if (type == ASSIGNMENT)
                ss << c << sep << a;
#endif
            else
            {
                ss << e << sep << v << sep << t;
                if (type == EDGE)
                    ss << sep << w;
            }
            return ss.str();
        }
    };

    std::unordered_map<string, int> vars;
    vector<Var> srav;

    // This is not very fast but flexible
    int to_cms(Var var)
    {
        auto vs = var.to_string();
        auto it = vars.find(vs);
        if (it != vars.end())
            return it->second;
        int ret = srav.size();
        vars.insert({vs, ret});
        srav.push_back(var);
        if (auto nvars = solver.nVars(); nvars < srav.size())
            solver.new_vars(std::max(nvars, 1u));
        return ret;
    }

    struct Lit : public Var
    {
        bool positive = true;

        using Var::Var;

        Lit operator!() const
        {
            Lit ret = *this;
            ret.positive = !positive;
            return ret;
        }

        static inline Solver *s;

        operator CMSat::Lit() const { return CMSat::Lit(s->to_cms(*this), !positive); }
    };

    void add_clause(vector<CMSat::Lit> &&clause)
    {
        ++stats.n_clauses;
        stats.n_literals += clause.size();
        stats.n_variables = vars.size();

        if (stats.n_literals > max_literals)
            throw maximum_makespan_e(stats);

        logger.start_sequence();
        solver.add_clause(clause);
        auto diff = logger.end_sequence();
        stats.t_extend -= diff;
        stats.t_solver += diff;

        assert(solver.okay());
    }

    template <class T = std::initializer_list<Lit>>
    void add(T &&literals)
    {
        vector<CMSat::Lit> clause;
        for (Lit l : literals)
            clause.push_back(l);
        add_clause(std::move(clause));
    }

    const vector<CMSat::lbool> *model;

    vector<CMSat::Lit> destination(int t)
    {
        vector<CMSat::Lit> ret;
        for (auto c : C)
            ret.push_back(Lit(c, g[c], t));
        return ret;
    }

    bool solve(int t)
    {
        if (t > max_T)
            throw maximum_makespan_e(stats);

        logger.start_sequence("Solving t = " + std::to_string(t));

        logger.start_sequence();
        extend(t);
        auto d = destination(t);
        stats.t_extend += logger.end_sequence();
        logger.put(" ...");

        auto timeout = max_time - logger.start_sequence();
        if (timeout.count() < 0)
            throw timeout_e(stats);
        solver.set_timeout_all_calls(static_cast<long double>(timeout.count()) / 1e9L);

        auto res = solver.solve(&d);
        stats.t_solver += logger.end_sequence();

        if (res == CMSat::l_Undef)
            throw timeout_e(stats);

        logger.end_sequence("\b\b\b");

        if (res == CMSat::l_True)
        {
            model = &solver.get_model();
            stats.upper_bound = t;
            return true;
        }
        assert(res == CMSat::l_False);
        stats.lower_bound = t + 1;
        return false;
    }

    std::chrono::steady_clock::time_point max_time;

public:
    Solution solve()
    {
#if VER >= 2
        if ((config.fixed_agent || config.fixed_container) && (config.edge_vars || config.move_vars))
            throw "Unsupported configuration";
#endif
        logger.log = config.log;

        max_time = logger.start_sequence() + std::chrono::seconds(config.timeout_s);
        logger.start_sequence("Initializing ");

        Lit::s = this;
        solver.set_num_threads(config.n_threads);

        logger.start_sequence();
        auto b = config.prep ? bound(config.transport) : 0;
        stats.t_bound = logger.end_sequence();

        if (!b)
            throw unsolvable;
        int l = b.value();
        stats.lower_bound = stats.initial_bound = l;

        logger.put(" -");

        logger.start_sequence();
        origin();
#if VER >= 2
        if (config.fixed_agent)
            fixed_agent();
        if (config.fixed_container)
            fixed_container();
#endif
        extend(l);
        stats.t_extend += logger.end_sequence();

        logger.end_sequence(" ");

        int t = l;
        while (!solve(t))
        {
            l = t; // Strictly lower
            t = std::max(static_cast<int>(ceil(t * config.f)), 1);
        }
        int r = t;
        while (l + 1 < r)
        {
            int m = (l + r) / 2;
            if (solve(m))
                r = m;
            else
                l = m;
        }
        stats.t_total = logger.end_sequence(
            "Found optimal solution of length " + std::to_string(r) + " ");

        vector<vector<int>> paths(C_u_A.size(), vector<int>(r + 1));
        for (auto l : range(*model))
        {
            if (model->at(l) != CMSat::l_True)
                continue;
            if (l >= static_cast<int>(srav.size()))
                continue;
            auto v = srav[l];
            if (v.type != Var::VERTEX)
                continue;
            if (v.t > r)
                continue;
            paths.at(v.e).at(v.t) = v.v;
        }
        return Solution(*this, r, paths, stats);
    }

private:
    // O(|vars|^2)
    void inline amo_binomial(const vector<Lit> &vars)
    {
        for (int i : range(vars))
            for (int j : range(vars))
                if (i != j)
                    add({!vars[i], !vars[j]});
    }

    // O(|vars|) with |vars| auxilary variables
    void inline amo_sequential(const vector<Lit> &vars)
    {
        int r = Var::aux;
        for (int i : range(vars))
        {
            add({!vars[i], Lit(r + i)});
            if (i + 1 < static_cast<int>(vars.size()))
            {
                add({!Lit(r + i), Lit(r + i + 1)});
                add({!vars[i + 1], !Lit(r + i)});
            }
        }
    }

    void amo(const vector<Lit> &vars)
    {
        if (config.amo)
            amo_sequential(vars);
        else
            amo_binomial(vars);
    }

    void edge_vars(int t)
    {
        for (auto a : A)
            for (auto e : E)
            {
                add({!Lit(a, e.first, e.second, t), Lit(a, e.first, t)});
                add({!Lit(a, e.first, e.second, t), Lit(a, e.second, t + 1)});
                if (config.edge_reservation)
                    add({!Lit(a, e.first, t), !Lit(a, e.second, t + 1), Lit(a, e.first, e.second, t)});
            }
    }

    void origin()
    {
        for (auto e : C_u_A)
            add({Lit(e, s[e], 0)});
    }

#if VER >= 2
    void fixed_agent()
    {
        for (auto c : C)
        {
            vector<Lit> vars;
            for (auto a : A)
                vars.emplace_back(c, a);
            amo(vars);
        }
    }

    void fixed_container()
    {
        for (auto a : A)
        {
            vector<Lit> vars;
            for (auto c : C)
                vars.emplace_back(c, a);
            amo(vars);
        }
    }
#endif

    // (|A| + |C|) amo(|V|)
    void uniqueness(int t)
    {
        for (auto e : C_u_A)
        {
            vector<Lit> vars;
            for (auto v : V)
                vars.emplace_back(e, v, t);
            amo(vars);
        }
    }

    // O(|A||V|) of size O(∆)
    void whereabouts(int t)
    {
        for (auto e : C_u_A)
            for (auto v : V)
            {
                vector<Lit> clause({!Lit(e, v, t), Lit(e, v, t + 1)});
                for (auto w : adj[v])
                    clause.emplace_back(e, w, t + 1);
                add(clause);
            }
    }

    // |V| (amo(|A|) + amo(|C|))
    void vertex_reservation(int t)
    {
        for (auto v : V)
        {
            vector<Lit> vars;
            for (auto a : A)
                vars.emplace_back(a, v, t);
            amo(vars), vars.clear();
            for (auto c : C)
                vars.emplace_back(c, v, t);
            amo(vars);
        }
    }

    // |E||A|^2
    void inline edge_reservation_(int t)
    {
        const auto &R = config.transport ? A : C_u_A;
        std::set<pair<int, int>> seen;
        for (auto e : E)
        {
            if (seen.count(swap(e)))
                continue;
            seen.insert(e);
            for (auto a : R)
                for (auto b : R)
                    if (a != b)
                        add({!Lit(a, e.first, t), !Lit(a, e.second, t + 1), !Lit(b, e.second, t), !Lit(b, e.first, t + 1)});
        }
    }

    // |E| amo(|A|)
    void inline edge_reservation_ev(int t)
    {
        const auto &R = config.transport ? A : C_u_A;
        std::set<pair<int, int>> seen;
        for (auto e : E)
        {
            if (seen.count(swap(e)))
                continue;
            seen.insert(e);
            vector<Lit> vars;
            for (auto a : R)
            {
                vars.emplace_back(a, e.first, e.second, t);
                vars.emplace_back(a, e.second, e.first, t);
            }
            amo(vars);
        }
    }

    void edge_reservation(int t)
    {
        if (config.edge_vars)
            edge_reservation_ev(t);
        else
            edge_reservation_(t);
    }

    // |C||E||A| and |C||E| of size amo(|A|)
    void inline transport_(int t)
    {
        for (auto c : C)
            for (auto e : E)
            {
                vector<Lit> base({!Lit(c, e.first, t), !Lit(c, e.second, t + 1)});
                vector<Lit> transported = base;
                for (auto a : A)
                {
                    transported.push_back(Lit(a, e.first, t));
                    vector<Lit> transporting = base;
                    transporting.push_back(!Lit(a, e.first, t));
#if VER >= 2
                    vector<Lit> assigned = transporting;
                    assigned.push_back(!Lit(c, a));
                    add(assigned);
#endif
                    transporting.push_back(Lit(a, e.second, t + 1));
                    add(transporting);
                }
                add(transported);
            }
    }

    // |C||E| of size amo(|A|)
    void inline transport_ev(int t)
    {
        for (auto c : C)
            for (auto e : E)
            {
                vector<Lit> clause({!Lit(c, e.first, t), !Lit(c, e.second, t + 1)});
                for (auto a : A)
                    clause.emplace_back(a, e.first, e.second, t);
                add(clause);
            }
    }

    // |E|(|C| + |A|) with 2|E| auxiliary variables
    void inline transport_mv(int t)
    {
        int aux = Var::aux;
        for (auto e : E)
        {
            int moving = aux++, moved = aux++;
            for (auto c : C)
                add({!Lit(c, e.first, t), !Lit(c, e.second, t + 1), Lit(moving)});
            vector<Lit> clause({!Lit(moved)});
            for (auto a : A)
                clause.emplace_back(a, e.first, e.second, t);
            add(clause);
            add({!Lit(moving), Lit(moved)});
        }
    }

    void transport(int t)
    {
        if (config.edge_vars)
            if (config.move_vars)
                transport_mv(t);
            else
                transport_ev(t);
        else
            transport_(t);
    }

    void extend(int t)
    {
        while (t > T)
        {
            ++T;
            if (config.prep)
                preprocessed(T);
            uniqueness(T);
            vertex_reservation(T);
            if (T)
            {
                if (config.edge_vars)
                    edge_vars(T - 1);
                whereabouts(T - 1);
                if (config.edge_reservation)
                    edge_reservation(T - 1);
                if (config.transport)
                    transport(T - 1);
            }
        }
    }

    void preprocessed(int t)
    {
        for (auto e : C_u_A)
            for (auto v : V)
                if (dist[e][v] > t)
                    add_clause({!Lit(e, v, t)});
    }
};
