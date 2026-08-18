#include "monte_carlo.hpp"
#include <Eigen/dense>
#include <cmath>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include "Research.hpp"
#include <random>
using json = nlohmann::json;

MonteCarlo::MonteCarlo(int num_samples, int steps, double target_price) : num_samples(num_samples), steps(steps), target_price(target_price), samples(Eigen::MatrixXd::Zero(num_samples, steps)) {
    Research research;
    std::tuple<double,double,double> var_creation = research.create_drift("SPY", 1.0);
    this->drift = std::get<0>(var_creation);
    this->volatility = std::get<1>(var_creation);
    this->last_price = std::get<2>(var_creation);
    double target = last_price + target_price;
    this->probability = 0.0;
    for(int i = 0; i<num_samples; i++){
        S = generateRandomWalk(steps, drift, volatility, last_price);
        samples.row(i) = S;
        if(S(steps-1) >= target){
            this->probability += 1.0;
        }
    }
    this->probability = probability/num_samples;


}

Eigen::VectorXd MonteCarlo::generateRandomWalk(int steps, double drift, double volatility, double last_price) {
    Eigen::VectorXd walk(steps); //steps = in days btw
    walk(0) = last_price; // Start from the last known price
    std::mt19937 generator{std::random_device{}()};
    std::normal_distribution<double> distribution(0.0, 1.0);
    for (int i = 1; i < steps; ++i) {
        double randomShock = distribution(generator); // Generates a normal distribution of a num between 0 and 1
        walk(i) = walk(i - 1) * std::exp((drift-0.5*volatility*volatility) + volatility * randomShock);
    }
    return walk;
}