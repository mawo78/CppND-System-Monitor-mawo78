#include "processor.h"

#include "linux_parser.h"

// Return the aggregate CPU utilization
float Processor::Utilization() {
  float active = LinuxParser::ActiveJiffies();
  float idle = LinuxParser::IdleJiffies();
  float util = active / (active + idle);
  return util;
}