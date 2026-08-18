#ifndef RESEARCH_HPP
#define RESEARCH_HPP

#include <string>
#include <Eigen/Dense>
#include <ixwebsocket/IXWebSocket.h>
#include "unscented_kalman_filter.hpp"
#include <ixwebsocket/IXHttpClient.h>
#include <tuple>
class Research {
private:
    int dim;
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
    void runLive();
    void HistoricReplay(std::string csv);
    void monte_carlo_simulation(Eigen::MatrixXd states, double probability);
    UKF ukf;
    void process_measurement(double spy, double qqq, const std::string& spy_time, const std::string& qqq_time);
    std::tuple<double,double, double> create_drift(std::string stock, double time); 
};

#endif
