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
#include "Research.cpp"
int main(){
    Research research;
    research.runLive();
    
    return 0;
}