#ifndef SATHELPER_SATHELPER_H
#define SATHELPER_SATHELPER_H
#endif // SATHELPER_SATHELPER_H
#include <unordered_map>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

class SatHelper {
private:
  int nextIntId = 1;
  std::vector<std::string> model;
  std::unordered_map<std::string, int> varIntMap;
  std::unordered_map<int, std::string> intVarMap;
  std::vector<std::vector<int>> clauses;
  int varToInt(const std::string &name) { return varIntMap[name]; };
  std::string intToVar(const int var) {
    if (var == 0)
      return "0";
    if (var > 0) {
      return intVarMap[var];
    }
    return "-" + intVarMap[-var];
  }
  static std::string getVarName(const std::string &lit) {
    if (isNegated(lit)) {
      return lit.substr(1);
    }
    return lit;
  }
  static bool isNegated(const std::string &lit) { return lit[0] == '-'; }
  int literalToInt(const std::string &lit) {
    int code = varToInt(getVarName(lit));
    if (isNegated(lit)) {
      return -code;
    }
    return code;
  }
  std::vector<int> literalsToIntStr(const std::vector<std::string> &clause) {
    std::vector<int> res(clause.size() + 1, 0);
    std::for_each(clause.begin(), clause.end(),
                  [idx = 0, &res, this](const auto &elem) mutable {
                    res[idx] = literalToInt(elem);
                    ++idx;
                  });
    return res;
  }
  static std::string getNegated(const std::string &lit) {
    if (isNegated(lit)) {
      return getVarName(lit);
    }
    return "-" + lit;
  }

  static std::string clauseToStr(const std::vector<int> &clause) {
    std::string res;
    for (const auto &lit : clause) {
      res += std::to_string(lit);
      if (lit == 0) {
        continue;
      }
      res += " ";
    }
    return res;
  }

  std::string formulaToStr() const {
    std::string res = "p cnf " + std::to_string(nextIntId - 1) + " " +
                      std::to_string((int)clauses.size()) + "\n";
    std::for_each(clauses.begin(), clauses.end(), [&res](const auto &clause) {
      res += clauseToStr(clause) + "\n";
    });
    return res;
  }
  const std::string FORMULA_FNAME = "/tmp/formula.cnf";

public:
  /*
   * you need to declare vars first before use to create a mapping from name to
   * int (dimacs doesn't use names)
   */
  void declareVar(const std::string &name) {
    if (varIntMap.contains(name)) {
      return;
    }
    this->varIntMap[name] = this->nextIntId;
    this->intVarMap[this->nextIntId] = name;
    this->nextIntId += 1;
  }
  /*
   * add a cnf clause
   */
  void addClause(const std::vector<std::string> &clause) {
    this->clauses.push_back(literalsToIntStr(clause));
  }
    void addExactlyOne(const std::vector<std::string> &clause) {
        std::ranges::for_each(
                clause.begin(), clause.end(), [this, &clause](const auto &lit) {
                    std::vector<std::string> cur_clause = {lit};
                    std::ranges::for_each(clause.begin(), clause.end(),
                                          [&lit, &cur_clause](const auto &other_lit) {
                                              if (other_lit == lit)
                                                  return;
                                              cur_clause.push_back(getNegated(other_lit));
                                          });
                    this->addClause(cur_clause);
                });
    }
  /*
   * add a clause with at-most-one constraint
   */
  void addAtMostOne(const std::vector<std::string> &clause) {
    for (int i = 0; i < clause.size(); ++i) {
      for (int j = i + 1; j < clause.size(); ++j) {
        std::vector<std::string> tmp = {getNegated(clause[i]),
                                        getNegated(clause[j])};
        this->addClause(tmp);
      }
    }
  }

  void printFormula() const { std::cout << formulaToStr() << std::endl; }

  bool solveSat(const bool quiet = true) {
    const std::string formula = formulaToStr();
    std::ofstream ofstream(FORMULA_FNAME);
    ofstream << formula << std::endl;
    FILE *fp = popen(("glucose -model " + FORMULA_FNAME).c_str(), "r");
    std::ostringstream out;
    while (!feof(fp) && !ferror(fp)) {
      char buf[128];
      int bytesRead = fread(buf, 1, 128, fp);
      out.write(buf, bytesRead);
    }
    // this can probably be written more elegant
    std::istringstream stream;
    stream.str(out.str());
    std::string line;
    while (getline(stream, line)) {
      if (line == "s SATISFIABLE") {
        getline(stream, line);
        std::string var;
        std::istringstream var_line(line.substr(2));
        while (getline(var_line, var, ' ')) {
          int varCode = atoi(var.c_str());
          this->model.push_back(intToVar(varCode));
        }
        if (quiet) {
          return true;
        }
        std::cout << "SATISFIABLE WITH VARS ";
        for (const auto &actual_var : model) {
          std::cout << actual_var << " ";
        }
        return true;
      }
      if (line == "s UNSATISFIABLE") {
        return false;
      }
    }
    return false;
  }

  std::vector<std::string> get_model() const { return this->model; }

  void reset() {
    this->nextIntId = 1;
    this->clauses = {};
    this->intVarMap = {};
    this->varIntMap = {};
    this->model = {};
  }
};
