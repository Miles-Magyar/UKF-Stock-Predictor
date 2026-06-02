#include "unscented_kalman_filter.hpp"
#include <cmath>
#include <stdexcept>

UKF::UKF(int dimensions, double p_noise, double measure_noise, double alpha, double beta, double kappa){
    size = dimensions;
    guess_points = size*2.0+1;
    noise = measure_noise;
    point_spread = alpha;
    recombine_correction = beta;
    weight_shift = kappa;
    step_size = (point_spread*point_spread)*(size+weight_shift)-size;
    uncertainty = Eigen::MatrixXd::Identity(size, size);
    process_noise = Eigen::MatrixXd::Identity(size, size)*p_noise;
    slope_intercept = Eigen::VectorXd::Zero(size);
    weights_mean = Eigen::VectorXd::Zero(guess_points);
    weights_cov = Eigen::VectorXd::Zero(guess_points);
    weights_mean(0) = step_size/(size+step_size);
    weights_cov(0) = weights_mean(0)+(1.0-point_spread*point_spread+recombine_correction);
    for(int i = 1;i<guess_points;i++){
        weights_mean(i) = 1.0/(2.0*(size+step_size));
        weights_cov(i) = 1.0/(2.0*(size+step_size));
    }
};

Eigen::MatrixXd CholeskySquareRoot(const Eigen::MatrixXd mat){
    Eigen::LLT<Eigen::MatrixXd> lower_lower_transpose_decomp_ofmat(mat);

    if (lower_lower_transpose_decomp_ofmat.info() == Eigen::NumericalIssue) {
        throw std::runtime_error("Cholesky failed: Matrix is not positive/definite!");
    }

    return lower_lower_transpose_decomp_ofmat.matrixL(); //matrixL take the lower triangular portion of the grid or the directional spread the guess (sigma) points need
};

void UKFUpdate(double price_A, double price_B){
    
};