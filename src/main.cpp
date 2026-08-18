#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include "unscented_kalman_filter.hpp"
#include "structures.hpp"
#include "Research.hpp"
#include "monte_carlo.hpp"
int main(){
    Research research;
    MonteCarlo monteCarlo(10000, 365, 10); // 1000 samples, 365 steps, target price increase of 10
    research.runLive();
    return 0;
}