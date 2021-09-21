#include "format.h"

#include <string>

using std::string;
using std::to_string;

// helper formatting function
string formatOnePart(int part) {
  string res = to_string(part);
  if (part < 10) {
    res = string("0") + res;
  }
  return res;
}

// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
// REMOVE: [[maybe_unused]] once you define the function
string Format::ElapsedTime(long seconds) {
  long s = seconds % 60;
  seconds /= 60;
  long m = seconds % 60;
  seconds /= 60;
  long h = seconds;
  return formatOnePart(h) + ":" + formatOnePart(m) + ":" + formatOnePart(s);
}