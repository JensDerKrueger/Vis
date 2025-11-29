#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <sstream>
#include <fstream>
#include <cctype>
#include <chrono>

enum class CommandResultCode {
  success = 0,
  finished,
  waitingNoop,
  fileOpenFailed,
  unknownCommand,
  invalidArguments,
  callbackError
};

class CommandInterpreter {
public:
  using Callback = std::function<CommandResultCode(const std::vector<std::string> &)>;
  using UnknownCommandCallback =
  std::function<CommandResultCode(const std::string &, const std::vector<std::string> &)>;

  CommandInterpreter() = default;

  CommandResultCode registerCommand(const std::string &commandName,
                                    Callback callbackFunction) {
    commandMap[commandName] = std::move(callbackFunction);
    return CommandResultCode::success;
  }

  void setUnknownCommandHandler(UnknownCommandCallback callbackFunction) {
    unknownCommandHandler = std::move(callbackFunction);
  }

  CommandResultCode loadFromFile(const std::string &filePath) {
    std::ifstream inStream(filePath);
    if (!inStream.is_open()) {
      return CommandResultCode::fileOpenFailed;
    }
    return parseFromStream(inStream);
  }

  CommandResultCode loadFromString(const std::string &commands) {
    std::istringstream inStream(commands);
    return parseFromStream(inStream);
  }

  CommandResultCode runBatch() {
    if (noopActive) {
      auto now = std::chrono::steady_clock::now();
      if (now < noopUntil) {
        return CommandResultCode::waitingNoop;
      }
      noopActive = false;
    }

    if (instructionIndex >= instructions.size()) {
      return CommandResultCode::finished;
    }

    bool executedAny = false;

    while (instructionIndex < instructions.size()) {
      Instruction &instruction = instructions[instructionIndex];

      if (instruction.isSeparator) {
        ++instructionIndex;
        if (!executedAny) {
          continue;
        }
        bool hasMoreCommands = false;
        for (std::size_t i = instructionIndex; i < instructions.size(); ++i) {
          if (!instructions[i].isSeparator) {
            hasMoreCommands = true;
            break;
          }
        }
        if (hasMoreCommands) {
          return CommandResultCode::success;
        } else {
          return CommandResultCode::finished;
        }
      }

      executedAny = true;

      const std::string &command = instruction.tokens[0];
      std::vector<std::string> args(instruction.tokens.begin() + 1,
                                    instruction.tokens.end());

      if (command == "noop") {
        if (args.size() != 1) {
          return CommandResultCode::invalidArguments;
        }
        try {
          long long milliseconds = std::stoll(args[0]);
          if (milliseconds <= 0) {
            ++instructionIndex;
            continue;
          }
          auto now = std::chrono::steady_clock::now();
          noopUntil = now + std::chrono::milliseconds(milliseconds);
          noopActive = true;
        } catch (...) {
          return CommandResultCode::invalidArguments;
        }
        ++instructionIndex;
        return CommandResultCode::waitingNoop;
      }

      auto it = commandMap.find(command);
      if (it != commandMap.end()) {
        CommandResultCode result = it->second(args);
        if (result != CommandResultCode::success) {
          return result;
        }
      } else {
        if (unknownCommandHandler) {
          CommandResultCode result = unknownCommandHandler(command, args);
          if (result != CommandResultCode::success) {
            return result;
          }
        } else {
          return CommandResultCode::unknownCommand;
        }
      }

      ++instructionIndex;
    }

    return CommandResultCode::finished;
  }

  void reset() {
    instructions.clear();
    instructionIndex = 0;
    noopActive = false;
  }

private:
  struct Instruction {
    bool isSeparator;
    std::vector<std::string> tokens;
  };

  std::unordered_map<std::string, Callback> commandMap;
  UnknownCommandCallback unknownCommandHandler{nullptr};
  std::vector<Instruction> instructions;
  std::size_t instructionIndex{0};
  bool noopActive{false};
  std::chrono::steady_clock::time_point noopUntil{};

  static std::string trim(const std::string &text) {
    std::size_t start = 0;
    while (start < text.size() &&
           std::isspace(static_cast<unsigned char>(text[start]))) {
      ++start;
    }

    std::size_t end = text.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(text[end - 1]))) {
      --end;
    }

    return text.substr(start, end - start);
  }

  static std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
      tokens.push_back(token);
    }
    return tokens;
  }

  CommandResultCode parseFromStream(std::istream &inStream) {
    instructions.clear();
    instructionIndex = 0;
    noopActive = false;

    std::string line;
    bool lastWasSeparator = false;

    while (std::getline(inStream, line)) {
      std::size_t hashPos = line.find('#');
      if (hashPos != std::string::npos) {
        line = line.substr(0, hashPos);
      }

      std::string trimmedLine = trim(line);

      if (trimmedLine.empty()) {
        if (!instructions.empty() && !lastWasSeparator) {
          Instruction separatorInstruction;
          separatorInstruction.isSeparator = true;
          instructions.push_back(separatorInstruction);
          lastWasSeparator = true;
        }
        continue;
      }

      std::vector<std::string> tokens = tokenize(trimmedLine);
      if (tokens.empty()) {
        continue;
      }

      Instruction instruction;
      instruction.isSeparator = false;
      instruction.tokens = std::move(tokens);
      instructions.push_back(std::move(instruction));
      lastWasSeparator = false;
    }

    return CommandResultCode::success;
  }
};
