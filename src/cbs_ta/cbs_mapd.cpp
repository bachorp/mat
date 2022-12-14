#include <fstream>
#include <iostream>

#include <boost/functional/hash.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/graphviz.hpp>

#include "cbs_mapd.hpp"
#include "next_best_assignment.hpp"
#include "timer.hpp"

#include "../Problem.hpp"

using libMultiRobotPlanning::CBSTA;
using libMultiRobotPlanning::Neighbor;
using libMultiRobotPlanning::PlanResult;
using libMultiRobotPlanning::NextBestAssignment;

///

enum class TransportStatus {
    Approach,
    Delivery,
    Done,
};

std::ostream& operator<<(std::ostream& os, const TransportStatus& s) {
  switch (s) {
    case TransportStatus::Approach:
      os << "Approach";
      break;
    case TransportStatus::Delivery:
      os << "Delivery";
      break;
    case TransportStatus::Done:
      os << "Done";
      break;
  }
  return os;
}

///

struct State {
  State(int time, int x, int y, TransportStatus ts) : time(time), x(x), y(y), ts(ts) {}

  bool operator==(const State& s) const {
    return time == s.time && x == s.x && y == s.y && ts == s.ts;
  }

  bool conflicts(const State& s) const { return x == s.x && y == s.y; }

  friend std::ostream& operator<<(std::ostream& os, const State& s) {
    return os << s.time << ": (" << s.x << "," << s.y << ") [" << s.ts << "]";
    // return os << "(" << s.x << "," << s.y << ")";
  }

  int time;
  int x;
  int y;
  TransportStatus ts;
};

namespace std {
template <>
struct hash<State> {
  size_t operator()(const State& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.time);
    boost::hash_combine(seed, s.x);
    boost::hash_combine(seed, s.y);
    boost::hash_combine(seed, s.ts);
    return seed;
  }
};
}  // namespace std

///
enum class Action {
  Up,
  Down,
  Left,
  Right,
  Wait,
};

std::ostream& operator<<(std::ostream& os, const Action& a) {
  switch (a) {
    case Action::Up:
      os << "Up";
      break;
    case Action::Down:
      os << "Down";
      break;
    case Action::Left:
      os << "Left";
      break;
    case Action::Right:
      os << "Right";
      break;
    case Action::Wait:
      os << "Wait";
      break;
  }
  return os;
}

///

struct Conflict {
  enum Type {
    Vertex,
    Edge,
  };

  int time;
  size_t agent1;
  size_t agent2;
  Type type;

  int x1;
  int y1;
  int x2;
  int y2;

  friend std::ostream& operator<<(std::ostream& os, const Conflict& c) {
    switch (c.type) {
      case Vertex:
        return os << c.time << ": Vertex(" << c.x1 << "," << c.y1 << ")";
      case Edge:
        return os << c.time << ": Edge(" << c.x1 << "," << c.y1 << "," << c.x2
                  << "," << c.y2 << ")";
    }
    return os;
  }
};

struct VertexConstraint {
  VertexConstraint(int time, int x, int y) : time(time), x(x), y(y) {}
  int time;
  int x;
  int y;

  bool operator<(const VertexConstraint& other) const {
    return std::tie(time, x, y) < std::tie(other.time, other.x, other.y);
  }

  bool operator==(const VertexConstraint& other) const {
    return std::tie(time, x, y) == std::tie(other.time, other.x, other.y);
  }

  friend std::ostream& operator<<(std::ostream& os, const VertexConstraint& c) {
    return os << "VC(" << c.time << "," << c.x << "," << c.y << ")";
  }
};

namespace std {
template <>
struct hash<VertexConstraint> {
  size_t operator()(const VertexConstraint& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.time);
    boost::hash_combine(seed, s.x);
    boost::hash_combine(seed, s.y);
    return seed;
  }
};
}  // namespace std

struct EdgeConstraint {
  EdgeConstraint(int time, int x1, int y1, int x2, int y2)
      : time(time), x1(x1), y1(y1), x2(x2), y2(y2) {}
  int time;
  int x1;
  int y1;
  int x2;
  int y2;

  bool operator<(const EdgeConstraint& other) const {
    return std::tie(time, x1, y1, x2, y2) <
           std::tie(other.time, other.x1, other.y1, other.x2, other.y2);
  }

  bool operator==(const EdgeConstraint& other) const {
    return std::tie(time, x1, y1, x2, y2) ==
           std::tie(other.time, other.x1, other.y1, other.x2, other.y2);
  }

  friend std::ostream& operator<<(std::ostream& os, const EdgeConstraint& c) {
    return os << "EC(" << c.time << "," << c.x1 << "," << c.y1 << "," << c.x2
              << "," << c.y2 << ")";
  }
};

namespace std {
template <>
struct hash<EdgeConstraint> {
  size_t operator()(const EdgeConstraint& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.time);
    boost::hash_combine(seed, s.x1);
    boost::hash_combine(seed, s.y1);
    boost::hash_combine(seed, s.x2);
    boost::hash_combine(seed, s.y2);
    return seed;
  }
};
}  // namespace std

struct Constraints {
  std::unordered_set<VertexConstraint> vertexConstraints;
  std::unordered_set<EdgeConstraint> edgeConstraints;

  void add(const Constraints& other) {
    vertexConstraints.insert(other.vertexConstraints.begin(),
                             other.vertexConstraints.end());
    edgeConstraints.insert(other.edgeConstraints.begin(),
                           other.edgeConstraints.end());
  }

  bool overlap(const Constraints& other) const {
    for (const auto& vc : vertexConstraints) {
      if (other.vertexConstraints.count(vc) > 0) {
        return true;
      }
    }
    for (const auto& ec : edgeConstraints) {
      if (other.edgeConstraints.count(ec) > 0) {
        return true;
      }
    }
    return false;
  }

  friend std::ostream& operator<<(std::ostream& os, const Constraints& c) {
    for (const auto& vc : c.vertexConstraints) {
      os << vc << std::endl;
    }
    for (const auto& ec : c.edgeConstraints) {
      os << ec << std::endl;
    }
    return os;
  }
};

struct Location {
  Location() = default;
  Location(int x, int y) : x(x), y(y) {}
  int x;
  int y;

  bool operator<(const Location& other) const {
    return std::tie(x, y) < std::tie(other.x, other.y);
  }

  bool operator==(const Location& other) const {
    return std::tie(x, y) == std::tie(other.x, other.y);
  }

  friend std::ostream& operator<<(std::ostream& os, const Location& c) {
    return os << "(" << c.x << "," << c.y << ")";
  }
};

namespace std {
template <>
struct hash<Location> {
  size_t operator()(const Location& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.x);
    boost::hash_combine(seed, s.y);
    return seed;
  }
};
}  // namespace std

struct Container {
  Container() = default;
  Container(Location start, Location goal) : start(start), goal(goal) {}
  Location start;
  Location goal;

  bool operator<(const Container& other) const {
    return std::tie(start, goal) < std::tie(other.start, other.goal);
  }

  bool operator==(const Container& other) const {
    return std::tie(start, goal) == std::tie(other.start, other.goal);
  }

  friend std::ostream& operator<<(std::ostream& os, const Container& c) {
    return os << "[" << c.start << "-" << c.goal << "]";
  }
};

namespace std {
template <>
struct hash<Container> {
  size_t operator()(const Container& s) const {
    size_t seed = 0;
    boost::hash_combine(seed, s.start.x);
    boost::hash_combine(seed, s.start.y);
    boost::hash_combine(seed, s.goal.x);
    boost::hash_combine(seed, s.goal.y);
    return seed;
  }
};
}  // namespace std

class ShortestPathHeuristic {
 public:
  ShortestPathHeuristic(size_t dimx, size_t dimy,
                        const std::unordered_set<Location>& obstacles)
          : m_shortestDistance(nullptr), m_dimx(dimx), m_dimy(dimy) {
    searchGraph_t searchGraph;

    // add vertices
    for (size_t x = 0; x < dimx; ++x) {
      for (size_t y = 0; y < dimy; ++y) {
        boost::add_vertex(searchGraph);
      }
    }

    // add edges
    for (size_t x = 0; x < dimx; ++x) {
      for (size_t y = 0; y < dimy; ++y) {
        Location l(x, y);
        if (obstacles.find(l) == obstacles.end()) {
          Location right(x + 1, y);
          if (x < dimx - 1 && obstacles.find(right) == obstacles.end()) {
            auto e =
                    boost::add_edge(locToVert(l), locToVert(right), searchGraph);
            searchGraph[e.first].weight = 1;
          }
          Location below(x, y + 1);
          if (y < dimy - 1 && obstacles.find(below) == obstacles.end()) {
            auto e =
                    boost::add_edge(locToVert(l), locToVert(below), searchGraph);
            searchGraph[e.first].weight = 1;
          }
        }
      }
    }

    writeDotFile(searchGraph, "searchGraph.dot");

    m_shortestDistance = new distanceMatrix_t(boost::num_vertices(searchGraph));
    distanceMatrixMap_t distanceMap(*m_shortestDistance, searchGraph);
    // The following generates a clang-tidy error, see
    // https://svn.boost.org/trac10/ticket/10830
    boost::floyd_warshall_all_pairs_shortest_paths(
            searchGraph, distanceMap, boost::weight_map(boost::get(&Edge::weight, searchGraph)));
  }

  ~ShortestPathHeuristic() { delete m_shortestDistance; }

  int getValue(const Location& a, const Location& b) {
    vertex_t idx1 = locToVert(a);
    vertex_t idx2 = locToVert(b);
    return (*m_shortestDistance)[idx1][idx2];
  }

 private:
  size_t locToVert(const Location& l) const { return l.x + m_dimx * l.y; }

  Location idxToLoc(size_t idx) {
    int x = idx % m_dimx;
    int y = idx / m_dimx;
    return Location(x, y);
  }

 private:
  typedef boost::adjacency_list_traits<boost::vecS, boost::vecS,
          boost::undirectedS>
          searchGraphTraits_t;
  typedef searchGraphTraits_t::vertex_descriptor vertex_t;
  typedef searchGraphTraits_t::edge_descriptor edge_t;

  struct Vertex {};

  struct Edge {
      int weight;
  };

  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
          Vertex, Edge>
          searchGraph_t;
  typedef boost::exterior_vertex_property<searchGraph_t, int>
          distanceProperty_t;
  typedef distanceProperty_t::matrix_type distanceMatrix_t;
  typedef distanceProperty_t::matrix_map_type distanceMatrixMap_t;

  class VertexDotWriter {
  public:
    explicit VertexDotWriter(const searchGraph_t& graph, size_t dimx) : m_graph(graph), m_dimx(dimx) {}

    void operator()(std::ostream& out, const vertex_t& v) const {
      static const float DX = 100;
      static const float DY = 100;
      out << "[label=\"";
      int x = v % m_dimx;
      int y = v / m_dimx;
      out << "\" pos=\"" << x * DX << "," << y * DY << "!\"]";
    }

  private:
      const searchGraph_t& m_graph;
      size_t m_dimx;
  };

  class EdgeDotWriter {
  public:
    explicit EdgeDotWriter(const searchGraph_t& graph) : m_graph(graph) {}

    void operator()(std::ostream& out, const edge_t& e) const {
      out << "[label=\"" << m_graph[e].weight << "\"]";
    }

  private:
      const searchGraph_t& m_graph;
  };

private:
  void writeDotFile(const searchGraph_t& graph, const std::string& fileName) {
    VertexDotWriter vw(graph, m_dimx);
    EdgeDotWriter ew(graph);
    std::ofstream dotFile(fileName);
    boost::write_graphviz(dotFile, graph, vw, ew);
  }

private:
  distanceMatrix_t* m_shortestDistance;
  size_t m_dimx;
  size_t m_dimy;
};

///
class Environment {
 public:
  Environment(size_t dimx, size_t dimy,
              const std::unordered_set<Location>& obstacles,
              const std::vector<State>& startStates,
              const std::vector<std::unordered_set<Container> >& tasks,
              size_t maxTaskAssignments)
      : m_dimx(dimx),
        m_dimy(dimy),
        m_obstacles(obstacles),
        m_agentIdx(0),
        m_task(nullptr),
        m_constraints(nullptr),
        m_lastGoalConstraint(),
        m_maxTaskAssignments(maxTaskAssignments),
        m_numTaskAssignments(0),
        m_highLevelExpanded(0),
        m_lowLevelExpanded(0),
        m_heuristic(dimx, dimy, obstacles) {
    m_numAgents = startStates.size();
    for (size_t i = 0; i < startStates.size(); ++i) {
      for (const auto& task : tasks[i]) {
        assert(!(task.start == task.goal)); // trivial tasks should have been filtered
        auto cost =
            m_heuristic.getValue(Location(startStates[i].x, startStates[i].y),
                                 task.start) +
            m_heuristic.getValue(task.start, task.goal);
        if (cost >= 0) {
            m_assignment.setCost(i, task, cost);
        }
        m_tasks.insert(task);
      }
    }
    m_assignment.solve();
  }

  void setLowLevelContext(size_t agentIdx, const Constraints* constraints,
                          const Container* task) {
    assert(constraints);
    m_agentIdx = agentIdx;
    m_task = task;
    m_constraints = constraints;
    m_lastGoalConstraint = {};
    if (m_task != nullptr) {
      assert(m_obstacles.find(m_task->goal) == m_obstacles.end());
      for (const auto& vc : constraints->vertexConstraints) {
        if (vc.x == m_task->goal.x && vc.y == m_task->goal.y) {
          auto insert = m_lastGoalConstraint.insert(std::make_pair(m_task->goal, vc.time));
          if (!insert.second) {
             m_lastGoalConstraint[m_task->goal] = std::max(m_lastGoalConstraint.at(m_task->goal), vc.time);
          }
        }
      }
    } else {
      for (const auto& vc : constraints->vertexConstraints) {
        auto insert = m_lastGoalConstraint.insert(std::make_pair(Location(vc.x, vc.y), vc.time));
        if (!insert.second) {
          m_lastGoalConstraint[Location(vc.x, vc.y)] = std::max(m_lastGoalConstraint.at(Location(vc.x, vc.y)), vc.time);
        }
      }
    }
    // std::cout << "setLLCtx: " << agentIdx << " " << m_lastGoalConstraint <<
    // std::endl;
  }

  int admissibleHeuristic(const State& s) {
    if (m_task == nullptr || s.ts == TransportStatus::Done) {
      return 0;
    }
    if (s.ts == TransportStatus::Delivery) {
      return m_heuristic.getValue(Location(s.x, s.y), m_task->goal);
    }
    assert(s.ts == TransportStatus::Approach);
    return m_heuristic.getValue(Location(s.x, s.y), m_task->start) +
           m_heuristic.getValue(m_task->start, m_task->goal);
  }
  
  bool isSolution(const State& s) {
    if (m_task == nullptr || getNextStatus(s) == TransportStatus::Done) {
      // if the agent has no task, just find a place where it can stay
      auto search = m_lastGoalConstraint.find(Location(s.x, s.y));
      if (search != m_lastGoalConstraint.end()) {
        return s.time > search->second;
      }
      return true;
    }
    return false;
  }

  void getNeighbors(const State& s,
                    std::vector<Neighbor<State, Action, int> >& neighbors) {
    // std::cout << "#VC " << constraints.vertexConstraints.size() << std::endl;
    // for(const auto& vc : constraints.vertexConstraints) {
    //   std::cout << "  " << vc.time << "," << vc.x << "," << vc.y <<
    //   std::endl;
    // }
    // TODO: Currently agents are forced to pick up their assigned container once they reach it.
    //       As far as I can tell, that does not make a difference in our setting, but in the long run it might be
    //       preferable to introduce a 'PickUp' action (but would need major refactoring)
    neighbors.clear();
    auto nextStatus = getNextStatus(s);
    {
      State n(s.time + 1, s.x, s.y, nextStatus);
      if (stateValid(n) && transitionValid(s, n)) {
        neighbors.emplace_back(
            Neighbor<State, Action, int>(n, Action::Wait, 1));
      }
    }
    {
      State n(s.time + 1, s.x - 1, s.y, nextStatus);
      if (stateValid(n) && transitionValid(s, n)) {
        neighbors.emplace_back(
            Neighbor<State, Action, int>(n, Action::Left, 1));
      }
    }
    {
      State n(s.time + 1, s.x + 1, s.y, nextStatus);
      if (stateValid(n) && transitionValid(s, n)) {
        neighbors.emplace_back(
            Neighbor<State, Action, int>(n, Action::Right, 1));
      }
    }
    {
      State n(s.time + 1, s.x, s.y + 1, nextStatus);
      if (stateValid(n) && transitionValid(s, n)) {
        neighbors.emplace_back(Neighbor<State, Action, int>(n, Action::Up, 1));
      }
    }
    {
      State n(s.time + 1, s.x, s.y - 1, nextStatus);
      if (stateValid(n) && transitionValid(s, n)) {
        neighbors.emplace_back(
            Neighbor<State, Action, int>(n, Action::Down, 1));
      }
    }
  }

  bool getFirstConflict(
      const std::vector<PlanResult<State, Action, int> >& solution,
      Conflict& result) {
    int max_t = 0;
    for (const auto& sol : solution) {
      max_t = std::max<int>(max_t, sol.states.size());
    }

    for (int t = 0; t < max_t; ++t) {
      // check drive-drive vertex collisions
      for (size_t i = 0; i < solution.size(); ++i) {
        State state1 = getState(i, solution, t);
        for (size_t j = i + 1; j < solution.size(); ++j) {
          State state2 = getState(j, solution, t);
          if (state1.conflicts(state2)) {
            result.time = t;
            result.agent1 = i;
            result.agent2 = j;
            result.type = Conflict::Vertex;
            result.x1 = state1.x;
            result.y1 = state1.y;
            // std::cout << "VC " << t << "," << state1.x << "," << state1.y <<
            // std::endl;
            return true;
          }
        }
      }
      // drive-drive edge (swap)
      for (size_t i = 0; i < solution.size(); ++i) {
        State state1a = getState(i, solution, t);
        State state1b = getState(i, solution, t + 1);
        for (size_t j = i + 1; j < solution.size(); ++j) {
          State state2a = getState(j, solution, t);
          State state2b = getState(j, solution, t + 1);
          if (state1a.conflicts(state2b) &&
              state1b.conflicts(state2a)) {
            result.time = t;
            result.agent1 = i;
            result.agent2 = j;
            result.type = Conflict::Edge;
            result.x1 = state1a.x;
            result.y1 = state1a.y;
            result.x2 = state1b.x;
            result.y2 = state1b.y;
            return true;
          }
        }
      }
    }

    return false;
  }

  void createConstraintsFromConflict(
      const Conflict& conflict, std::map<size_t, Constraints>& constraints) {
    if (conflict.type == Conflict::Vertex) {
      Constraints c1;
      c1.vertexConstraints.emplace(
          VertexConstraint(conflict.time, conflict.x1, conflict.y1));
      constraints[conflict.agent1] = c1;
      constraints[conflict.agent2] = c1;
    } else if (conflict.type == Conflict::Edge) {
      Constraints c1;
      c1.edgeConstraints.emplace(EdgeConstraint(
          conflict.time, conflict.x1, conflict.y1, conflict.x2, conflict.y2));
      constraints[conflict.agent1] = c1;
      Constraints c2;
      c2.edgeConstraints.emplace(EdgeConstraint(
          conflict.time, conflict.x2, conflict.y2, conflict.x1, conflict.y1));
      constraints[conflict.agent2] = c2;
    }
  }

  void nextTaskAssignment(std::map<size_t, Container>& tasks) {
    if (m_numTaskAssignments > m_maxTaskAssignments) {
      return;
    }

    /* int64_t cost = */ m_assignment.nextSolution(tasks);
    if (!tasks.empty()) {
      // std::cout << "nextTaskAssignment: cost: " << cost << std::endl;
      // for (const auto& s : tasks) {
      //   std::cout << s.first << "->" << s.second << std::endl;
      // }

      ++m_numTaskAssignments;
    }
  }

  void onExpandHighLevelNode(int /*cost*/) { m_highLevelExpanded++; }

  void onExpandLowLevelNode(const State& /*s*/, int /*fScore*/,
                            int /*gScore*/) {
    m_lowLevelExpanded++;
  }

  int highLevelExpanded() { return m_highLevelExpanded; }

  int lowLevelExpanded() const { return m_lowLevelExpanded; }

  size_t numTaskAssignments() const { return m_numTaskAssignments; }

 private:
  State getState(size_t agentIdx,
                 const std::vector<PlanResult<State, Action, int> >& solution,
                 size_t t) {
    assert(agentIdx < solution.size());
    if (t < solution[agentIdx].states.size()) {
      return solution[agentIdx].states[t].first;
    }
    assert(!solution[agentIdx].states.empty());
    return solution[agentIdx].states.back().first;
  }

  bool stateValid(const State& s) {
    assert(m_constraints);
    const auto& con = m_constraints->vertexConstraints;
    return s.x >= 0 && s.x < m_dimx && s.y >= 0 && s.y < m_dimy &&
           m_obstacles.find(Location(s.x, s.y)) == m_obstacles.end() &&
           con.find(VertexConstraint(s.time, s.x, s.y)) == con.end();
  }

  bool transitionValid(const State& s1, const State& s2) {
    assert(m_constraints);
    const auto& con = m_constraints->edgeConstraints;
    return con.find(EdgeConstraint(s1.time, s1.x, s1.y, s2.x, s2.y)) ==
           con.end();
  }


  TransportStatus getNextStatus(const State& s) {
    if (m_task == nullptr) {
      return TransportStatus::Done;
    }
    if (s.ts == TransportStatus::Approach && s.x == m_task->start.x && s.y == m_task->start.y) {
      return TransportStatus::Delivery;
    }
    if (s.ts == TransportStatus::Delivery && s.x == m_task->goal.x && s.y == m_task->goal.y) {
      return TransportStatus::Done;
    }
    return s.ts;
  }

 private:
  int m_dimx;
  int m_dimy;
  std::unordered_set<Location> m_obstacles;
  size_t m_agentIdx;
  const Container* m_task;
  const Constraints* m_constraints;
  std::unordered_map<Location, int> m_lastGoalConstraint;
  NextBestAssignment<size_t, Container> m_assignment;
  size_t m_maxTaskAssignments;
  size_t m_numTaskAssignments;
  int m_highLevelExpanded;
  int m_lowLevelExpanded;
  ShortestPathHeuristic m_heuristic;
  size_t m_numAgents;
  std::unordered_set<Container> m_tasks;
};

Location vToLoc(int v, int g) {
    return {v/g, v%g};
}

constexpr int from_percentage(int g, float b) {
  return b / 100 * g * g + .5; // rounding
}

template <typename T = std::string>
bool buildProblem(int g, int b, int a, int c, T seed,
                  std::unordered_set<Location>& obstacles,
                  std::vector<State>& agentStarts,
                  std::vector<std::unordered_set<Container>>& tasks) {
  vector<int> nodes;
  vector<int> cStarts;
  std::unordered_set<Container> containers;
  for (auto v : range(g*g))
    nodes.push_back(v);
  std::stringstream ss;
  static const auto sep = ",";
  ss << g << sep << b << sep << a << sep << c << sep << seed;
  std::mt19937 r(hashCode(ss.str()));
  shuffle_(nodes.begin(), nodes.end(), r);
  cStarts.resize(c + a);
  std::copy(nodes.begin(), nodes.begin() + c, cStarts.begin());
  shuffle_(nodes.begin(), nodes.end() - b, r);
  for (auto it = nodes.begin(); it < nodes.begin() + a; ++it) {
    auto l = vToLoc(*it, g);
    agentStarts.emplace_back(0, l.x, l.y, TransportStatus::Approach);
  }
  shuffle_(nodes.begin(), nodes.end() - b, r);
  for (auto i : range(c)) {
    auto start = vToLoc(cStarts.at(i), g);
    auto goal = vToLoc(nodes.at(i), g);
    if (!(start == goal)) {
      containers.emplace(start, goal);
    }
  }
  if (containers.empty()) {
    return false; // return false for trivial problems
  }
  tasks.resize(a);
  std::fill(tasks.begin(), tasks.end(), containers);
  for (auto it = nodes.end() - b; it < nodes.end(); ++it) {
    obstacles.insert(vToLoc(*it, g));
  }
  return true;
}

template <typename T = std::string>
void solve(int g, int b, int a, int c, int t, std::string o, std::string m, T seed = "", bool p = false)
{
  std::cout << "????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????" << std::endl;
  std::printf("g = %d, b = %d, a = %d, c = %d, seed = ", g, b, a, c);
  std::cout << seed;
  std::cout << std::endl;
  Timer tTimer;

  std::unordered_set<Location> obstacles;
  std::vector<std::unordered_set<Container> > tasks;
  std::vector<State> startStates;

  bool success = false;
  std::vector<PlanResult<State, Action, int> > solution;
  std::map<size_t, Container> taskAssignment;

  int hExpandend = 0;
  int lExpanded = 0;
  int nAssignments = 0;
  std::stringstream sTime;
  if (buildProblem(g, from_percentage(g, b), a, c, seed, obstacles, startStates, tasks)) {
    Environment mapf(g, g, obstacles, startStates, tasks, 1e9);
    CBSTA<State, Action, int, Conflict, Constraints, Container, Environment>
            cbs(mapf);

    Timer sTimer;
    try {
      success = cbs.search(startStates, solution, taskAssignment, t);
    } catch (const std::runtime_error& e) {
      if (std::string(e.what()) == "Timeout") {
      }
    }

    sTimer.stop();
    sTime << std::fixed << std::setprecision(3) << float(sTimer.elapsedSeconds());

    hExpandend = mapf.highLevelExpanded();
    lExpanded = mapf.lowLevelExpanded();
    nAssignments = mapf.numTaskAssignments();
  } else {
    success = true;
    sTime << std::fixed << std::setprecision(3) << 0.0;
  }


  tTimer.stop();
  std::stringstream tTime;
  tTime << std::fixed << std::setprecision(3) << float(tTimer.elapsedSeconds());

  if (success) {
    std::cout << "Planning successful! " << std::endl;
    int64_t cost = 0;
    int64_t makespan = 0;
    for (const auto& s : solution) {
      cost += s.cost;
      makespan = std::max<int64_t>(makespan, s.cost);
    }

    if (!o.empty()) {
      std::ofstream out(o);
      out << "statistics:" << std::endl;
      out << "  sum-of-costs: " << cost << std::endl;
      out << "  makespan: " << makespan << std::endl;
      out << "  t_total: " << tTime.str() << std::endl;
      out << "  t_solver: " << sTime.str() << std::endl;
      out << "  highLevelExpanded: " << hExpandend << std::endl;
      out << "  lowLevelExpanded: " << lExpanded << std::endl;
      out << "  numTaskAssignments: " << nAssignments << std::endl;
      if (p) {
        out << "assignment:" << std::endl;
        for (const auto& s : taskAssignment) {
          out << "  agent" <<  s.first << ":" << std::endl
              << "    - start:" << std::endl
              << "      - x: " << s.second.start.x << std::endl
              << "        y: " << s.second.start.y << std::endl
              << "    - goal:" << std::endl
              << "      - x: " << s.second.goal.x << std::endl
              << "        y: " << s.second.goal.y << std::endl;
        }
        out << "schedule:" << std::endl;
        for (size_t a = 0; a < solution.size(); ++a) {
          // std::cout << "Solution for: " << a << std::endl;
          // for (size_t i = 0; i < solution[a].actions.size(); ++i) {
          //   std::cout << solution[a].states[i].second << ": " <<
          //   solution[a].states[i].first << "->" << solution[a].actions[i].first
          //   << "(cost: " << solution[a].actions[i].second << ")" << std::endl;
          // }
          // std::cout << solution[a].states.back().second << ": " <<
          // solution[a].states.back().first << std::endl;

          out << "  agent" << a << ":" << std::endl;
          for (const auto& state : solution[a].states) {
            out << "    - x: " << state.first.x << std::endl
                << "      y: " << state.first.y << std::endl
                << "      t: " << state.second << std::endl;
          }
        }
      }
    }
    // print a yaml scenario file
    if (!m.empty()) {
      std::ofstream out(m);
      out << "map:" << std::endl;
      out << "  dimensions: [" << g << ", " << g << "]" << std::endl;
      out << "  obstacles:" << std::endl;
      for (const auto& obs : obstacles) {
        out << "    - [" << obs.x << ", " << obs.y << "]" << std::endl;
      }
      out << "agents:" << std::endl;
      for (size_t aIdx = 0; aIdx < startStates.size(); aIdx++) {
        out << "  - name: agent" << aIdx << std::endl;
        out << "    start: [" << startStates.at(aIdx).x << ", " << startStates.at(aIdx).y << "]" << std::endl;
        out << "    potentialTasks:" << std::endl;
        for (const auto& container : tasks.at(aIdx)) {
          out << "      - [[" << container.start.x << ", " << container.start.y << "], ["
                              << container.goal.x << ", " << container.goal.y << "]]" << std::endl;
        }
      }
    }
  } else {
      std::cout << "Planning NOT successful!" << std::endl;
  }

}

int main(int argc, char **argv)
{
  int i = 0;

  int g = 7;
  string s;
  int b = 10;
  int a = 3;
  int c = 3;
  int t = 0; // timeout TODO: not sure if we want to keep that option, it's implemented in a very hacky way
  std::string o; // output file (yaml)
  std::string m; // output map file (yaml)

  option:
  if (argc > i + 1)
    switch (tolower(argv[++i][0]))
    {
      case 'g' /*rid size*/:
        g = atoi(argv[++i]);
        goto option;
      case 's' /*eed*/:
        s = argv[++i];
        goto option;
      case 'b' /*locked*/:
        b = atoi(argv[++i]);
        goto option;
      case 'a' /*gent number*/:
        a = atoi(argv[++i]);
        goto option;
      case 'c' /*ontainer number*/:
        c = atoi(argv[++i]);
        goto option;
      case 't' /*imeout*/:
        t = atoi(argv[++i]);
        goto option;
      case 'o' /*utput file*/:
        o = argv[++i];
        goto option;
      case 'm' /*ap print*/:
        m = argv[++i];
        goto option;
    }

  solve(g, b, a, c, t, o, m, s, true);
}
