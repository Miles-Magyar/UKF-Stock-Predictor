#ifndef UNSCENTED_KALMAN_FILTER_HPP
#define UNSCENTED_KALMAN_FILTER_HPP
#include <Eigen/dense>
#include <stdexcept>

class UKF{
private:

    int size;
    int guess_points;
    int dim_variable;
    Eigen::MatrixXd process_noise;
    Eigen::MatrixXd noise;

    double point_spread;
    double recombine_correction;
    double weight_shift;
    double step_size; //spread^2(size+w_shift)-size
    
    Eigen::VectorXd weights_mean; //weighted trust when averaging for new state
    Eigen::VectorXd weights_cov; //weighted trust when calculating new uncertainty

    Eigen::MatrixXd CholeskySquareRoot(const Eigen::MatrixXd& mat);

public:
    UKF(int dimensions, double p_noise, Eigen::MatrixXd measure_noise, double alpha, double beta, double kappa);
    Eigen::VectorXd slope_intercept;
    void UKFUpdate(const Eigen::VectorXd& mat);
    Eigen::MatrixXd uncertainty;
    Eigen::VectorXd getState() const;
};

#endif