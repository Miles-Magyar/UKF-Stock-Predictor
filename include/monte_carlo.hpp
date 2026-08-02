#ifndef MONTE_CARLO_HPP
#define MONTE_CARLO_HPP
#include <Eigen/dense>
#include <stdexcept>
#include <tuple>
class MonteCarlo{
private:
    int num_samples;
    int steps;
    Eigen::MatrixXd samples;
    double drift;
    double volatility;
    Eigen::VectorXd S;
    double probability;
    double last_price;
    int target_price;
public:
    MonteCarlo(int num_samples, int steps, int target_price);
    Eigen::VectorXd generateRandomWalk(int steps, double drift, double volatility, double last_price);
};

#endif