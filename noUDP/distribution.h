#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <vector>
#include <math.h>

std::vector<double> ideal_distribution(int n)
{
    if(n < 1) 
    {
        throw std::logic_error("n should be at least 1!");
    }
    double sum = 0.0;
    std::vector<double> probabilities(n + 1);

    // calculate probabilities of ideal_distribution
    probabilities[0] = 0;
    probabilities[1] = 1 / (double)n;
    for(int i = 2; i <= n; ++i)
        probabilities[i] = 1 / (double)(i * (i - 1));
    // test : sum of probabilities must equal to 1
    for(double number : probabilities) 
        sum += number;
    if(abs(1 - sum) > 0.000001) 
    {
        std::cerr << "sum : " << sum << std::endl;
        throw std::runtime_error("sum of probabilities not equal to 1!");
    }
    return probabilities;
}

std::vector<double> robust_distribution(int n)
{
    double RFP = 0.01;
    int m = n / 2 + 1;
    double sum = 0.0;
    std::vector<double> probabilities(n + 1, 0);
    std::vector<double> ideal_pros = ideal_distribution(n);

    // calculate probabilities of robust_distribution
    for(int i = 1; i < m; ++i)
        probabilities[i] = 1 / (double)(i * m);
    probabilities[m] = log(n / ((double)m * RFP)) / (double)m;
    for(int i = 0; i <= n; ++i)
        probabilities[i] += ideal_pros[i];
    for(double number : probabilities) 
        sum += number;
    for(double& num : probabilities) 
        num /= sum;
    // test : sum of probabilities must equal to 1
    sum = 0;
    for(double number : probabilities) 
        sum += number;
    if(abs(1 - sum) > 0.000001) 
    {
        std::cerr << "sum : " << sum << std::endl;
        throw std::runtime_error("sum of probabilities not equal to 1!");
    }
    return probabilities;
}


#endif
