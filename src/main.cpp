#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include "unscented_kalman_filter.hpp"
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
int main(){
    int dim = 2;
    Eigen::MatrixXd noise = Eigen::MatrixXd::Identity(dim, dim)*.01;
    UKF ukf(dim, 0.001, noise, 2.0, 0.0, 0.0);
    double latest_price_A = 0.0;
    double latest_price_B = 0.0;
    bool has_stock_A = false;
    bool has_stock_B = false;
    ix::initNetSystem();
    ix::WebSocket webSocket;
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
                        } else if (symbol == "QQQ") {
                            latest_price_B = price;
                            has_stock_B = true;
                        }
                        if (has_stock_A && has_stock_B) {
                            Eigen::VectorXd mat(2);
                            mat << latest_price_A, latest_price_B;
                            //update and spit out slope and intercept
                            ukf.UKFUpdate(mat);
                            std::cout<<"Current Slope: "<<ukf.slope_intercept(0)<<" | Current Intercept: "<<ukf.slope_intercept(1)<<std::endl;
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
    return 0;
}