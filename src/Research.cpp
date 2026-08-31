#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <Eigen/Dense>
#include <cmath>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXHttpClient.h>
#include <nlohmann/json.hpp>
#include "Research.hpp"
#include "structures.hpp"
#include "unscented_kalman_filter.hpp"
#include <utility>
using namespace std;
using json = nlohmann::json;

//loads the API keys from the json file, and if it can't find it, it will exit the program
void loadjson(std::string& apiKey, std::string& secretKey) {
    std::ifstream file("keys.json");
    if (!file.is_open()){
        std::cerr << "Couldn't open keys.json, ensure it exists and is findable" << std::endl;
        exit(1);
    }
    json config;
    file >> config;
    apiKey = config["API_KEY"];
    secretKey = config["SECRET_KEY"];
}

//takes the string of time and converts to seconds
double timeStringToSeconds(const std::string& timeStr){
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss>>std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return static_cast<double>(std::mktime(&tm));
}

//initialization
Research::Research(): dim(2), noise(Eigen::MatrixXd::Identity(dim, dim)*.01), ukf1(dim, 0.001, noise, 2.0, 0.0, 0.0), ukf2(1, 0.001, noise, 2.0, 0.0, 0.0){
       
}

//does ukf update for 2 stocks, useful for pairs trading
void Research::process_measurement(double latest_price_A, double latest_price_B, const std::string& timestamp_A, const std::string& timestamp_B){
    Eigen::VectorXd measurement(2);
    measurement<<latest_price_A, latest_price_B;
    double sec_A = timeStringToSeconds(timestamp_A);
    double sec_B = timeStringToSeconds(timestamp_B);
    if(abs(sec_A-sec_B)<5){
        ukf1.UKFUpdate(measurement);
        std::cout<<"Current Slope: "<<ukf1.slope_intercept(0)<<" | Current Intercept: "<<ukf1.slope_intercept(1)<<std::endl;
    }
}

//does UKF update for 1 stock for HMM
void Research::process_measurement_for_HMM(double latest_price_A, const std::string& timestamp_A, UKF& currukf){
    Eigen::VectorXd measurement(1);
    measurement<<latest_price_A;
    double sec_A = timeStringToSeconds(timestamp_A);
    currukf.UKFUpdate1Stock(measurement);
    std::cout<<"Current Slope: "<<currukf.smoothed_price(0)<<" | Current Intercept: "<<currukf.smoothed_price(1)<<std::endl;
}

//creates the window for the HMM to run on, and updates the HMM with new data as it comes in
void Research::window_creation(){
    double latest_price_A = 0.0;
    has_stock_A = false;
    timestamp_A.clear();
    ix::initNetSystem();
    std::string API_KEY, SECRET_KEY;
    loadjson(API_KEY, SECRET_KEY);
    std::string url = "wss://stream.data.alpaca.markets/v2/iex";
    webSocket.setUrl(url);
    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg){
        //send connection
        if(msg->type == ix::WebSocketMessageType::Open){
            std::cout<<"Connected"<<std::endl;
            json auth = {
                {"action", "auth"},
                {"key", API_KEY},
                {"secret", SECRET_KEY}
            };
            webSocket.send(auth.dump());
        }
        //recieve and send stocks
        else if(msg->type == ix::WebSocketMessageType::Message){
            try{
                json message = json::parse(msg->str);
                for(const auto& event : message){
                    if((event.contains("T")&&(event["T"] == ("success"))) && (event["msg"] == "authenticated")){
                        std::cout << "Auth successful." << std::endl;
                        json subscription = {
                            {"action", "subscribe"},
                            {"trades", {"SPY"}} //specific asset
                        };
                        webSocket.send(subscription.dump());
                    }
                    else if(event.contains("T")&&(event["T"] == ("t"))){
                        std::string symbol = event["S"];
                        double price = event["p"];
                        //symbol needs to be correct, so when adding many variables, increase the # of if statements
                        if (symbol == "SPY") {
                            latest_price_A = price;
                            has_stock_A = true;
                            timestamp_A = event["t"];
                        }
                        if (has_stock_A) {
                            hmm.stock_data.push_front(std::make_pair(latest_price_A, timestamp_A));
                            double sec_A = timeStringToSeconds(hmm.stock_data.back().second);
                            double sec_B = timeStringToSeconds(hmm.stock_data.front().second);
                            while((sec_B-sec_A > 60)&&(hmm.stock_data.size() > 1)){
                                hmm.stock_data.pop_back();
                                sec_A = timeStringToSeconds(hmm.stock_data.back().second);
                            }
                            for(int i = 0;i<hmm.stock_data.size();++i){
                                Research::process_measurement_for_HMM(hmm.stock_data[i].first, hmm.stock_data[i].second, ukf2);
                            }
                            ukf2.reset(1, 0.001, noise, 2.0, 0.0, 0.0);
                            has_stock_A = false;
                        }
                    }
                }
            } catch (const json::parse_error& e){
                std::cout<<"Error parsing JSON: "<<e.what()<<std::endl;
                has_stock_A = false;
                has_stock_B = false;
            }
        }
        else if(msg->type == ix::WebSocketMessageType::Close){
            std::cout<<"Closing connection:"<<std::endl;
            has_stock_A = false;
            has_stock_B = false;
        }
        else if(msg->type == ix::WebSocketMessageType::Error){
            std::cerr<<"WebSocket Error: "<<msg->errorInfo.reason<<std::endl;
            has_stock_A = false;
            has_stock_B = false;
        }
    });

    webSocket.start();

    std::cout << "Press Ctrl+C to exit." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    webSocket.stop();
    ix::uninitNetSystem();
}

//replays csv data for testing
void Research::HistoricReplay(std::string csv){
    bool file_exists = std::filesystem::exists(csv);
    std::ifstream csvfile(csv);
    if(!csvfile.is_open()){
        std::cerr<<"Failed to create or open CSV file"<<std::endl;
    }
    if(!file_exists){
        std::cout<<"No historical data to show."<<std::endl;
    } else{
        std::string line;
        std::getline(csvfile, line);
        std::cout<<line<<std::endl;
        while(std::getline(csvfile, line)){
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> fields;

            while(std::getline(ss, token, ',')){
                fields.push_back(token);
            }
            for(int i = 0;i<fields.size();i++){
                std::cout<<"Field "<<i<<":"<<fields[i]<<std::endl;
            }
            if(fields.size() < 4){
                continue;
            }
            try{
                process_measurement(std::stod(fields[1]), std::stod(fields[3]), fields[0], fields[2]);
                std::cout<<"UKF Slope & Intercept: "<<ukf1.slope_intercept(0)<<" , "<<ukf1.slope_intercept(1);
            } catch(const std::exception& e){
                std::cerr << "Bad CSV row: " << e.what() << '\n';
            }
        }
    }
}

//gets data from websocket and calculates drift for monte carlo
std::tuple<double, double, double> Research::create_drift(std::string stock, double time){ //time does nothing for now, neither does the stock
    ix::initNetSystem();
    std::string API_KEY, SECRET_KEY;
    loadjson(API_KEY, SECRET_KEY);
    std::string url = "https://data.alpaca.markets/v2/stocks/bars?symbols=SPY&timeframe=1Day&start=2025-01-01&end=2026-07-30&feed=iex&adjustment=all&limit=1000&sort=asc";
    auto request = httpClient.createRequest(url);
    request->extraHeaders["APCA-API-KEY-ID"] = API_KEY;
    request->extraHeaders["APCA-API-SECRET-KEY"] = SECRET_KEY;
    auto response = httpClient.get(url, request);
    if(response->statusCode == 200){
        try{
            json data = json::parse(response->body);
            if(data.contains("bars") && data["bars"].contains("SPY")){
                auto bars = data["bars"]["SPY"];
                if(bars.is_array() && !bars.empty()){
                    double last_price = 0;
                    std::vector<double> ri(bars.size());
                    for(size_t i = 1; i < bars.size(); i++){
                        double c_now = bars[i]["c"].get<double>();
                        double c_prev = bars[i - 1]["c"].get<double>();
                        ri[i] = std::log(c_now/c_prev);
                        if(i == bars.size() - 1){
                            last_price = c_now;
                        }
                    }
                    double ri_size = 0;
                    for(int i = 0; i<ri.size(); i++){
                        ri_size += ri[i];
                    }
                    double drift = ri_size/(ri.size()-1);
                    double ri_minus_mean = 0;
                    for(int i = 1; i<ri.size(); i++){
                        ri_minus_mean += (ri[i] - drift) * (ri[i] - drift);
                    }
                    
                    double volatility = std::sqrt(ri_minus_mean/(ri.size()-2));
                    return std::make_tuple(drift, volatility, last_price); // Assuming the third value is unused or a placeholder
                } else{
                    std::cerr<<"No bars data found for SPY"<<" "<<response->statusCode<<" "<<response->description<<std::endl;
                    return std::make_tuple(0.0, 0.0, 0.0);
                }
            } else{
                std::cerr<<"Unexpected JSON structure: "<<response->body<<std::endl;
                return std::make_tuple(0.0, 0.0, 0.0);
            }
        } catch(const json::parse_error& e){
            std::cerr<<"JSON parse error: "<<e.what()<<std::endl;
            return std::make_tuple(0.0, 0.0, 0.0);
        }
    } else{
        std::cerr<<"Error fetching data: "<<response->statusCode<<" "<<response->description<<std::endl;
        return std::make_tuple(0.0, 0.0, 0.0);
    }
}

void Research::monte_carlo_simulation(Eigen::MatrixXd states, double probability){
    csv = "monte_carlo_results.csv";
    std::cout<<"Monte Carlo Simulation Results:"<<std::endl;
    std::cout<<"Probability of reaching target: "<<probability<<std::endl;
    std::cout<<"Sampled States:"<<std::endl;
    std::cout<<states<<std::endl;
    bool file_exists = std::filesystem::exists(csv);
    std::ofstream csvfile(csv, std::ios::app);
    if(!csvfile.is_open()){
        std::cerr<<"Failed to create or open CSV file"<<std::endl;
    }
    if(!file_exists){
        int count = 0;
        for(int j = 0; j<states.cols(); j++){
            csvfile<<"step"+std::to_string(count);
            csvfile<<",";
            count++;
        }
        csvfile<<"Probability\n";
        csvfile.flush();
    }
    if(csvfile.is_open()){
        for(int i = 0; i < states.rows(); i++){
            for(int j = 0; j < states.cols(); j++){
                csvfile << states(i, j);
                if(j < states.cols() - 1){
                    csvfile << ",";
                }
            }
            csvfile << "," << probability << "\n";
        }
        csvfile.flush();
    }
}


//runs live from Alpaca websocket and updates UKF from new data, and saves to csv for later analysis
void Research::runLive(){
    double latest_price_A = 0.0;
    double latest_price_B = 0.0;
    has_stock_A = false;
    has_stock_B = false;
    timestamp_A.clear();
    timestamp_B.clear();
    csv = "ukf_training_data.csv";
    bool file_exists = std::filesystem::exists(csv);
    std::ofstream csvfile(csv, std::ios::app);
    if(!csvfile.is_open()){
        std::cerr<<"Failed to create or open CSV file"<<std::endl;
    }
    if(!file_exists){
        csvfile<<"Timestamp_a,Price_SPY,Timestamp_b,Price_QQQ,Slope,Intercept,Slope_Uncertainty,Intercept_Uncertainty\n";
        csvfile.flush();
    }
    ix::initNetSystem();
    std::string API_KEY, SECRET_KEY;
    loadjson(API_KEY, SECRET_KEY);
    std::string url = "wss://stream.data.alpaca.markets/v2/iex";
    webSocket.setUrl(url);
    webSocket.setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg){
        //send connection
        if(msg->type == ix::WebSocketMessageType::Open){
            std::cout<<"Connected"<<std::endl;
            json auth = {
                {"action", "auth"},
                {"key", API_KEY},
                {"secret", SECRET_KEY}
            };
            webSocket.send(auth.dump());
        } 
        //recieve and send stocks
        else if(msg->type == ix::WebSocketMessageType::Message){
            try{
                json message = json::parse(msg->str);
                for(const auto& event : message){
                    if((event.contains("T")&&(event["T"] == ("success"))) && (event["msg"] == "authenticated")){
                        std::cout << "Auth successful." << std::endl;
                        json subscription = {
                            {"action", "subscribe"},
                            {"trades", {"SPY", "QQQ"}} //specific assets
                        };
                        webSocket.send(subscription.dump());
                    }
                    else if(event.contains("T")&&(event["T"] == ("t"))){
                        std::string symbol = event["S"];
                        double price = event["p"];
                        //symbol needs to be correct, so when adding many variables, increase the # of if statements
                        if (symbol == "SPY") {
                            latest_price_A = price;
                            has_stock_A = true;
                            timestamp_A = event["t"];
                        } else if (symbol == "QQQ") {
                            latest_price_B = price;
                            has_stock_B = true;
                            timestamp_B = event["t"];
                        }
                        if (has_stock_A && has_stock_B) {
                            //update and spit out slope and intercept
                            Research::process_measurement(latest_price_A, latest_price_B, timestamp_A, timestamp_B);
                            if(csvfile.is_open()){
                                csvfile<<timestamp_A<<","<<latest_price_A<<","<<timestamp_B<<","<<latest_price_B<<","<<ukf1.slope_intercept(0)<<","<<ukf1.slope_intercept(1)<<","<<ukf1.uncertainty(0, 0)<<","<<ukf1.uncertainty(1, 1)<<"\n";
                                csvfile.flush();
                            }
                            has_stock_A = false;
                            has_stock_B = false;
                        }
                    }
                }
            } catch (const json::parse_error& e){
                std::cout<<"Error parsing JSON: "<<e.what()<<std::endl;
                has_stock_A = false;
                has_stock_B = false;
            }
        }
        else if(msg->type == ix::WebSocketMessageType::Close){
            std::cout<<"Closing connection:"<<std::endl;
            has_stock_A = false;
            has_stock_B = false;
        }
        else if(msg->type == ix::WebSocketMessageType::Error){
            std::cerr<<"WebSocket Error: "<<msg->errorInfo.reason<<std::endl;
            has_stock_A = false;
            has_stock_B = false;
        }
    });

    webSocket.start();

    std::cout << "Press Ctrl+C to exit." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    webSocket.stop();
    ix::uninitNetSystem();
    if (csvfile.is_open()) {
        csvfile.close();
    }
}