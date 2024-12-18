#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include "csvstream.hpp"
using namespace std;
class Classifier {
  private:
  int numT; 
  vector<string> content; 
  map<string, int> vocab; 
  map<string, int> tag_counts; 
  map<string, double> tag_prob; 
  map<string, map<string, int>> word_counts;
  map<string, map<string, double>> word_prob;
  public:
  Classifier() : numT(0) {} 
  ~Classifier() = default;
  void initialize() {
    numT = 0;
    tag_counts.clear();
    word_counts.clear();
    tag_prob.clear();
    vocab.clear();
    content.clear();
  }
  void train(const string& filename) {  
    csvstream csvin(filename); 
    map<string, string> row;
    while (csvin >> row) {
      numT++; //Increment total number of posts counted
      string tag = row["tag"];
      string content = row["content"]; 
      tag_counts[tag]++; // Increment the count for this tag
      set<string> unique_words_post = unique_words(content);
      for (const auto& word : unique_words_post) {
          word_counts[tag][word]++;
          vocab[word]++;
      }
    }
    for (const auto& tag_entry : tag_counts) {
      log_prior(tag_entry.first);
    }
  }
  pair<string, double> predict(const string& content) {
    map <string, double> potentials; 
    set<string> unique_words_post = unique_words(content);
    for (const auto& label : tag_prob){
      double total_log_prob = label.second; 
      for (const auto& word : unique_words_post) {
        total_log_prob += cal_word_prob(word, label.first); 
      }
      potentials[label.first] = total_log_prob;
    }
    pair<std::string, double> maxPair = *potentials.begin(); 
    for (const auto& pair : potentials) {
        if (pair.second > maxPair.second) {
            maxPair = pair;
        }
    }
    return maxPair; 
  }
  double cal_word_prob(const string& word, const string& tag) {
    const auto& tag_word_counts = word_counts.at(tag); // tag_word_counts = C
    bool word_in_vocab = vocab.find(word) != vocab.end();
    if (tag_word_counts.find(word) == tag_word_counts.end() && word_in_vocab){
      return log(static_cast<double>(vocab[word])/ numT);
    }
    if (!word_in_vocab) { 
      return log(1.0 / numT); 
    } 
    return log(static_cast<double> (tag_word_counts.at(word)) / tag_counts[tag]);
  }
  void log_prior(const string& tag) {
    auto it = tag_counts.find(tag);
    if (it == tag_counts.end()) {
      cout << "Runtime Error: Label not found in training data";
    }
    tag_prob[tag] = log(static_cast<double> (tag_counts[tag]) / numT);
  }
  set<string> unique_words(const string& str) {
    istringstream source(str);
    set<string> words;
    string word;
    while (source >> word) {
      words.insert(word);
    }
    return words;
  }
  int get_numT() const {
    return numT;
  } 
  int get_numV() const{
    return vocab.size();
  }
  const map<string, int>& get_tag_counts() const {
    return tag_counts;
  }
  const map<string, double>& get_tag_prob() const {
    return tag_prob;
  }
  const map<string, map<string, int>>& get_word_counts() const {
    return word_counts;
  }
  const map<string, map<string, double>>& get_word_prob() const{
    return word_prob;
  }
  void train_out(Classifier classifier, const string& filename) {
      int numT = classifier.get_numT();
      cout << "training data:" << endl;
      csvstream csvin(filename);
      map<string, string> row; // tag and content
      while (csvin >> row) {
        string label = row["tag"];
        string content = row["content"];
        cout << "  label = " << label << ", content = " << content << endl;
      }
      cout << "trained on " << numT << " examples" << endl;
      cout << "vocabulary size = " << classifier.get_numV() << endl;
      cout << endl; // additional blank line as per spec
      cout << "classes:" << endl;
      for (const auto& pair : classifier.get_tag_prob()) {
        const string& label = pair.first;
        int label_count = classifier.get_tag_counts().at(label);
        double log_value = pair.second;
        cout << "  " << label << ", " << label_count  
          << " examples" << ", log-prior = " << log_value << endl;
      }
      cout << "classifier parameters:" << endl;
      const auto& word_counts = classifier.get_word_counts();
      for (const auto& tag : classifier.get_word_counts()) {      
        const string& label = tag.first;
        for (const auto& word_entry : tag.second) {
          const string& word = word_entry.first;
          double log_value = classifier.cal_word_prob(word, label);
          cout << "  " << label << ":" << word << ", count = " 
            << word_counts.at(label).at(word) 
            << ", log-likelihood = " << log_value << endl;
        }
      }
      cout << endl; // Extra line as per spec    
  }
  void test_out(Classifier& classifier, const string& filename) {
    int numC = 0, numP = 0; // Total Correct Predictions, and Predictions
      cout << "trained on " << classifier.get_numT() << " examples" << endl << endl;
      csvstream csvin(filename);
      map<string, string> row; 
      cout << "test data:" << endl;
      while (csvin >> row) {
        string correct_tag = row["tag"];
        string content = row["content"];
        const auto& prediction = classifier.predict(content);
        cout << "  correct = " << correct_tag << ", predicted = "
          << prediction.first << ", log-probability score = " 
          << prediction.second << endl;
        if (correct_tag == prediction.first) { ++numC;} 
        cout << "  content = " << content << endl << endl; 
        ++numP;
      }
      cout << "performance: " << numC 
        << " / " << numP << " posts predicted correctly" << endl;
  }
};
int main(int argc, char **argv) {
  cout.precision(3); // setting floating point precision
  bool trainOnly = argc != 3; //
  if (argc != 2 && trainOnly){ 
    cout << "Usage: classifier.exe TRAIN_FILE [TEST_FILE]" << endl;
    return 1;
  }
  ifstream trainFile(argv[1]);
  if (!trainFile.is_open()) { // Error handling
    cout << "Error opening file: " << argv[1] << endl;
    return 1;
  }
  ifstream testFile; //initialize testFile
  if(!trainOnly) {
    testFile.open(argv[2]); //opens testFile
    if (!testFile.is_open()) { // Error handling
      cout << "Error opening file: " << argv[2] << endl;
      return 1;
    } 
  } 
  Classifier classifier;
  classifier.train(argv[1]);
  if(trainOnly){
    classifier.train_out(classifier, argv[1]);
  return 0;
  } else {
    classifier.test_out(classifier, argv[2]);
    return 0;
  }
}