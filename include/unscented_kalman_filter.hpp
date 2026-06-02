#ifndef UNSCENTED_KALMAN_FILTER_HPP
#define UNSCENTED_KALMAN_FILTER_HPP
#include <Eigen/dense>
#include <stdexcept>

class UKF{
private:

    int size;
    int guess_points;
    Eigen::VectorXd slope_intercept;
    Eigen::MatrixXd uncertainty;
    Eigen::MatrixXd process_noise;
    double noise;

    double point_spread;
    double recombine_correction;
    double weight_shift;
    double step_size; //spread^2(size+w_shift)-size
    
    Eigen::VectorXd weights_mean; //weighted trust when averaging for new state
    Eigen::VectorXd weights_cov; //weighted trust when calculating new uncertainty

    Eigen::MatrixXd CholeskySquareRoot(const Eigen::MatrixXd mat);

public:
    UKF(int dimensions, double p_noise, double measure_noise, double alpha, double beta, double kappa);

    void UKFUpdate(double price_A, double price_B);

    Eigen::VectorXd getState() const;
};

#endif