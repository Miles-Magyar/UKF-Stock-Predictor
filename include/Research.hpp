#ifndef RESEARCH_HPP
#define RESEARCH_HPP

#include <string>
#include <Eigen/Dense>
#include <ixwebsocket/IXWebSocket.h>
#include "unscented_kalman_filter.hpp"

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
public:
    Research();
    void runLive();
    void HistoricReplay(std::string csv);
    UKF ukf;
    void process_measurement(double spy, double qqq, const std::string& spy_time, const std::string& qqq_time);
};

#endif
