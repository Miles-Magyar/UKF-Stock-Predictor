#ifndef HIDDEN_MARKOV_MODEL_HPP
#define HIDDEN_MARKOV_MODEL_HPP
#include <Eigen/Dense>
#include <utility>
#include <deque>
#include <string>
#include <cmath>
#include <vector>
#include <numbers>
struct RegimeParameters {
    double mean_return;
    double mean_volatility;
    double variance_return;
    double variance_volatility;
};
class HiddenMarkovModel {
public:
    HiddenMarkovModel();
    std::pair<Eigen::MatrixXd, double> forward_algorithm(Eigen::MatrixXd& observations);
    double baum_welch_algorithm(Eigen::MatrixXd& observations);
    std::deque<std::pair<double, std::string>> stock_data; 
    std::vector<RegimeParameters> k_means(int states);
    std::pair<Eigen::VectorXd, double> viterbi_algorithm(Eigen::MatrixXd& observations); //returns statepath and probability of the most likely state sequence
private:
    Eigen::MatrixXd transition_matrix;
    Eigen::MatrixXd backward_algorithm(Eigen::MatrixXd& observations);
    Eigen::VectorXd initial_state_vector;
    Eigen::MatrixXd emission_matrix;
    std::vector<RegimeParameters> regimes;
    int num_states;
    Eigen::VectorXd observations;
    //continuous emissions
    Eigen::VectorXd emission_means;
    Eigen::VectorXd emission_variances;
    void initializefromRegimes(const std::vector<RegimeParameters>& regimes);
    std::vector<double> computeReturns() const;
    double gaussian_pdf(double x, double mean, double variance) const;
    double emissionProbability(double return_value, int state, double vol) const;
    Eigen::MatrixXd buildFeatures(int window = 3) const;
    Eigen::MatrixXd normalizeFeatures(const Eigen::MatrixXd& features);
    Eigen::MatrixXd initCentroids(const Eigen::MatrixXd& features, int k);
    std::vector<int> assignClusters(const Eigen::MatrixXd& features, const Eigen::MatrixXd& centroids);
    Eigen::MatrixXd updateCentroids(const Eigen::MatrixXd& features, const std::vector<int>& assignments, int k);
};

#endif