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
#include "hidden_markov_model.hpp"

//main function, everything runs
int main(){
    Research research;
    MonteCarlo monteCarlo(10000, 365, 10); // 1000 samples, 365 steps, target price increase of 10
    research.window_creation();
    research.hmm.initializefromRegimes(research.hmm.k_means(3));
    research.runLive();
    return 0;
}