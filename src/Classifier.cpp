#include "Classifier.h"

void Classifier::setData(std::vector<std::vector<std::vector<double>>> data)
{
  // Finger
  for (size_t i = 0; i < data.size(); i++)
  {
    // Recording
    for (size_t j = 0; j < data.at(i).size(); j++)
    {
      std::vector<int> temp;
      y.push_back(i);
      for (size_t k = 0; k < data.at(i).at(j).size(); k++)
      {
        temp.push_back((int)data.at(i).at(j).at(k));
      }
      x.push_back(temp);
    }
  }
}

Classifier::Classifier()
{
  accuracy = 0;
  trained = false;
  loadModel();
}

Classifier::~Classifier()
{
  // svm_free_and_destroy_model(&model);
}

void Classifier::train()
{
  if (!x.size())
    return;

  auto [X_train, X_test, y_train, y_test] = split_data(x, y, 0.1, rand());

  svm_problem prob;
  svm_parameter param;

  prob.l = X_train.size();
  prob.y = new double[prob.l];
  prob.x = new svm_node *[prob.l];

  for (int i = 0; i < prob.l; ++i)
  {
    prob.y[i] = (double)y_train[i];
    prob.x[i] = new svm_node[X_train[i].size() + 1];
    for (int j = 0; j < X_train[i].size(); ++j)
    {
      prob.x[i][j].index = j + 1;
      prob.x[i][j].value = (double)X_train[i][j];
    }
    prob.x[i][X_train[i].size()].index = -1;
  }

  param.svm_type = C_SVC;
  param.kernel_type = RBF;
  param.C = 100;
  param.shrinking = 1;
  param.cache_size = 200;
  param.probability = 1;
  param.eps = 0.001f;
  param.nr_weight = 0;
  param.gamma = 0.000125f;

  model = svm_train(&prob, &param);

  int correct = 0;
  for (int i = 0; i < X_test.size(); ++i)
  {
    svm_node *x_test_node = new svm_node[X_test[i].size() + 1];
    for (int j = 0; j < X_test[i].size(); ++j)
    {
      x_test_node[j].index = j + 1;
      x_test_node[j].value = (double)X_test[i][j];
    }
    x_test_node[X_test[i].size()].index = -1;

    double prediction = svm_predict(model, x_test_node);
    correct += prediction == (double)y_test[i] ? 1 : 0;

    delete[] x_test_node;
  }
  accuracy = (double)correct / (double)y_test.size();
  trained = true;
  saveModel();
}

void Classifier::minMaxScale()
{
}

bool Classifier::loadModel()
{
  svm_model *loadedModel = svm_load_model("model");
  if (loadedModel)
  {
    model = loadedModel;
    trained = true;
  }
  return trained;
}

bool Classifier::saveModel()
{
  if (!trained)
    return false;
  int saved = svm_save_model("model", model);
  trained = saved == 0;
  return trained;
}

int Classifier::predict(std::vector<double> data, double *probs)
{
  if (!trained)
    return -1;
  svm_node *data_node = new svm_node[data.size() + 1];
  for (int j = 0; j < data.size(); ++j)
  {
    data_node[j].index = j + 1;
    data_node[j].value = (double)data[j];
  }
  data_node[data.size()].index = -1;
  double prediction = 0;
  if (probs != nullptr)
  {
    prediction = svm_predict_probability(model, data_node, probs);
  }
  else
  {
    prediction = svm_predict(model, data_node);
  }

  delete[] data_node;
  return (int)prediction;
}

std::tuple<std::vector<std::vector<int>>, std::vector<std::vector<int>>, std::vector<int>, std::vector<int>> Classifier::split_data(const std::vector<std::vector<int>> &XX, const std::vector<int> &yy, double test_size, int random_state)
{
  std::vector<int> indices(XX.size());          // 0, 1, 2, ...
  std::iota(indices.begin(), indices.end(), 0); // Fill with indices

  std::mt19937 generator(random_state);
  std::shuffle(indices.begin(), indices.end(), generator);

  int split_index = static_cast<int>(XX.size() * (1.0 - test_size));

  std::vector<std::vector<int>> X_train, X_test;
  std::vector<int> y_train, y_test;

  for (int i = 0; i < XX.size(); ++i)
  {
    if (i < split_index)
    {
      X_train.push_back(XX[indices[i]]);
      y_train.push_back(yy[indices[i]]);
    }
    else
    {
      X_test.push_back(XX[indices[i]]);
      y_test.push_back(yy[indices[i]]);
    }
  }

  return {X_train, X_test, y_train, y_test};
}
