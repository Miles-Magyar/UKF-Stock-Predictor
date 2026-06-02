#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP

#include <string>
#include <vector>
#include <unordered_map>

namespace webdata{
    struct stockdata{
        std::string timestamp;

    };

    struct asset{
        std::string symbol;
        double bid_price;
        double ask_price;
        double volume;
    };

    struct commissiondata{
        std::string timestamp;
        std::unordered_map<std::string, webdata::asset> ticker_string;
    };

    struct position{
        double average_price;
        double qty;
    };

    struct portfoliodata{
        double money;
        std::unordered_map<std::string, webdata::position> positions;
        double total_equity;
    };
};
#endif