#ifndef HIDDEN_MARKOV_MODEL_HPP
#define HIDDEN_MARKOV_MODEL_HPP
#include <Eigen/dense>
#include <utility>
class HiddenMarkovModel {
public:
    HiddenMarkovModel();
    double forward_algorithm(Eigen::VectorXd& observations);
    double baum_welch_algorithm(Eigen::VectorXd& observations);
    std::pair<Eigen::VectorXi, double> viterbi_algorithm(Eigen::VectorXd& observations); //returns statepath and probability of the most likely state sequence
private:
    Eigen::MatrixXd transition_matrix;
    Eigen::VectorXd initial_state_vector;
    Eigen::MatrixXd emission_matrix;
    int num_states;
    Eigen::VectorXd observations;
    Eigen::VectorXd emission_means;
    Eigen::VectorXd emission_variances;

};

#endif