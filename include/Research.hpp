#ifndef RESEARCH_HPP
#define RESEARCH_HPP

#include <string>
#include <Eigen/Dense>
#include <ixwebsocket/IXWebSocket.h>
#include "unscented_kalman_filter.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <tuple>
#include <deque>
#include <utility>
#include "hidden_markov_model.hpp"
class Research {
private:
    Eigen::MatrixXd noise;
    bool has_stock_A;
    bool has_stock_B;
    std::string timestamp_A;
    std::string timestamp_B;
    std::string csv;
    ix::WebSocket webSocket;
    ix::HttpClient httpClient;
public:
    Research();
    int dim;
    void runLive();
    void HistoricReplay(std::string csv);
    void window_creation();
    void monte_carlo_simulation(Eigen::MatrixXd states, double probability);
    UKF ukf1;
    UKF ukf2;
    HiddenMarkovModel hmm;
    void process_measurement_for_HMM(double latest_price_A, const std::string& timestamp_A, UKF& currukf);
    void process_measurement(double latest_price_A, double latest_price_B, const std::string& timestamp_A, const std::string& timestamp_B);
    std::tuple<double,double, double> create_drift(std::string stock, double time);
};

#endif