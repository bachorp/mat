#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace libMultiRobotPlanning {

template <typename Agent, typename Task>
class Assignment {
public:
    Assignment()
            : m_costs(), m_agents(), m_tasks(), m_makespanQueue(), m_prioAgents() {}

    void clear() {
        m_costs.clear();
        m_agents.clear();
        m_tasks.clear();
        m_makespanQueue.clear();
        m_prioAgents.clear();
    }

    void setCost(const Agent& agent, const Task& task, long cost) {
        m_agents.insert(agent);
        m_tasks.insert(task);
        m_costs[agent][task] = cost;
        m_makespanQueue.insert(cost);
    }

    void prioritizeAgent(const Agent& agent) {
        m_prioAgents.insert(agent);
    }

    // find first (optimal) solution with minimal cost
    long solve(std::map<Agent, Task>& solution, long curMakespan) {
        solution.clear();
        // std::cout << "Computing assignment for:" << std::endl;
        // std::cout << "  Agents:";
        // for (const auto& a : m_agents) {
        //   std::cout << " " << a;
        //   if (m_prioAgents.find(a) != m_prioAgents.end()) std::cout << "(p)";
        // }
        // std::cout << std::endl;
        // std::cout << "  Tasks:";
        // for (const auto& t : m_tasks) {
        //   std::cout << " " << t;
        // }
        // std::cout << std::endl;
        // std::cout << "  Costs:" << std::endl;
        // for (const auto& e : m_costs) {
        //   std::cout << "    " << e.first << ":";
        //   for (const auto& c : e.second) {
        //     std::cout << " " << c.first << "/" << c.second;
        //   }
        //   std::cout << std::endl;
        // }
        // std::cout << "  Min makespan: " << curMakespan << std::endl;
        if (m_prioAgents.size() > m_tasks.size()) {
            return 0;
        }

        for (auto ms : m_makespanQueue) {
            if (ms < curMakespan) {
                continue;
            }
            if (m_prioAgents.empty() || solveBounded(m_prioAgents, solution, ms)) {
                if (solveBounded(m_agents, solution, ms)) {
                    return ms;
                }
            }
        }
        return 0;
    }

private:

    // Hopcroft-Karp
    bool solveBounded(std::unordered_set<Agent> agents,
                      std::map<Agent, Task>& solution, long maxMakespan) {

        if (m_tasks.size() > m_agents.size()) {
          return false;
        }
        // std::cout << "Checking max makespan " << maxMakespan << ": " << std::endl;
        std::unordered_map<Agent, std::vector<Task>> agentMatches;
        for (const auto& aAsgn : m_costs) {
            for (const auto& tAsgn : aAsgn.second) {
                if (tAsgn.second <= maxMakespan) {
                    Agent a = aAsgn.first;
                    Task t = tAsgn.first;
                    auto aIt = agentMatches.insert({a, {}}).first;
                    aIt->second.push_back(t);
                }
            }
        }
        // std::cout << "  Possible matchings:" << std::endl;
        // for (const auto& m : agentMatches) {
        //   std::cout << "    " << m.first << " :";
        //   for (const auto& t : m.second) std::cout << " " << t;
        //   std::cout << std::endl;
        // }

        std::unordered_map<Agent, Task> agentAssignment;
        std::unordered_map<Task, Agent> taskAssignment;

        // This ensures that all agents that already have a task, will again get one
        for (const auto& pair : solution) {
            agentAssignment.insert(pair);
            taskAssignment.insert(std::make_pair(pair.second, pair.first));
        }

        solution.clear();

        std::unordered_map<Agent, size_t> layerMap;
        size_t numMatched = taskAssignment.size();

        while(true) {
            // BFS
            std::queue<Agent> q;
            for (const Agent& a : agents) {
                agentMatches.insert({a, {}});
                if (agentAssignment.find(a) == agentAssignment.end()) {
                    layerMap[a] = 0;
                    q.push(a);
                } else {
                    layerMap[a] = SIZE_MAX;
                }
            }
            size_t k = SIZE_MAX;
            while (!q.empty()) {
                auto a = q.front();
                size_t l = layerMap.at(a);
                q.pop();
                if (l < k) {
                    for (const Task& t : agentMatches.at(a)) {
                        auto asgnIt = taskAssignment.find(t);
                        if (asgnIt == taskAssignment.end()) {
                            k = l + 1;
                            if (k == SIZE_MAX) {
                                // In case we get more layers than numbers (pretty unlikely)
                                return false;
                            }
                            continue;
                        }
                        assert(agents.find(asgnIt->second) != agents.end());
                        if (layerMap.at(asgnIt->second) == SIZE_MAX) {
                            layerMap[asgnIt->second] = l + 1;
                            q.push(asgnIt->second);
                        }
                    }
                }
            }
            if (k == SIZE_MAX) {
                break;
            }
            // DFS
            for (const Agent& a : agents) {
                if (agentAssignment.find(a) == agentAssignment.end()) {
                    if (hkDFS(a, agentMatches, agentAssignment, taskAssignment,
                              layerMap)) {
                        numMatched++;
                    }
                }
            }
        }
        // std::cout << "  Best assignment:" << std::endl;
        // for (const auto& pair : agentAssignment) {
        //   std::cout << "    " << pair.first << " -> " << pair.second << std::endl;
        // }
        if (numMatched >= std::min(m_agents.size(), m_tasks.size())) {
            assert(numMatched == std::min(m_agents.size(), m_tasks.size()));
            for (const auto& pair : agentAssignment) {
                solution.insert(pair);
            }
            return true;
        }
        return false;
    }

    bool hkDFS(const Agent& a,
               const std::unordered_map<Agent, std::vector<Task>>& agentMatches,
               std::unordered_map<Agent, Task>& agentAssignment,
               std::unordered_map<Task, Agent>& taskAssignment,
               std::unordered_map<Agent, size_t>& layerMap) {
        for (const Task& t : agentMatches.at(a)) {
            auto asgnIt = taskAssignment.find(t);
            if (asgnIt == taskAssignment.end() ||
                (layerMap.at(asgnIt->second) == layerMap.at(a) + 1 &&
                 hkDFS(asgnIt->second, agentMatches, agentAssignment,
                       taskAssignment, layerMap))) {
                agentAssignment[a] = t;
                taskAssignment[t] = a;
                return true;
            }
        }
        layerMap[a] = SIZE_MAX;
        return false;
    }

    std::unordered_map<Agent, std::unordered_map<Task, long>> m_costs;

    std::unordered_set<Agent> m_agents;
    std::unordered_set<Task> m_tasks;

    std::set<long> m_makespanQueue;

    // agents that MUST get a task
    std::unordered_set<Agent> m_prioAgents;
};

template <typename Agent, typename Task, typename Assignment = Assignment<Agent, Task> >
class NextBestAssignment {
 public:
  NextBestAssignment(const Assignment& assignment = Assignment()) : m_assignment(assignment), m_cost(), m_open(), m_numMatching(0) {}

  void setCost(const Agent& agent, const Task& task, long cost) {
    // std::cout << "setCost: " << agent << "->" << task << ": " << cost <<
    // std::endl;
    m_cost[std::make_pair<>(agent, task)] = cost;
    if (m_agentsSet.find(agent) == m_agentsSet.end()) {
      m_agentsSet.insert(agent);
      m_agentsVec.push_back(agent);
    }
    // m_tasksSet.insert(task);
  }

  // find first (optimal) solution with minimal cost
  void solve() {
    const std::set<std::pair<Agent, Task> > I, O;
    const std::set<Agent> Iagents, Oagents;
    Node n;
    n.cost = constrainedMatching(I, O, Iagents, Oagents, n.solution, 0);
    m_open.emplace(n);
    m_numMatching = numMatching(n.solution);
  }

  // find next solution
  long nextSolution(std::map<Agent, Task>& solution) {
    solution.clear();
    if (m_open.empty()) {
      return std::numeric_limits<long>::max();
    }

    const Node next = m_open.top();
    // std::cout << "next: " << next << std::endl;
    m_open.pop();
    for (const auto& entry : next.solution) {
      solution.insert(std::make_pair(entry.first, entry.second));
    }
    long result = next.cost;

    std::set<Agent> fixedAgents;
    for (const auto c : next.I) {
      fixedAgents.insert(c.first);
    }

    // prepare for next query
    for (size_t i = 0; i < m_agentsVec.size(); ++i) {
      if (fixedAgents.find(m_agentsVec[i]) == fixedAgents.end()) {
        Node n;
        n.I = next.I;
        n.O = next.O;
        n.Iagents = next.Iagents;
        n.Oagents = next.Oagents;
        // fix assignment for agents 0...i
        for (size_t j = 0; j < i; ++j) {
          const Agent& agent = m_agentsVec[j];
          // n.I.insert(std::make_pair<>(agent, next.solution.at(agent)));
          const auto iter = solution.find(agent);
          if (iter != solution.end()) {
            n.I.insert(std::make_pair<>(agent, iter->second));
          } else {
            // this agent should keep having no solution =>
            // enforce that no task is allowed
            n.Oagents.insert(agent);
            // for (const auto& task : m_tasksSet) {
            //   n.O.insert(std::make_pair<>(agent, task));
            // }
          }
        }
        // n.O.insert(
        //     std::make_pair<>(m_agentsVec[i], next.solution.at(m_agentsVec[i])));
        const auto iter = solution.find(m_agentsVec[i]);
        if (iter != solution.end()) {
          n.O.insert(std::make_pair<>(m_agentsVec[i], iter->second));
        } else {
          // this agent should have a solution next
          // std::cout << "should have sol: " << m_agentsVec[i] << std::endl;
          n.Iagents.insert(m_agentsVec[i]);
        }
        // std::cout << " consider adding: " << n << std::endl;
        n.cost = constrainedMatching(n.I, n.O, n.Iagents, n.Oagents,
                                     n.solution, n.cost);
        if (n.solution.size() > 0) {
          m_open.push(n);
          // std::cout << "add: " << n << std::endl;
        }
      }
    }

    return result;
  }

 protected:
  // I enforces that the respective pair is part of the solution
  // O enforces that the respective pair is not part of the solution
  // Iagents enforces that these agents must have a task assignment
  // Oagents enforces that these agents should not have any task assignment
  long constrainedMatching(const std::set<std::pair<Agent, Task> >& I,
                           const std::set<std::pair<Agent, Task> >& O,
                           const std::set<Agent>& Iagents,
                           const std::set<Agent>& Oagents,
                           std::map<Agent, Task>& solution,
                           long minCost) {
    // prepare assignment problem

    m_assignment.clear();

    std::unordered_set<Agent> processedAgents;

    for (const auto& c : I) {
      if (Oagents.find(c.first) == Oagents.end()) {
        m_assignment.setCost(c.first, c.second, m_cost.at(c));
        m_assignment.prioritizeAgent(c.first);
        assert(processedAgents.find(c.first) == processedAgents.end());
        processedAgents.insert(c.first);
      }
    }

    for (const auto& c : m_cost) {
      if (Iagents.find(c.first.first) != Iagents.end()) {
        m_assignment.prioritizeAgent(c.first.first);
      }
      if (O.find(c.first) == O.end() &&
          processedAgents.find(c.first.first) == processedAgents.end() &&
          Oagents.find(c.first.first) == Oagents.end()) {
        m_assignment.setCost(c.first.first, c.first.second, c.second);
      }
    }


    m_assignment.solve(solution, minCost);
    size_t matching = numMatching(solution);

    // std::cout << "constrainedMatching: internal Solution: " << std::endl;
    // for (const auto& c : solution) {
    //   std::cout << "    " << c.first << "->" << c.second << std::endl;
    // }

    // check if all agents in Iagents have an assignment as requested
    bool solutionValid = true;
    for (const auto& agent : Iagents) {
      if (solution.find(agent) == solution.end()) {
        solutionValid = false;
        break;
      }
    }
    // check that I constraints have been fulfilled
    for (const auto& c : I) {
      const auto& iter = solution.find(c.first);
      if (iter == solution.end()
          || !(iter->second == c.second)) {
        solutionValid = false;
        break;
      }
    }

    if (!solutionValid || matching < m_numMatching) {
      solution.clear();
      return std::numeric_limits<long>::max();
    }
    return cost(solution);
  }

  long cost(const std::map<Agent, Task>& solution) {
    long result = 0;
    for (const auto& entry : solution) {
      result = std::max(m_cost.at(entry), result);
    }
    return result;
  }

  size_t numMatching(const std::map<Agent, Task>& solution) {
    return solution.size();
  }

 private:
  struct Node {
    Node() : I(), O(), Iagents(), Oagents(), solution(), cost(0) {}

    std::set<std::pair<Agent, Task> > I;  // enforced assignment
    std::set<std::pair<Agent, Task> > O;  // invalid assignments
    std::set<Agent> Iagents;  // agents that must have an assignment
    std::set<Agent> Oagents;  // agents that should not have an assignment
    std::map<Agent, Task> solution;
    long cost;

    bool operator<(const Node& n) const {
      // Our heap is a maximum heap, so we invert the comperator function here
      return cost > n.cost;
    }

    friend std::ostream& operator<<(std::ostream& os, const Node& n) {
      os << "Node with cost: " << n.cost << std::endl;
      os << "  I: ";
      for (const auto& c : n.I) {
        os << c.first << "->" << c.second << ",";
      }
      os << std::endl;
      os << "  O: ";
      for (const auto& c : n.O) {
        os << c.first << "->" << c.second << ",";
      }
      os << std::endl;
      os << "  Iagents: ";
      for (const auto& c : n.Iagents) {
        os << c << ",";
      }
      os << std::endl;
      os << "  Oagents: ";
      for (const auto& c : n.Oagents) {
        os << c << ",";
      }
      os << std::endl;
      os << "  solution: ";
      for (const auto& c : n.solution) {
        os << "    " << c.first << "->" << c.second << std::endl;
      }
      os << std::endl;
      return os;
    }
  };

 private:
  Assignment m_assignment;
  std::map<std::pair<Agent, Task>, long> m_cost;
  std::vector<Agent> m_agentsVec;
  std::set<Agent> m_agentsSet;
  // std::set<Task> m_tasksSet;
  // size_t m_numAgents;
  // size_t m_numTasks;
  // std::vector<long> m_costMatrix;
  std::priority_queue<Node> m_open;
  size_t m_numMatching;
};

}  // namespace libMultiRobotPlanning
