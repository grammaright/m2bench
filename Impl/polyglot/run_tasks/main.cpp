#include <string.h>

#include <chrono>
#include <iostream>
#include <string>

#include "Polyglot/tasks.h"

using namespace std;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;

template <typename T, typename O>
void timer(void (*f)(T, T, O, O), T x, T y, O a, O b) {
  cout << "======================= TASK START ========================="
       << endl;
  auto t1 = high_resolution_clock::now();
  (*f)(x, y, a, b);
  auto t2 = high_resolution_clock::now();
  auto ms_int = duration_cast<milliseconds>(t2 - t1);
  cout << "ELAPSED TIME: " << ms_int.count() << " ms" << endl;
  cout << "======================= TASK COMPLETE ======================\n"
       << endl;
}

template <typename T, typename O>
void timer(void (*f)(T, O), T x, O y) {
  cout << "======================= TASK START ========================="
       << endl;
  auto t1 = high_resolution_clock::now();
  (*f)(x, y);
  auto t2 = high_resolution_clock::now();
  auto ms_int = duration_cast<milliseconds>(t2 - t1);
  cout << "ELAPSED TIME: " << ms_int.count() << " ms" << endl;
  cout << "======================= TASK COMPLETE ======================\n"
       << endl;
}

template <typename T>
void timer(void (*f)(T), T x) {
  cout << "======================= TASK START ========================="
       << endl;
  auto t1 = high_resolution_clock::now();
  (*f)(x);
  auto t2 = high_resolution_clock::now();
  auto ms_int = duration_cast<milliseconds>(t2 - t1);
  cout << "ELAPSED TIME: " << ms_int.count() << " ms" << endl;
  cout << "======================= TASK COMPLETE ======================\n"
       << endl;
}

void timer(void (*f)()) {
  cout << "======================= TASK START ========================="
       << endl;
  auto t1 = high_resolution_clock::now();
  (*f)();
  auto t2 = high_resolution_clock::now();
  auto ms_int = duration_cast<milliseconds>(t2 - t1);
  cout << "ELAPSED TIME: " << ms_int.count() << " ms" << endl;
  cout << "======================= TASK COMPLETE ======================\n"
       << endl;
}

int main(int argc, char *argv[]) {
  int SF = 1;
  int task = -1;
  bool isValidation = 0;
  if (argc != 4) {
    cout << "usage: m2bench <TASK_NUM> <SF> <IS_VALIDATION>" << endl;
    cout << "\te.g.,: `m2bench 0 5 1` will execute task 0 with SF=5 in "
            "validation mode (checking the answer is correct)."
         << endl;
    return 0;
  }

  task = atoi(argv[1]);
  SF = atoi(argv[2]);
  isValidation = (atoi(argv[3]) == 0) ? false : true;

  switch (task) {
    case 0:
      timer(T0, SF, isValidation);
      break;
    case 2:
      timer(T2, SF, isValidation);
      break;
    case 9:
      timer(T9, SF, isValidation);
      break;
    case 14:
      timer(T14, SF, isValidation);
      break;
    case 15:
      timer(T15, SF, isValidation);
      break;
    case 16:
      timer(T16, SF, isValidation);
      break;
    default:
      break;
  }

  return 0;
}
