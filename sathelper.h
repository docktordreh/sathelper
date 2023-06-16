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

  std::string formulaToStr() {
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
  /*
   * add a clause with at-most-one constraint
   */
  void addAtMostOne(const std::vector<std::string> &clause) {
    std::for_each(clause.begin(), clause.end(),
                  [idx = 0, &clause, this](const auto &lit) mutable {
                    for (int j = idx; j < clause.size(); ++j) {
                      std::vector<std::string> tmp = {getNegated(lit),
                                                      getNegated(clause[j])};
                      this->addClause(tmp);
                    }
                    ++idx;
                  });
  }
  void printFormula() { std::cout << formulaToStr() << std::endl; }
  bool solveSat() {
    const std::string formula = formulaToStr();
    std::ofstream fout(FORMULA_FNAME);
    fout << formula << std::endl;
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
        std::vector<std::string> actual_vars;
        std::istringstream var_line(line.substr(2));
        while (getline(var_line, var, ' ')) {
          int varCode = atoi(var.c_str());
          actual_vars.push_back(intToVar(varCode));
        };
        std::cout << "SATISFIABLE WITH VARS ";
        for (int i = 0; i < actual_vars.size(); ++i) {
          std::cout << actual_vars[i] << " ";
        }
        return true;
      }
      if (line == "s UNSATISFIABLE") {
        return false;
      }
    }
    return false;
  }
  void reset() {
    this->nextIntId = 1;
    this->clauses = {};
    this->intVarMap = {};
    this->varIntMap = {};
  }
};
