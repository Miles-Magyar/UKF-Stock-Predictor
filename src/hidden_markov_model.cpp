#include "hidden_markov_model.hpp"

HiddenMarkovModel::HiddenMarkovModel(){
    num_states = 3;

}

double HiddenMarkovModel::baum_welch_algorithm(Eigen::VectorXd& observations){
    
}

double HiddenMarkovModel::forward_algorithm(Eigen::VectorXd& observations){

}

std::pair<Eigen::VectorXd, double> HiddenMarkovModel::viterbi_algorithm(Eigen::VectorXd& observations){

}

//puts the deque stocks in front and computes log returns since its large
std::vector<double> HiddenMarkovModel::computeReturns() const{
    std::vector<double> returns;
    if(stock_data.size()<num_states){
        return returns;
    }
    for (int i = stock_data.size()-1; i>0; --i){
        double p_now  = stock_data[i-1].first;
        double p_prev = stock_data[i].first;
        if (p_prev>0.0 && p_now>0.0){
            returns.push_back(std::log(p_now/p_prev));
        }
    }
    return returns;
}

//builds a feature matrix with log returns
Eigen::MatrixXd HiddenMarkovModel::buildFeatures(int window) const{
    std::vector<double> logReturns = computeReturns();
    int n = static_cast<int>(logReturns.size());
    if(n == 0){
        return Eigen::MatrixXd(0, 2);
    }
    Eigen::MatrixXd features(n, 2);

    for(int i = 0; i<n;++i){
        features(i, 0) = logReturns[i];
        int start = std::max(0, i - window + 1);
        int count = i-start+1;
        double mean = 0.0;
        for(int j = start; j<= i;++j){
            mean+= logReturns[j];
        }
        mean= mean/count;
        double sum_sq = 0.0;
        for(int j = start;j<i;++j){
            double diff = logReturns[j]-mean;
            sum_sq += diff*diff;
        }
        if(count>1){
            features(i, 1) = std::sqrt(sum_sq/(count-1));
        } else{
            features(i, 1) = 0.0;
        }
    }
    return features;
}

//normalizes the features to have mean 0 and variance 1
Eigen::MatrixXd HiddenMarkovModel::normalizeFeatures(const Eigen::MatrixXd& features){
    if(features.rows() == 0){
        return features;
    }
    Eigen::MatrixXd normalized = features;
    for(int i = 0; i<features.cols();++i){
        double mean = normalized.col(i).mean();
        double sum_sq = 0.0;
        for(int j = 0; j<normalized.rows();++j){
            double diff = normalized(j, i)-mean;
            sum_sq += diff*diff;
        }
        double standard_dev = 0.0;
        if(features.rows()>1){
            standard_dev = std::sqrt(sum_sq/(features.rows()-1));
        } else{
            standard_dev = 0.0;
        }
        if(standard_dev<1e-12){
            standard_dev = 1.0;
        }
        for(int j = 0;j<features.rows();++j){
            normalized(j, i) = (normalized(j, i)-mean)/standard_dev;
        }
    }
    return normalized;
}

//initializes centroids for k-means clustering
Eigen::MatrixXd HiddenMarkovModel::initCentroids(const Eigen::MatrixXd& features, int k){
    Eigen::MatrixXd centroids(k, features.cols());
    int n = features.rows();

    for(int i = 0;i<k;++i){
        int index = std::rand()%n;
        centroids.row(i) = features.row(index);
    }
    return centroids;
}

//assigns clusters based on the closest centroid for each feature vector
std::vector<int> HiddenMarkovModel::assignClusters(const Eigen::MatrixXd& features, const Eigen::MatrixXd& centroids){
    std::vector<int> assignments(features.rows());

    for(int i = 0;i<features.rows();++i){
        double min_dist = std::numeric_limits<double>::max();
        int best_cluster = -1;
        for(int j = 0;j<centroids.rows();++j){
            double dist = (features.row(i)-centroids.row(j)).norm();
            if(dist<min_dist){
                min_dist = dist;
                best_cluster = j;
            }
        }
        assignments[i] = best_cluster;
    }
    return assignments;
}

//updates centroids based on the current assignments of features to clusters
Eigen::MatrixXd HiddenMarkovModel::updateCentroids(const Eigen::MatrixXd& features, const std::vector<int>& assignments, int k){
    int dims = features.cols();
    Eigen::MatrixXd centroids = Eigen::MatrixXd::Zero(k, dims);
    std::vector<int> counts(k, 0);

    for(int i = 0;i<features.rows();++i){
        int cluster = assignments[i];
        centroids.row(cluster) += features.row(i);
        counts[cluster]++;
    }

    for(int i = 0;i<k;++i){
        if(counts[i]>0){
            centroids.row(i) /= counts[i];
        } else{
            centroids.row(i) = features.row(i%features.rows());
        }
    }
    return centroids;
}

//calculates the k-means clustering and returns the mean and variance of each cluster
std::vector<RegimeParameters> HiddenMarkovModel::k_means(int states){
   Eigen::MatrixXd features = buildFeatures(states);
   if(features.rows()<states){
        return {};
   }
   Eigen::MatrixXd normalized_features = normalizeFeatures(features);
   Eigen::MatrixXd centroids = initCentroids(normalized_features, states);
   std::vector<int> assignments(normalized_features.rows(), 0);
   for(int i = 0;i<100;++i){
        std::vector<int> new_assignments = assignClusters(normalized_features, centroids);
        bool converged = true;
        for(int j = 0;j<normalized_features.rows();++j){
            if(new_assignments[j]!=assignments[j]){
                converged = false;
                break;
            }
        }
        if(converged){
            break;
        }
        assignments = new_assignments;
        centroids = updateCentroids(normalized_features, assignments, states);
    }
    std::vector<RegimeParameters> regimes(states);
    std::vector<int> counts(states, 0);

    std::vector<double> sum_returns(states, 0.0);
    std::vector<double> sum_volatility(states, 0.0);
    for(int i = 0;i<features.rows();++i){
        int cluster = assignments[i];
        sum_returns[cluster] += features(i, 0);
        sum_volatility[cluster] += features(i, 1);
        counts[cluster]++;
    }
    for(int i = 0;i<states;++i){
        if(counts[i] == 0){
            continue;
        }
        regimes[i].mean_return = sum_returns[i] / counts[i];
        regimes[i].mean_volatility = sum_volatility[i] / counts[i];
    }
    for(int i = 0;i<features.rows();++i){
        int cluster = assignments[i];
        double diff_return = features(i, 0) - regimes[cluster].mean_return;
        double diff_volatility = features(i, 1) - regimes[cluster].mean_volatility;
        regimes[cluster].variance_return += diff_return * diff_return;
        regimes[cluster].variance_volatility += diff_volatility * diff_volatility;
    }
    for(int i = 0; i<states;++i){
        if(counts[i]>1){
            regimes[i].variance_return /= (counts[i]-1);
            regimes[i].variance_volatility /= (counts[i]-1);
        }
    }
    return regimes;
}