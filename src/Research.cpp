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
using namespace std;
using json = nlohmann::json;
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
double timeStringToSeconds(const std::string& timeStr){
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss>>std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return static_cast<double>(std::mktime(&tm));
}
class Research{
private:
    bool has_stock_A;
    bool has_stock_B;
    std::string timestamp_A;
    std::string timestamp_B;
    std::string csv;
    Eigen::MatrixXd noise;
    int dim;
    ix::WebSocket webSocket;
public:
    void runLive();
    void HistoricReplay(std::string csv);
    Research();
    UKF ukf;
    void process_measurement(double spy, double qqq, const std::string& spy_time, const std::string& qqq_time);
};
Research::Research(): dim(2), noise(Eigen::MatrixXd::Identity(dim, dim)*.01), ukf(dim, 0.001, noise, 2.0, 0.0, 0.0){
        
}

void Research::process_measurement(double latest_price_A, double latest_price_B, const std::string& timestamp_A, const std::string& timestamp_B){
    Eigen::VectorXd measurement(2);
    measurement<<latest_price_A, latest_price_B;
    double sec_A = timeStringToSeconds(timestamp_A);
    double sec_B = timeStringToSeconds(timestamp_B);
    if(abs(sec_A-sec_B)<5){
        ukf.UKFUpdate(measurement);
        std::cout<<"Current Slope: "<<ukf.slope_intercept(0)<<" | Current Intercept: "<<ukf.slope_intercept(1)<<std::endl;
    }
}

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
            if(fields.size() < 8){
                continue;
            } else{
                break;
            }
            try{
                process_measurement(std::stod(fields[1]), std::stod(fields[3]), fields[0], fields[2]);
                std::cout<<"UKF Slope & Intercept: "<<ukf.slope_intercept(0)<<" , "<<ukf.slope_intercept(1);
            } catch(const std::exception& e){
                std::cerr << "Bad CSV row: " << e.what() << '\n';
            }
        }
    }
}

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
                                csvfile<<timestamp_A<<","<<latest_price_A<<","<<timestamp_B<<","<<latest_price_B<<","<<ukf.slope_intercept(0)<<","<<ukf.slope_intercept(1)<<","<<ukf.uncertainty(0, 0)<<","<<ukf.uncertainty(1, 1)<<"\n";
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