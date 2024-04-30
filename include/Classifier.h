#pragma once

#include <svm.h>
#include <vector>
#include <stdio.h>
#include <random>
#include <algorithm>
#include <iostream>

class Classifier
{
private:
  std::vector<std::vector<int>> x;
  std::vector<std::vector<int>> xScaled;
  std::vector<int> y;
  svm_model *model;

public:
  double accuracy;
  bool trained;
  Classifier();
  ~Classifier();
  void setData(std::vector<std::vector<std::vector<double>>> data);
  void train();
  void minMaxScale();
  int predict(std::vector<double> data);
  std::tuple<std::vector<std::vector<int>>, std::vector<std::vector<int>>, std::vector<int>, std::vector<int>> split_data(const std::vector<std::vector<int>> &X,
                                                                                                                                     const std::vector<int> &y,
                                                                                                                                     double test_size, int random_state);
};