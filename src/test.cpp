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
            csv->write();
        }
        solution.stats.print();
        return solution;
    }
    catch(unsolvable_e) {
        if (csv) {
            csv->set("result", "Unsolvable");
            csv->write();
        }
        std::cout << std::endl << "Unsolvable!" << std::endl;
    }
    catch(partially_solved &e) {
        auto t = e.what();
        if (csv) {
            csv->set("result", t);
            for (auto s : e.stats.get_all())
                csv->set(std::move(s.first), s.second);
            csv->write();
        }
        std::cout << std::endl << t << std::endl;
        e.stats.print();
    }
    return std::nullopt;
}

void example(Csv *csv = nullptr) // Figure 1
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

void counter(Csv *csv = nullptr) // Figure 2
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

void grid_input(Csv *csv = nullptr, bool transport = true)
{
    int g, b, a = 0, c;
    string s;
    std::cout << std::endl;
    std::cout << "√ Grid size:  "; std::cin >> g;
    std::cout << "% Blockades:  "; std::cin >> b;
    if (transport) { std::cout << "# Agents:     "; std::cin >> a; }
    std::cout << "# Containers: "; std::cin >> c;
    std::cout << "X Seed:       "; std::cin.ignore(); std::getline(std::cin, s);
    std::cout << std::endl;

    constexpr static Config mat;
    constexpr static Config mapf = []{ Config conf; conf.transport = false; return conf; }();

    auto solution = solve(csv, transport ? mat : mapf, g, from_percentage(g, b), a, c, s);

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
    Csv csv(all_columns, out);
    int i = 0;

    auto get_csv = [&]() -> Csv* {
        if (argc > i + 1) {
            out.open(argv[++i]);
            csv.write_header();
            return &csv;
        }
        return nullptr;
    };

    bool transport = true;

    command:
    if (argc > i + 1) switch (tolower(argv[++i][0]))
    {
    case 'r'/*egular MAPF*/:
        transport = false;
        goto command;
    case '0':
        break;
    case /*Figure */'1':
        example(get_csv());
        return 0;
    case /*Figure */'2':
        counter(get_csv());
        return 0;
    case 'b'/*arták*/:
        {
            string s;
            if (argc > i + 2 and argv[++i][0] == 's')
                s = argv[++i];
            regular_mapf(get_csv(), s);
        }
        return 0;
    case 'i'/*interactive*/:
        grid_input(get_csv(), transport);
        return 0;
    }
    else {
        grid_input();
        return 0;
    }

    int g = 7;
    string s;
    Config conf;

    option:
    if (argc > i + 2) switch (tolower(argv[++i][0]))
    {
    case 'g'/*rid size*/:
        g = atoi(argv[++i]);
        goto option;
    case 's'/*eed*/:
        s = argv[++i];
        goto option;
    case 'c'/*onfig*/:
        conf = configs[atoi(argv[++i])];
        goto option;
    }

    conf.transport = transport;
    auto csv_p = get_csv();

    for (int b : {10, 20})
    for (int a : transport ? range(1, 10 + 1) : range(1))
    for (int c : range(1, 10 + 1))
    if (g * g - from_percentage(g, b) >= std::max(a, c))
        grid_test(csv_p, g, b, a, c, s, conf);
}
