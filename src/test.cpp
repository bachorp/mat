#include <map>
#include <fstream>
#include <array>

#include "Solver.hpp"

template <typename... Args>
optional<Solution> solve(Csv *csv, Args... args)
{
    Solver p(args...);
    if (!csv)
        p.print_grid();
    try {
        auto solution = p.solve();
        if (csv) {
            csv->set("makespan", solution.makespan);
            for (auto s : solution.stats.get_all())
                csv->set(std::move(s.first), s.second);
        }
        solution.stats.print();
        return solution;
    }
    catch(unsolvable_e) {
        if (csv)
            csv->set("result", "Unsolvable");
        std::cout << std::endl << "Unsolvable!" << std::endl;
    }
    catch(partially_solved &e) {
        auto t = e.what();
        if (csv) {
            csv->set("result", t);
            for (auto s : e.stats.get_all())
                csv->set(std::move(s.first), s.second);
        }
        std::cout << std::endl << t << std::endl;
        e.stats.print();
    }
    return std::nullopt;
}

void example(Csv *csv) // Figure 1
{
    vector<int> start = {4, 9, 7, 13};
    vector<int> goal = {12, 1};
    vector<int> blokades = {6, 11};

    auto solution = solve(csv, Config(), 4, start, goal, blokades);
    assert(solution);

    auto v = solution.value();
    for (auto e : v.C_u_A)
        v.print_distances(e);
    v.visualize();
}

void counter(Csv *csv) // Figure 2
{
    vector<int> start = {0, 0, 2};
    vector<int> goal = {2};
    vector<int> blockades = {3, 4, 5, 6, 7, 8};

    auto solution = solve(csv, Config(), 3, start, goal, blockades);
    assert(solution);

    auto v = solution.value();
    v.print();
    v.visualize();
}

constexpr int from_percentage(int g, float b)
{
    return b / 100 * g * g + .5; // rounding
}

void grid_input(Csv *csv)
{
    int g, b, a, c;
    string s;
    std::cout << std::endl;
    std::cout << "√ Grid size:  "; std::cin >> g;
    std::cout << "% Blockades:  "; std::cin >> b;
    std::cout << "# Agents:     "; std::cin >> a;
    std::cout << "# Containers: "; std::cin >> c; std::cin.ignore();
    std::cout << "X Seed:       "; std::getline(std::cin, s);
    std::cout << std::endl;

    auto solution = solve(csv, Config(), g, from_percentage(g, b), a, c, s);

    if (solution)
        solution.value().visualize();
}

template <typename T = std::string>
void grid_test(Csv* csv,
    int g, int b, int a, int c, T seed = "", Config config = Config())
{
    std::cout << "────────────────────────────────────────────────────────────" << std::endl;
    std::printf("g = %d, b = %d, a = %d, c = %d, seed = "
        , g, b, a, c); std::cout << seed;
    std::printf(", config = %s", config.fingerprint().c_str());
    std::cout << std::endl;

    if (csv) {
        csv->clear();
        csv->set("g", g); csv->set("b", b); csv->set("a", a); csv->set("c", c); csv->set("seed", seed);
        csv->set("config", config.fingerprint());
    }

    solve(csv, config, g, from_percentage(g, b), a, c, seed);
    if (csv)
        csv->write();
}

template <typename T = std::string>
void regular_mapf(Csv *csv, T seed = "") // cf. Barták et al.
{
    Config p, nop;
    p.transport = nop.transport = false;
    p.edge_reservation = nop.edge_reservation = false;
    p.prep = true;
    nop.prep = false;

    for (Config config : {p, nop})
    for (int g : {16, 32})
    for (int b : {10, 20})
    for (int c : {5, 10, 20, 30, 40})
        grid_test(csv, g, b, 0, c, seed, config);
}

constexpr unsigned N_CONFIGS = 4;

constexpr std::array<Config, N_CONFIGS> configs = []{
    std::array<Config, N_CONFIGS> ret;
    int i = 0;
    for (bool prep : {false, true})
    for (double f : {1.5, 2.0})
        ret[i++] = Config(prep, f);
    return ret;
}();

const vector<string> all_columns = []{
    vector<string> columns = {"g", "b", "a", "c", "seed", "config", "result", "makespan"};
    columns.insert(columns.end(), Stats::fields.begin(), Stats::fields.end());
    return columns;
}();

int main(int argc, char **argv)
{
    std::ofstream out;
    Csv csv_instance(all_columns, out);
    Csv *csv = nullptr;

    if (argc > 2) {
        char *outfile;
        if (argc == 3)
            outfile = argv[2];
        else
            outfile = argv[1];
        out.open(outfile);
        csv = &csv_instance;
        csv->write_header();
    }

    if (argc == 1) {
        grid_input(nullptr);
        return 0;
    }
    if (argc < 4) {
        switch (atoi(argv[1])) {
            case 0:
                grid_input(csv);
                break;
            case 1:
                example(csv);
                break;
            case 2:
                counter(csv);
                break;
            if (csv)
                csv->write();
            return 0;
        }
    }
    if (argc == 4 && argv[1][0] == 'r') {
        regular_mapf(csv, argv[3]);
        return 0;
    }

    int g = atoi(argv[2]);
    auto seed = argv[3];

    Config conf;

    if (argc > 4)
        conf = configs[atoi(argv[4])];

    for (int b : {10, 20})
    for (int a : range(1, 10 + 1))
    for (int c : range(1, 10 + 1))
    if (g * g - from_percentage(g, b) >= std::max(a, c))
        grid_test(csv, g, b, a, c, seed, conf);
}
