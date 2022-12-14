#pragma once

#include <map>

#include "a_star.hpp"
#include "timer.hpp"

namespace libMultiRobotPlanning {

/// Implentation of CBS-MAPD based on CBS-TA from libMultiRobotPlanning https://github.com/whoenig/libMultiRobotPlanning

template <typename State, typename Action, typename Cost, typename Conflict,
          typename Constraints, typename Task, typename Environment>
class CBSTA {
 public:
  CBSTA(Environment& environment) : m_env(environment) {}

  bool search(const std::vector<State>& initialStates,
              std::vector<PlanResult<State, Action, Cost> >& solution,
              std::map<size_t, Task>& taskAssignment,
              double timeout = 0) {
    Timer timer;
    HighLevelNode start;
    size_t numAgents = initialStates.size();
    start.solution.resize(numAgents);
    start.constraints.resize(numAgents);
    start.cost = 0;
    start.id = 0;
    start.isRoot = true;
    m_env.nextTaskAssignment(start.tasks);

    for (size_t i = 0; i < initialStates.size(); ++i) {
      // if (   i < solution.size()
      //     && solution[i].states.size() > 1) {
      //   start.solution[i] = solution[i];
      //   std::cout << "use existing solution for agent: " << i << std::endl;
      // } else {
      bool success = false;
      if (!start.tasks.empty()) {
        success = findDeliveryPlan(i, initialStates.at(i), start, timer, timeout);
      }
      if (!success) {
        return false;
      }

      // }
      start.cost = std::max(start.solution[i].cost, start.cost);
    }

    // std::priority_queue<HighLevelNode> open;
    typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                     boost::heap::mutable_<true> >
        open;

    auto handle = open.push(start);
    (*handle).handle = handle;

    solution.clear();
    int id = 1;
    while (!open.empty()) {
      if (timeout != 0 && timer.elapsedSeconds() > timeout) {
          throw std::runtime_error("Timeout");
      }
      HighLevelNode P = open.top();
      m_env.onExpandHighLevelNode(P.cost);
      // std::cout << "expand: " << P << std::endl;

      open.pop();

      Conflict conflict;
      if (!m_env.getFirstConflict(P.solution, conflict)) {
        std::cout << "done; cost: " << P.cost << std::endl;
        solution = P.solution;
        taskAssignment = P.tasks;
        return true;
      }

      if (P.isRoot) {
        // std::cout << "root node expanded; add new root" << std::endl;
        HighLevelNode n;
        m_env.nextTaskAssignment(n.tasks);

        if (n.tasks.size() > 0) {
          n.solution.resize(numAgents);
          n.constraints.resize(numAgents);
          n.cost = 0;
          n.id = id;
          n.isRoot = true;

          bool allSuccessful = true;
          for (size_t i = 0; i < numAgents; ++i) {
            bool success = findDeliveryPlan(i, initialStates.at(i), n, timer, timeout);
            if (!success) {
              allSuccessful = false;
              break;
            }
            n.cost = std::max(n.solution[i].cost, n.cost);
          }
          if (allSuccessful) {
            auto handle = open.push(n);
            (*handle).handle = handle;
            ++id;
            // std::cout << " new root added! cost: " << n.cost << std::endl;
          }
        }
      }

      // create additional nodes to resolve conflict
      // std::cout << "Found conflict: " << conflict << std::endl;
      // std::cout << "Found conflict at t=" << conflict.time << " type: " <<
      // conflict.type << std::endl;

      std::map<size_t, Constraints> constraints;
      m_env.createConstraintsFromConflict(conflict, constraints);
      for (const auto& c : constraints) {
        // std::cout << "Add HL node for " << c.first << std::endl;
        size_t i = c.first;
        // std::cout << "create child with id " << id << std::endl;
        HighLevelNode newNode = P;
        newNode.id = id;
        // (optional) check that this constraint was not included already
        // std::cout << newNode.constraints[i] << std::endl;
        // std::cout << c.second << std::endl;
        assert(!newNode.constraints[i].overlap(c.second));

        newNode.constraints[i].add(c.second);

        bool success = findDeliveryPlan(i, initialStates.at(i), newNode, timer, timeout);

        newNode.cost = std::max(newNode.solution[i].cost, newNode.cost);

        if (success) {
          // std::cout << "  success. cost: " << newNode.cost << std::endl;
          auto handle = open.push(newNode);
          (*handle).handle = handle;
        }

        ++id;
      }
    }
    return false;
  }

 private:
  struct HighLevelNode {
    std::vector<PlanResult<State, Action, Cost> > solution;
    std::vector<Constraints> constraints;
    std::map<size_t, Task> tasks; // maps from index to task (and does not contain an entry if no task was assigned)

    Cost cost;

    int id;
    bool isRoot;

    typename boost::heap::d_ary_heap<HighLevelNode, boost::heap::arity<2>,
                                     boost::heap::mutable_<true> >::handle_type
        handle;

    bool operator<(const HighLevelNode& n) const {
      // if (cost != n.cost)
      return cost > n.cost;
      // return id > n.id;
    }

    Task* task(size_t idx)
    {
      Task* task = nullptr;
      auto iter = tasks.find(idx);
      if (iter != tasks.end()) {
        task = &iter->second;
      }
      return task;
    }

    friend std::ostream& operator<<(std::ostream& os, const HighLevelNode& c) {
      os << "id: " << c.id << " cost: " << c.cost << std::endl;
      for (size_t i = 0; i < c.solution.size(); ++i) {
        os << "Agent: " << i << std::endl;
        os << " States:" << std::endl;
        for (size_t t = 0; t < c.solution[i].states.size(); ++t) {
          os << "  " << c.solution[i].states[t].first << std::endl;
        }
        os << " Constraints:" << std::endl;
        os << c.constraints[i];
        os << " cost: " << c.solution[i].cost << std::endl;
      }
      return os;
    }
  };

  struct LowLevelEnvironment {
    LowLevelEnvironment(Environment& env, size_t agentIdx,
                        const Constraints& constraints, const Task* task)
        : m_env(env)
    // , m_agentIdx(agentIdx)
    // , m_constraints(constraints)
    {
      m_env.setLowLevelContext(agentIdx, &constraints, task);
    }

    Cost admissibleHeuristic(const State& s) {
      return m_env.admissibleHeuristic(s);
    }

    bool isSolution(const State& s) { return m_env.isSolution(s); }

    void getNeighbors(const State& s,
                      std::vector<Neighbor<State, Action, Cost> >& neighbors) {
      m_env.getNeighbors(s, neighbors);
    }

    void onExpandNode(const State& s, Cost fScore, Cost gScore) {
      // std::cout << "LL expand: " << s << std::endl;
      m_env.onExpandLowLevelNode(s, fScore, gScore);
    }

    void onDiscover(const State& /*s*/, Cost /*fScore*/, Cost /*gScore*/) {
      // std::cout << "LL discover: " << s << std::endl;
      // m_env.onDiscoverLowLevel(s, m_agentIdx, m_constraints);
    }

   private:
    Environment& m_env;
    // size_t m_agentIdx;
    // const Constraints& m_constraints;
  };

  bool findDeliveryPlan(int agent, const State& start, HighLevelNode& n, const Timer& timer, double timeout) {
    LowLevelEnvironment llenv(m_env, agent, n.constraints.at(agent),
                              n.task(agent));
    LowLevelSearch_t lowLevel(llenv);
    double subTimeout = (timeout == 0 ? 0 : timeout - timer.elapsedSeconds());
    //    bool success = lowLevel.search(start, n.solution[agent], 0, subTimeout);
    //    if (success) {
    //      LowLevelEnvironment dllenv(m_env, agent, n.constraints.at(agent),
    //                                n.task(agent), true);
    //      LowLevelSearch_t dlowLevel(dllenv);
    //      PlanResult<State, Action, Cost> deliveryPlan;
    //      success = dlowLevel.search(n.solution[agent].states.back().first,
    //                                deliveryPlan, n.solution[agent].cost, subTimeout);
    //      if (success) {
    //        success = n.solution[agent].append(deliveryPlan);
    //        assert(success);
    //        LowLevelEnvironment allenv(m_env, agent, n.constraints.at(agent),
    //                                   nullptr, true);
    //        LowLevelSearch_t alowLevel(allenv);
    //        PlanResult<State, Action, Cost> afterPlan;
    //        success = dlowLevel.search(n.solution[agent].states.back().first,
    //                                   afterPlan, n.solution[agent].cost, subTimeout);
    //        if (success) {
    //          success = n.solution[agent].append(afterPlan);
    //          assert(success);
    //        }
    //      }
    //    }
    return lowLevel.search(start, n.solution[agent], 0, subTimeout);
  }

 private:
  Environment& m_env;
  typedef AStar<State, Action, Cost, LowLevelEnvironment> LowLevelSearch_t;
};

}  // namespace libMultiRobotPlanning
