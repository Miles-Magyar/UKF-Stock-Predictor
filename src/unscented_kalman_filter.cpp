#include "unscented_kalman_filter.hpp"
#include <Eigen/dense>
#include <cmath>
#include <stdexcept>

UKF::UKF(int dimensions, double p_noise, Eigen::MatrixXd measure_noise, double alpha, double beta, double kappa){
    size = dimensions;
    guess_points = size*2+1.0;
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

Eigen::MatrixXd UKF::CholeskySquareRoot(const Eigen::MatrixXd& mat){
    Eigen::LLT<Eigen::MatrixXd> lower_lower_transpose_decomp_ofmat(mat);

    if (lower_lower_transpose_decomp_ofmat.info() == Eigen::NumericalIssue) {
        throw std::runtime_error("Cholesky failed: Matrix is not positive/definite!");
    }

    return lower_lower_transpose_decomp_ofmat.matrixL(); //matrixL take the lower triangular portion of the grid or the directional spread the guess (sigma) points need
};

void UKF::UKFUpdate(const Eigen::VectorXd& mat){
    dim_variable = mat.rows();
    Eigen::MatrixXd spread = CholeskySquareRoot((step_size+size)*uncertainty);
    Eigen::MatrixXd sigma_points = Eigen::MatrixXd::Zero(size, guess_points);
    sigma_points.col(0) = slope_intercept;
    for(int i = 0;i<size;i++){
        sigma_points.col(i+1) =  slope_intercept+spread.col(i);
    }
    for(int i = 0; i<size;i++){
        sigma_points.col(i+1+size) = slope_intercept-spread.col(i);
    }
    Eigen::MatrixXd predicted_sigma_points = sigma_points; // needs system model to update
    Eigen::MatrixXd predicted_covariance = Eigen::MatrixXd::Zero(size, size);
    Eigen::VectorXd predicted_state = Eigen::VectorXd::Zero(size);
    for(int i = 0;i<guess_points;i++){
        predicted_state += weights_mean(i)*predicted_sigma_points.col(i);
    }
    Eigen::VectorXd temp = Eigen::VectorXd::Zero(size);
    for(int i = 0;i<guess_points;i++){
        temp = predicted_sigma_points.col(i)-predicted_state;
        predicted_covariance += weights_cov(i)*(temp*temp.transpose());
    }
    predicted_covariance += process_noise;
    Eigen::MatrixXd measurement_points = Eigen::MatrixXd::Zero(dim_variable, guess_points);
    Eigen::VectorXd predicted_measurement = Eigen::VectorXd::Zero(dim_variable);
    double actual_price_A = mat(0);
    double actual_price_B = mat(1);
    for(int i = 0; i < guess_points; i++) {
        double predicted_slope = predicted_sigma_points(0, i);
        double predicted_intercept = predicted_sigma_points(1, i);
        double price_A = (predicted_slope * actual_price_B) + predicted_intercept;
        double price_B = 0.0;
        if(predicted_slope != 0.0){
            price_B = (actual_price_A - predicted_intercept)/predicted_slope;
        } else{
            price_B = actual_price_B;
        }
        measurement_points(0, i) = price_A;
        measurement_points(1, i) = price_B;
    }
    Eigen::MatrixXd innovation_covariance = Eigen::MatrixXd::Zero(dim_variable, dim_variable); //uncertainty in real prices
    Eigen::VectorXd price_error = Eigen::VectorXd::Zero(dim_variable);
    Eigen::MatrixXd cross_covariance = Eigen::MatrixXd::Zero(size, dim_variable); //connection between guess and real price
    for(int i = 0;i<guess_points;i++){
        predicted_measurement += measurement_points.col(i)*weights_mean(i);
    }
    for(int i = 0;i<guess_points;i++){
        price_error = measurement_points.col(i)-predicted_measurement;
        innovation_covariance += weights_cov(i)*(price_error*price_error.transpose());
    }
    innovation_covariance+=noise;
    Eigen::VectorXd slope_var = Eigen::VectorXd::Zero(size);
    Eigen::VectorXd measure_var = Eigen::VectorXd::Zero(size);
    for(int i = 0;i<guess_points;i++){
        measure_var = measurement_points.col(i)-predicted_measurement;
        slope_var = predicted_sigma_points.col(i)-predicted_state;
        cross_covariance += slope_var*weights_cov(i)*measure_var.transpose();
    }
    Eigen::MatrixXd kalman_gain = Eigen::MatrixXd::Zero(size, dim_variable);
    kalman_gain = cross_covariance*innovation_covariance.inverse();
    slope_intercept = slope_intercept+kalman_gain*(mat-predicted_measurement);
    uncertainty = predicted_covariance-kalman_gain*innovation_covariance*kalman_gain.transpose();
};