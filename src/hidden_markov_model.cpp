#include "hidden_markov_model.hpp"
#include <numbers>
HiddenMarkovModel::HiddenMarkovModel(){
    num_states = 3;

}

//initializes regimes, state vector and transition matrix
void HiddenMarkovModel::initializefromRegimes(const std::vector<RegimeParameters>& regimes){
    int K = regimes.size();
    num_states = K;
    this->regimes = regimes;
    transition_matrix = Eigen::MatrixXd::Zero(K, K);
    initial_state_vector = Eigen::VectorXd::Constant(K, 1.0/K);

    for(int i = 0;i<K;++i){
        for(int j = 0;j<K;++j){
            if(i==j){
                transition_matrix(i, j) = 0.9;
            } else{
                transition_matrix(i, j) = 0.1/(K-1);
            }
        }
    }
}

double HiddenMarkovModel::gaussian_pdf(double x, double mean, double variance) const{
    if(variance <= 1e-12){
        variance = 1e-12; //if variance small to avoid division by zero
    }
    double pi = 3.14159265358979323846;
    double coeff = 1.0/std::sqrt(2.0*pi*variance);
    double exponent = -((x-mean)*(x-mean))/(2.0*variance);
    return coeff * std::exp(exponent);
}

double HiddenMarkovModel::emissionProbability(double return_value, int state, double vol) const {
    double mean = regimes[state].mean_return;
    double variance = regimes[state].variance_return;
    return gaussian_pdf(return_value, mean, variance)*gaussian_pdf(vol, regimes[state].mean_volatility, regimes[state].variance_volatility);
}

double HiddenMarkovModel::baum_welch_algorithm(Eigen::MatrixXd& observations){
    int t = observations.rows();
    int k = num_states;
    if(t == 0 || k == 0){
        return 0.0;
    }
    double prev_log_likelihood = -std::numeric_limits<double>::infinity();
    for(int loop = 0;loop<100;loop++){
        auto [alpha, likelihood] = forward_algorithm(observations);
        Eigen::MatrixXd beta = backward_algorithm(observations);
        if(likelihood<=0.0){
            break;
        }

        Eigen::MatrixXd gamma(t, k);
        for(int i = 0;i<t;++i){
            double norm = 0.0;
            for(int j = 0;j<k;++j){
                gamma(i, j) = alpha(i, j)*beta(i, j);
                norm+= gamma(i, j);
            }
            for(int j = 0;j<k;++j){
                gamma(i, j)/= norm;
            }
        }

        Eigen::MatrixXd xi = Eigen::MatrixXd::Zero(k, k);
        for(int i = 0;i<t-1;++i){
            for( int j = 0;j<k;++j){
                for(int l = 0;l<k;++l){
                    double b = emissionProbability(observations(i+1, 0), l, observations(i+1, 1));
                    xi(j, l)+= alpha(i, j)*transition_matrix(j, l)*b*beta(i+1, l)/likelihood;
                }
            }
        }
        for(int i = 0;i<k;++i){
            initial_state_vector(i) = gamma(0, i);
        }
        for(int i = 0;i<k;++i){
            double total = xi.row(i).sum();
            if(total>0.0){
                transition_matrix.row(i) = xi.row(i)/total;
            }
        }
        for(int j = 0;j<k;++j){
            double weight = gamma.col(j).sum();
            if(weight<1e-10){
                continue;
            }
            double mean_return = 0.0;
            double mean_volatility = 0.0;
            for(int l = 0;l<t;++l){
                mean_return += gamma(l, j)*observations(l, 0);
                mean_volatility += gamma(l, j)*observations(l, 1);
            }
            mean_return /= weight;
            mean_volatility /= weight;
            double variance_return = 0.0;
            double variance_volatility = 0.0;
            for(int l = 0;l<t;++l){
                variance_return += gamma(l, j)*(observations(l, 0) - mean_return)*(observations(l, 0) - mean_return);
                variance_volatility += gamma(l, j)*(observations(l, 1) - mean_volatility)*(observations(l, 1) - mean_volatility);
            }
            regimes[j].mean_return = mean_return;
            regimes[j].mean_volatility = mean_volatility;
            regimes[j].variance_return = std::max(variance_return/weight, 1e-10);
            regimes[j].variance_volatility = std::max(variance_volatility/weight, 1e-10);
        }
        double log_likelihood = std::log(likelihood);
        if(std::abs(log_likelihood - prev_log_likelihood) < 1e-6){
            prev_log_likelihood = log_likelihood;
            break;
        }
        prev_log_likelihood = log_likelihood;
    }
    return prev_log_likelihood;
}

//used for baum-welch algorithm
Eigen::MatrixXd HiddenMarkovModel::backward_algorithm(Eigen::MatrixXd& observations){
    int t = observations.rows();
    int k = num_states;
    if(t == 0 || k == 0){
        return Eigen::MatrixXd(0, 0);
    }
    if(transition_matrix.rows() == 0 || transition_matrix.cols() == 0){
        return Eigen::MatrixXd(0, 0);
    }
    Eigen::MatrixXd beta(t, k); 
    for (int i = 0; i < k; ++i){
        beta(t-1, i) = 1.0; //initialize last row to 1.0
    }
    //for loop to fill in delta and psi matrices
    for (int T = t - 2; T >= 0; --T){
        for (int j = 0; j < k; ++j){
            double sum = 0.0;
            for (int i = 0; i < k; ++i){
                double b = emissionProbability(observations(T+1, 0), i, observations(T+1, 1));
                sum+= beta(T+1, i)*transition_matrix(j, i)*b;
            }
            beta(T, j) = sum;
        }
    }
    return beta;
}


std::pair<Eigen::MatrixXd, double> HiddenMarkovModel::forward_algorithm(Eigen::MatrixXd& observations){
    int t = observations.rows();
    int k = num_states;
    if(t == 0 || k == 0){
        return {Eigen::MatrixXd(0, 0), 0.0};
    }
    if(transition_matrix.rows() == 0 || transition_matrix.cols() == 0){
        return {Eigen::MatrixXd(0, 0), 0.0};
    }
    Eigen::MatrixXd alpha(t, k); //best score for each state at time t
    for (int i = 0; i < k; ++i){
        double b = emissionProbability(observations(0, 0), i, observations(0, 1));
        alpha(0, i) = initial_state_vector(i)*b;
    }
    //for loop to fill in delta and psi matrices
    for (int T = 1; T < t; ++T){
        for (int j = 0; j < k; ++j){
            double sum = 0.0;
            for (int i = 0; i < k; ++i){
                sum+= alpha(T-1, i)*transition_matrix(i, j);
            }
            double b = emissionProbability(observations(T, 0), j, observations(T, 1));
            alpha(T, j) = sum*b;
        }
    }
    double total = 0.0;
    for(int i = 0;i<k;++i){
        total+= alpha(t-1, i);
    }
    return {alpha, total};
}

//viterbi algorithm implementation
std::pair<Eigen::VectorXd, double> HiddenMarkovModel::viterbi_algorithm(Eigen::MatrixXd& observations){
    int t = observations.rows();
    int k = num_states;
    if(t == 0 || k == 0){
        return {Eigen::VectorXd(0), 0.0};
    }
    if(transition_matrix.rows() == 0 || transition_matrix.cols() == 0){
        return {Eigen::VectorXd(0), 0.0};
    }
    Eigen::MatrixXd delta(t, k); //best score for each state at time t
    Eigen::MatrixXi psi(t, k); //points back @ best score
    for (int i = 0; i < k; ++i){
        double b = emissionProbability(observations(0, 0), i, observations(0, 1));
        delta(0, i) = initial_state_vector(i)*b;
        psi(0, i) = 0;
    }
    //for loop to fill in delta and psi matrices
    for (int T = 1; T < t; ++T){
        for (int j = 0; j < k; ++j){
            double best = delta(T - 1, 0) * transition_matrix(0, j);
            int best_i = 0;
            for (int i = 0; i < k; ++i){
                double v = delta(T - 1, i) * transition_matrix(i, j);
                if (v > best) {
                    best = v;
                    best_i = i;
                }
            }
            double b = emissionProbability(observations(T, 0), j, observations(T, 1));
            delta(T, j) = best * b;
            psi(T, j) = best_i;
        }
    }
    int last = 0; 
    double best = delta(t-1, 0);
    //pick best and walk back to get most likely state
    for(int i = 1;i<k;++i){
        if(delta(t-1, i)>best){
            best = delta(t-1, i);
            last = i;
        }
    }
    Eigen::VectorXd path(t);
    path(t-1) = last;
    for(int T = t-2;T>=0;--T){
        path(T) = psi(T+1, (int)path(T+1));
    }
    return {path, best};
}



//first thing
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

//second thing
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

//third thing
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

//fourth thing
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

//fifth thing
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

//sixth thing
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

//uses all of the things above to calculate the k-means clustering and returns the mean and variance of each cluster
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