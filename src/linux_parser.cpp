#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "format.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

const int HZ = sysconf(_SC_CLK_TCK);

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

class LineReader {
 public:
  static string ReadNthLineFull(const string& filePath, int lineNr = 0) {
    string line;
    string res;
    std::ifstream stream(filePath);
    if (stream.is_open()) {
      do {
        if (!std::getline(stream, line)) {
          return "";
        }
      } while (lineNr--);
    }
    return line;
  }

  static vector<string> ReadNthLine(const string& filePath, int lineNr = 0) {
    string line = ReadNthLineFull(filePath, lineNr);
    std::istringstream linestream(line);
    std::string pom;
    vector<string> res;
    while (linestream) {
      linestream >> pom;
      res.emplace_back(pom);
    }
    return res;
  }

  static vector<string> ReadAllLines(const string& filePath,
                                     const char delim = '\n') {
    string line;
    vector<string> res;
    std::ifstream stream(filePath);
    if (stream.is_open()) {
      while (std::getline(stream, line, delim)) {
        res.emplace_back(line);
      }
    }
    return res;
  }
};

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  vector<string> infoLine =
      LineReader::ReadNthLine(kProcDirectory + kVersionFilename);
  if (infoLine.size() >= 3)
    return infoLine[2];
  else
    return "";
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// TODO: Read and return the system memory utilization
float LinuxParser::MemoryUtilization() {
  vector<string> memLine1 =
      LineReader::ReadNthLine(kProcDirectory + kMeminfoFilename);
  vector<string> memLine2 =
      LineReader::ReadNthLine(kProcDirectory + kMeminfoFilename, 1);
  if (memLine1.size() >= 2 && memLine2.size() >= 2) {
    int memTotal = std::stoi(memLine1[1]);
    int memFree = std::stoi(memLine2[1]);
    return 1.0f - (static_cast<float>(memFree) / memTotal);
  }
  return 0.0f;
}

// Read and return the system uptime
long LinuxParser::UpTime() {
  vector<string> uptimeLine =
      LineReader::ReadNthLine(kProcDirectory + kUptimeFilename);
  long upTime = static_cast<long>(stof(uptimeLine[0]));
  // std::cout<<upTime<<std::endl;
  return upTime;
}

// TODO: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  vector<string> uptimeLine =
      LineReader::ReadNthLine(kProcDirectory + kUptimeFilename);
  long upTime = static_cast<long>(stof(uptimeLine[0]) * sysconf(_SC_CLK_TCK));
  return upTime;
}

// TODO: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::ActiveJiffies(int pid) {
  vector<string> lineParsed;
  lineParsed =
      LineReader::ReadNthLine(kProcDirectory + to_string(pid) + kStatFilename);
  if (lineParsed.size() <= kCStime) return 0;
  long res = stoi(lineParsed[kUtime]);
  res += stoi(lineParsed[kStime]);
  res += stoi(lineParsed[kCUtime]);
  res += stoi(lineParsed[kCStime]);
  return res;
}

long LinuxParser::StartTime(int pid) {
  vector<string> lineParsed;
  lineParsed =
      LineReader::ReadNthLine(kProcDirectory + to_string(pid) + kStatFilename);
  if (lineParsed.size() <= kStartTime)
    return 0;
  else
    return stoi(lineParsed[kStartTime]);
}

// TODO: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  vector<string> cpuInfo = CpuUtilization();
  long sumTimes = 0;
  cpuInfo[kIdle_ + 1] = "0";
  for (auto it = ++cpuInfo.begin(); it != cpuInfo.end(); it++) {
    sumTimes += std::stoi(*it);
  }
  return sumTimes;
}

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  vector<string> cpuInfo = CpuUtilization();
  return std::stoi(cpuInfo[kIdle_ + 1]);
}

// Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() {
  return LineReader::ReadNthLine(kProcDirectory + kStatFilename);
}

// Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  int lineNr = 0;
  vector<string> lineParsed;
  do {
    lineParsed =
        LineReader::ReadNthLine(kProcDirectory + kStatFilename, lineNr++);
  } while (!lineParsed.empty() && lineParsed[0] != "processes");

  if (lineParsed.size() >= 2)
    return stoi(lineParsed[1]);
  else
    return 0;
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  int lineNr = 0;
  vector<string> lineParsed;
  do {
    lineParsed =
        LineReader::ReadNthLine(kProcDirectory + kStatFilename, lineNr++);
  } while (!lineParsed.empty() && lineParsed[0] != "procs_running");

  if (lineParsed.size() >= 2)
    return stoi(lineParsed[1]);
  else
    return 0;
}

// Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid) {
  vector<string> cmdFile;
  cmdFile = LineReader::ReadAllLines(
      kProcDirectory + to_string(pid) + kCmdlineFilename, '\0');

  std::stringstream str;
  std::copy(cmdFile.begin(), cmdFile.end(),
            std::ostream_iterator<string>(str, " "));
  return str.str();
}

// Read and return the memory used by a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Ram(int pid) {
  vector<string> lineParsed;
  lineParsed = LineReader::ReadNthLine(
      kProcDirectory + to_string(pid) + kStatusFilename, 17);
  if (lineParsed.size() >= 2)
    return to_string(stoi(lineParsed[1]) / 1024);
  else
    return "0";
}

// Read and return the user ID associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Uid(int pid) {
  vector<string> lineParsed;
  lineParsed = LineReader::ReadNthLine(
      kProcDirectory + to_string(pid) + kStatusFilename, 8);
  if (lineParsed.size() >= 2)
    return lineParsed[1];
  else
    return "";
}

// TODO: Read and return the user associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::User(int pid) {
  string uid = Uid(pid);

  static std::unordered_map<string, string> uidToName;
  if (uidToName.empty() || uidToName.end() == uidToName.find(uid)) {
    vector<string> allUsers = LineReader::ReadAllLines(kPasswordPath);
    for (auto line : allUsers) {
      std::stringstream ss(line);
      string item;
      vector<string> parsedLine;
      while (getline(ss, item, ':')) {
        parsedLine.push_back(item);
      }
      if (parsedLine.size() >= 3) uidToName[parsedLine[2]] = parsedLine[0];
    }
  }

  return uidToName[uid];
}

// TODO: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid) {
  // vector<string> lineParsed;
  // lineParsed =
  //     LineReader::ReadNthLine(kProcDirectory + to_string(pid) +
  //     kStatFilename);
  // if (lineParsed.size() >= 22)
  //   return LinuxParser::UpTime() - stoi(lineParsed[21]) / HZ;
  // else
  //   return 0;
  return LinuxParser::ActiveJiffies(pid) / HZ;
}

float LinuxParser::CpuUtilization(int pid) {
  float totalTime = LinuxParser::ActiveJiffies(pid);
  float upTime = LinuxParser::UpTime();
  float seconds =
      upTime - (static_cast<float>(LinuxParser::StartTime(pid)) / HZ);
  return (totalTime / HZ) / seconds;
}