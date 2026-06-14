#ifndef SPSA_H
#define SPSA_H

#include <string>
#include <vector>

struct SPSAParam {
    std::string name;
    int* ptr;
    int def, min, max;
    double step; // perturbation magnitude c
    double lr;   // base learning rate r
};

void initSPSA();
void printSPSAParams();

const std::vector<SPSAParam>& spsaParams();

#endif // SPSA_H