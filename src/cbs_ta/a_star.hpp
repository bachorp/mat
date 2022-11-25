#pragma once

#ifdef USE_FIBONACCI_HEAP
#include <boost/heap/fibonacci_heap.hpp>
#endif

#include <boost/heap/d_ary_heap.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "timer.hpp"

namespace libMultiRobotPlanning {

/// Slightly modified A* from libMultiRobotPlanning https://github.com/whoenig/libMultiRobotPlanning

template <typename State, typename Action, typename Cost>
struct PlanResult {
  //! states and their gScore
  std::vector<std::pair<State, Cost> > states;
  //! actions and their cost
  std::vector<std::pair<Action, Cost> > actions;
  //! actual cost of the result
  Cost cost;
  //! lower bound of the cost (for suboptimal solvers)
  Cost fmin;

  // no longer required
  //  bool append(PlanResult<State, Action, Cost>& other) {
  //    if (states.empty() || other.states.empty()
  //        || states.back() != other.states.front()) {
  //        return false;
  //    }
  //    states.insert(states.end(),
  //                  std::make_move_iterator(other.states.begin() + 1),
  //                  std::make_move_iterator(other.states.end()));
  //    other.states.clear();
  //    actions.insert(actions.end(),
  //                   std::make_move_iterator(other.actions.begin()),
  //                   std::make_move_iterator(other.actions.end()));
  //    other.actions.clear();
  //    cost = other.cost;
  //    fmin = other.fmin;
  //    return true;
  //  }
};


template <typename State, typename Action, typename Cost>
struct Neighbor {
  Neighbor(const State& state, const Action& action, Cost cost)
          : state(state), action(action), cost(cost) {}

  //! neighboring state
  State state;
  //! action to get to the neighboring state
  Action action;
  //! cost to get to the neighboring state
  Cost cost;
};


template <typename State, typename Action, typename Cost, typename Environment,
          typename StateHasher = std::hash<State> >
class AStar {
 public:
  AStar(Environment& environment) : m_env(environment) {}

  bool search(const State& startState,
              PlanResult<State, Action, Cost>& solution, Cost initialCost = 0, double timeout = 0) {
    Timer timer;
    solution.states.clear();
    solution.states.push_back(std::make_pair<>(startState, 0));
    solution.actions.clear();
    solution.cost = 0;

    openSet_t openSet;
    std::unordered_map<State, fibHeapHandle_t, StateHasher> stateToHeap;
    std::unordered_set<State, StateHasher> closedSet;
    std::unordered_map<State, std::tuple<State, Action, Cost, Cost>,
                       StateHasher>
        cameFrom;

    auto handle = openSet.push(
        Node(startState, m_env.admissibleHeuristic(startState), initialCost));
    stateToHeap.insert(std::make_pair<>(startState, handle));
    (*handle).handle = handle;

    std::vector<Neighbor<State, Action, Cost> > neighbors;
    neighbors.reserve(10);

    while (!openSet.empty()) {
      if (timeout != 0 && timer.elapsedSeconds() > timeout) {
        throw std::runtime_error("Timeout");
      }
      Node current = openSet.top();
      m_env.onExpandNode(current.state, current.fScore, current.gScore);

      if (m_env.isSolution(current.state)) {
        solution.states.clear();
        solution.actions.clear();
        auto iter = cameFrom.find(current.state);
        while (iter != cameFrom.end()) {
          solution.states.push_back(
              std::make_pair<>(iter->first, std::get<3>(iter->second)));
          solution.actions.push_back(std::make_pair<>(
              std::get<1>(iter->second), std::get<2>(iter->second)));
          iter = cameFrom.find(std::get<0>(iter->second));
        }
        solution.states.push_back(std::make_pair<>(startState, initialCost));
        std::reverse(solution.states.begin(), solution.states.end());
        std::reverse(solution.actions.begin(), solution.actions.end());
        solution.cost = current.gScore;
        solution.fmin = current.fScore;

        return true;
      }

      openSet.pop();
      stateToHeap.erase(current.state);
      closedSet.insert(current.state);

      // traverse neighbors
      neighbors.clear();
      m_env.getNeighbors(current.state, neighbors);
      for (const Neighbor<State, Action, Cost>& neighbor : neighbors) {
        if (closedSet.find(neighbor.state) == closedSet.end()) {
          if(m_env.admissibleHeuristic(neighbor.state) < 0) {
            continue;
          }
          Cost tentative_gScore = current.gScore + neighbor.cost;
          auto iter = stateToHeap.find(neighbor.state);
          if (iter == stateToHeap.end()) {  // Discover a new node
            Cost fScore =
                tentative_gScore + m_env.admissibleHeuristic(neighbor.state);
            if(fScore < 0) {
              continue;
            }
            auto handle =
                openSet.push(Node(neighbor.state, fScore, tentative_gScore));
            (*handle).handle = handle;
            stateToHeap.insert(std::make_pair<>(neighbor.state, handle));
            m_env.onDiscover(neighbor.state, fScore, tentative_gScore);
            // std::cout << "  this is a new node " << fScore << "," <<
            // tentative_gScore << std::endl;
          } else {
            auto handle = iter->second;
            // std::cout << "  this is an old node: " << tentative_gScore << ","
            // << (*handle).gScore << std::endl;
            // We found this node before with a better path
            if (tentative_gScore >= (*handle).gScore) {
              continue;
            }

            // update f and gScore
            Cost delta = (*handle).gScore - tentative_gScore;
            (*handle).gScore = tentative_gScore;
            (*handle).fScore -= delta;
            openSet.increase(handle);
            m_env.onDiscover(neighbor.state, (*handle).fScore,
                             (*handle).gScore);
          }

          // Best path for this node so far
          // TODO: this is not the best way to update "cameFrom", but otherwise
          // default c'tors of State and Action are required
          cameFrom.erase(neighbor.state);
          cameFrom.insert(std::make_pair<>(
              neighbor.state,
              std::make_tuple<>(current.state, neighbor.action, neighbor.cost,
                                tentative_gScore)));
        }
      }
    }

    return false;
  }

 private:
  struct Node {
    Node(const State& state, Cost fScore, Cost gScore)
        : state(state), fScore(fScore), gScore(gScore) {}

    bool operator<(const Node& other) const {
      // Sort order
      // 1. lowest fScore
      // 2. highest gScore

      // Our heap is a maximum heap, so we invert the comperator function here
      if (fScore != other.fScore) {
        return fScore > other.fScore;
      } else {
        return gScore < other.gScore;
      }
    }

    friend std::ostream& operator<<(std::ostream& os, const Node& node) {
      os << "state: " << node.state << " fScore: " << node.fScore
         << " gScore: " << node.gScore;
      return os;
    }

    State state;

    Cost fScore;
    Cost gScore;

#ifdef USE_FIBONACCI_HEAP
    typename boost::heap::fibonacci_heap<Node>::handle_type handle;
#else
    typename boost::heap::d_ary_heap<Node, boost::heap::arity<2>,
                                     boost::heap::mutable_<true> >::handle_type
        handle;
#endif
  };

#ifdef USE_FIBONACCI_HEAP
  typedef typename boost::heap::fibonacci_heap<Node> openSet_t;
  typedef typename openSet_t::handle_type fibHeapHandle_t;
#else
  typedef typename boost::heap::d_ary_heap<Node, boost::heap::arity<2>,
                                           boost::heap::mutable_<true> >
      openSet_t;
  typedef typename openSet_t::handle_type fibHeapHandle_t;
#endif

 private:
  Environment& m_env;
};

}  // namespace libMultiRobotPlanning
