#ifndef Z_SCORE_EXECUTION_HPP
#define Z_SCORE_EXECUTION_HPP
#include <Eigen/dense>
#include <stdexcept>

class Z_Score{
private:
    double price_asset_a;
    double price_asset_b;
    double hedge_ratio;
    double spread;
    double EMA_window;
    double EMA_weight;
    double rolling_spread_mean;
    double rolling_spread_variance;
    double rolling_spread_std;
    double z_score;
    enum class Position { NONE, LONG, SHORT } position;
    double entry_threshold = 2.0;
    double exit_threshold = 0.5;
public:
    Z_Score(double price_a, double price_b, double hedge_r, double ema_window);
    void Z_Score_Update();
};

#endif