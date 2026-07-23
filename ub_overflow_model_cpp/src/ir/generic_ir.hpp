#ifndef CVPIPELINE_UB_MODEL_CPP_GENERIC_IR_HPP
#define CVPIPELINE_UB_MODEL_CPP_GENERIC_IR_HPP

#include "semantic_ir.hpp"
#include "generic_op_semantics.hpp"

#include <iomanip>
#include <unordered_map>

namespace cvub {

struct GenericOperation {
  int id = -1;
  // Optional identity retained while a pass projects/clones an operation and
  // later needs to relate surviving records to the source module.  This is
  // analysis-only metadata; it is never serialized as an IR attribute.
  int projectionSourceId = -1;
  int parentId = -1;
  int regionId = -1;
  int blockId = -1;
  int ordinal = 0;
  std::string name;
  std::vector<int> results;
  std::vector<int> operands;
  std::vector<std::string> resultTypes;
  std::vector<std::string> operandTypes;
  std::string properties;
  std::string attributes;
  std::vector<int> successors;
  std::string effects = "none";
  std::vector<int> dpsInputs;
  std::vector<int> dpsInits;
  std::vector<int> regions;
};

struct GenericRegion {
  int id = -1;
  int parentOperation = -1;
  int ordinal = 0;
  std::vector<int> blocks;
};

struct GenericBlock {
  int id = -1;
  int regionId = -1;
  int ordinal = 0;
  std::vector<int> arguments;
  std::vector<std::string> argumentTypes;
  std::vector<int> operations;
};

struct GenericModule {
  std::vector<GenericOperation> operations;
  std::vector<GenericRegion> regions;
  std::vector<GenericBlock> blocks;
};

// Optional observer used by GenericRewriter-backed passes to keep derived
// analysis state synchronized without coupling the rewriter to a concrete
// analysis implementation.
class GenericMutationListener {
public:
  virtual ~GenericMutationListener() = default;
  // Returns the operation IDs that currently use value as an SSA operand.
  // A null pointer means that the listener does not provide an incremental
  // use-list and the rewriter must fall back to a module scan.
  virtual const std::vector<int> *replacementUsers(int) const {
    return nullptr;
  }
  virtual void operationCreated(const GenericOperation &) {}
  virtual void operationWillModify(const GenericOperation &) {}
  virtual void operandReplaced(int, size_t, int, int) {}
  virtual void operationMoved(int, int, int) {}
  virtual void regionCreated(const GenericRegion &) {}
  virtual void blockCreated(const GenericBlock &) {}
};

inline std::string HexEncode(const std::string &value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string result;
  result.reserve(value.size() * 2);
  for (char character : value) {
    const auto byte = static_cast<unsigned char>(character);
    result.push_back(digits[byte >> 4]);
    result.push_back(digits[byte & 0xf]);
  }
  return result;
}

inline bool isOpening(char value) {
  return value == '(' || value == '[' || value == '{' || value == '<';
}

inline char matchingClose(char value) {
  switch (value) {
  case '(':
    return ')';
  case '[':
    return ']';
  case '{':
    return '}';
  case '<':
    return '>';
  default:
    return '\0';
  }
}

inline size_t findBalancedClose(const std::string &text, size_t open) {
  if (open >= text.size() || !isOpening(text[open]))
    return std::string::npos;
  std::vector<char> closes = {matchingClose(text[open])};
  bool quoted = false;
  bool escaped = false;
  for (size_t index = open + 1; index < text.size(); ++index) {
    char value = text[index];
    if (quoted) {
      if (escaped)
        escaped = false;
      else if (value == '\\')
        escaped = true;
      else if (value == '"')
        quoted = false;
      continue;
    }
    if (value == '"') {
      quoted = true;
      continue;
    }
    if (isOpening(value)) {
      closes.push_back(matchingClose(value));
      continue;
    }
    if (!closes.empty() && value == closes.back()) {
      closes.pop_back();
      if (closes.empty())
        return index;
    }
  }
  return std::string::npos;
}

inline std::vector<std::string> splitTopLevel(const std::string &text,
                                              char delimiter = ',') {
  std::vector<std::string> result;
  std::vector<char> closes;
  bool quoted = false;
  bool escaped = false;
  size_t start = 0;
  for (size_t index = 0; index < text.size(); ++index) {
    char value = text[index];
    if (quoted) {
      if (escaped)
        escaped = false;
      else if (value == '\\')
        escaped = true;
      else if (value == '"')
        quoted = false;
      continue;
    }
    if (value == '"') {
      quoted = true;
    } else if (isOpening(value)) {
      closes.push_back(matchingClose(value));
    } else if (!closes.empty() && value == closes.back()) {
      closes.pop_back();
    } else if (value == delimiter && closes.empty()) {
      result.push_back(trim(text.substr(start, index - start)));
      start = index + 1;
    }
  }
  std::string tail = trim(text.substr(start));
  if (!tail.empty())
    result.push_back(std::move(tail));
  return result;
}

inline std::string dictionaryBody(const std::string &text, bool properties) {
  size_t open = properties ? text.find("<{") : std::string::npos;
  if (properties) {
    if (open == std::string::npos)
      return "";
    ++open;
  } else {
    for (size_t index = 0; index < text.size(); ++index) {
      if (text[index] != '{' || (index > 0 && text[index - 1] == '<'))
        continue;
      size_t previous = index;
      while (previous > 0 &&
             std::isspace(static_cast<unsigned char>(text[previous - 1])))
        --previous;
      if (previous > 0 && text[previous - 1] == '(')
        continue;
      if (findBalancedClose(text, index) != std::string::npos) {
        open = index;
        break;
      }
    }
    if (open == std::string::npos)
      return "";
  }
  size_t close = findBalancedClose(text, open);
  if (close == std::string::npos)
    throw std::runtime_error("generic IR parser: unbalanced dictionary");
  return text.substr(open + 1, close - open - 1);
}

inline std::string dictionaryFromBodies(
    const std::vector<std::string> &bodies) {
  std::map<std::string, std::string> entries;
  for (const std::string &body : bodies) {
    for (const std::string &entry : splitTopLevel(body)) {
      size_t equal = entry.find('=');
      std::string key = trim(entry.substr(0, equal));
      if (key.size() >= 2 && key.front() == '"' && key.back() == '"')
        key = key.substr(1, key.size() - 2);
      if (!key.empty())
        entries[key] = entry;
    }
  }
  std::string result = "{";
  bool first = true;
  for (const auto &[key, entry] : entries) {
    (void)key;
    if (!first)
      result += ", ";
    first = false;
    result += entry;
  }
  result += "}";
  return result;
}

inline std::vector<std::string> parseTypeGroup(std::string text) {
  text = trim(std::move(text));
  if (text == "()" || text.empty())
    return {};
  if (text.front() == '(') {
    size_t close = findBalancedClose(text, 0);
    if (close == text.size() - 1)
      return splitTopLevel(text.substr(1, close - 1));
  }
  return {text};
}

inline void parseOperationSignature(const std::string &text,
                                    std::vector<std::string> &operands,
                                    std::vector<std::string> &results) {
  size_t colon = text.rfind(" : ");
  if (colon == std::string::npos)
    return;
  std::string signature = trim(text.substr(colon + 3));
  size_t arrow = signature.find(" -> ");
  if (arrow == std::string::npos)
    return;
  operands = parseTypeGroup(signature.substr(0, arrow));
  results = parseTypeGroup(signature.substr(arrow + 4));
}

inline std::vector<std::string> genericResultNames(const std::string &prefix) {
  std::string value = trim(prefix);
  if (value.empty())
    return {};
  std::vector<std::string> result;
  for (std::string group : splitTopLevel(value)) {
    size_t colon = group.rfind(':');
    if (colon == std::string::npos) {
      result.push_back(trim(group));
      continue;
    }
    std::string base = trim(group.substr(0, colon));
    std::string countText = trim(group.substr(colon + 1));
    if (countText.empty() ||
        !std::all_of(countText.begin(), countText.end(), [](unsigned char ch) {
          return std::isdigit(ch) != 0;
        })) {
      result.push_back(std::move(group));
      continue;
    }
    size_t count = static_cast<size_t>(std::stoull(countText));
    for (size_t index = 0; index < count; ++index)
      result.push_back(base + "#" + std::to_string(index));
  }
  return result;
}

inline std::vector<int> parseIdList(const std::vector<std::string> &names,
                                    const std::map<std::string, int> &ids) {
  std::vector<int> result;
  for (const std::string &name : names) {
    auto found = ids.find(name);
    if (found == ids.end())
      throw std::runtime_error("generic IR parser: undefined SSA value " + name);
    result.push_back(found->second);
  }
  return result;
}

inline std::string joinIds(const std::vector<int> &values) {
  std::ostringstream result;
  for (size_t index = 0; index < values.size(); ++index) {
    if (index)
      result << ", ";
    result << values[index];
  }
  return result.str();
}

inline std::string joinHexTypes(const std::vector<std::string> &values) {
  std::ostringstream result;
  for (size_t index = 0; index < values.size(); ++index) {
    if (index)
      result << ", ";
    result << HexEncode(values[index]);
  }
  return result.str();
}

class GenericIRParser {
public:
  explicit GenericIRParser(bool shouldApplyOperationSemantics = true)
      : applyOperationSemantics(shouldApplyOperationSemantics) {}

  GenericModule Parse(const fs::path &path) {
    std::ifstream input(path);
    if (!input)
      throw std::runtime_error("generic IR parser: cannot open " + path.string());
    std::string line;
    while (std::getline(input, line))
      parseLine(trim(line));
    if (!openRegions.empty())
      throw std::runtime_error("generic IR parser: unterminated region");
    for (const auto &[operationId, successorNames] : pendingSuccessors) {
      GenericOperation &operation =
          module.operations.at(static_cast<size_t>(operationId));
      for (const std::string &name : successorNames) {
        auto found = blockLabels.find({operation.regionId, name});
        if (found == blockLabels.end())
          throw std::runtime_error("generic IR parser: undefined successor " +
                                   name);
        operation.successors.push_back(found->second);
      }
    }
    if (module.operations.empty())
      throw std::runtime_error(
          "generic IR parser: no generic operations found; expected "
          "generic-form IR with a quoted builtin.module root");
    if (module.operations.front().name != "builtin.module" ||
        module.operations.front().parentId != -1)
      throw std::runtime_error(
          "generic IR parser: expected quoted builtin.module root, found " +
          module.operations.front().name);
    for (size_t index = 1; index < module.operations.size(); ++index)
      if (module.operations[index].parentId < 0)
        throw std::runtime_error(
            "generic IR parser: operation outside builtin.module: " +
            module.operations[index].name);
    for (const GenericOperation &operation : module.operations)
      ValidateGenericOperationTypeContract(operation);
    if (applyOperationSemantics)
      for (GenericOperation &operation : module.operations)
        ApplyOperationSemantics(operation);
    return std::move(module);
  }

private:
  struct RegionState {
    int regionId = -1;
    int ownerOperation = -1;
    int currentBlock = -1;
  };

  int addRegion(int owner) {
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(owner));
    int id = static_cast<int>(module.regions.size());
    GenericRegion region;
    region.id = id;
    region.parentOperation = owner;
    region.ordinal = static_cast<int>(operation.regions.size());
    operation.regions.push_back(id);
    module.regions.push_back(std::move(region));
    openRegions.push_back({id, owner, -1});
    return id;
  }

  int addBlock(const std::vector<std::string> &names,
               const std::vector<std::string> &types,
               const std::string &label = "") {
    if (openRegions.empty())
      throw std::runtime_error("generic IR parser: block outside region");
    RegionState &state = openRegions.back();
    GenericRegion &region =
        module.regions.at(static_cast<size_t>(state.regionId));
    GenericBlock block;
    block.id = static_cast<int>(module.blocks.size());
    block.regionId = region.id;
    block.ordinal = static_cast<int>(region.blocks.size());
    if (names.size() != types.size())
      throw std::runtime_error("generic IR parser: block argument/type mismatch");
    for (size_t index = 0; index < names.size(); ++index) {
      int valueId = nextValue++;
      values[names[index]] = valueId;
      block.arguments.push_back(valueId);
      block.argumentTypes.push_back(types[index]);
    }
    state.currentBlock = block.id;
    if (!label.empty())
      blockLabels[{state.regionId, label}] = block.id;
    region.blocks.push_back(block.id);
    module.blocks.push_back(std::move(block));
    return state.currentBlock;
  }

  int ensureBlock() {
    if (openRegions.empty())
      return -1;
    if (openRegions.back().currentBlock < 0)
      return addBlock({}, {});
    return openRegions.back().currentBlock;
  }

  std::string expandAliases(std::string text) const {
    std::vector<std::pair<std::string, std::string>> ordered(aliases.begin(),
                                                             aliases.end());
    std::sort(ordered.begin(), ordered.end(), [](const auto &lhs,
                                                  const auto &rhs) {
      return lhs.first.size() > rhs.first.size();
    });
    for (const auto &[name, value] : ordered) {
      size_t position = 0;
      while ((position = text.find(name, position)) != std::string::npos) {
        const size_t end = position + name.size();
        const bool boundary = end == text.size() ||
            (!std::isalnum(static_cast<unsigned char>(text[end])) &&
             text[end] != '_');
        if (!boundary) {
          position = end;
          continue;
        }
        text.replace(position, name.size(), value);
        position += value.size();
      }
    }
    return text;
  }

  void parseBlockHeader(const std::string &line) {
    size_t labelEnd = line.find_first_of("(:");
    std::string label = trim(line.substr(0, labelEnd));
    size_t open = line.find('(');
    if (open == std::string::npos) {
      addBlock({}, {}, label);
      return;
    }
    size_t close = findBalancedClose(line, open);
    if (close == std::string::npos)
      throw std::runtime_error("generic IR parser: malformed block header");
    std::vector<std::string> names;
    std::vector<std::string> types;
    for (const std::string &argument :
         splitTopLevel(line.substr(open + 1, close - open - 1))) {
      size_t colon = argument.find(':');
      if (colon == std::string::npos)
        throw std::runtime_error("generic IR parser: untyped block argument");
      names.push_back(trim(argument.substr(0, colon)));
      types.push_back(trim(argument.substr(colon + 1)));
    }
    addBlock(names, types, label);
  }

  void updateOperationSuffix(int operationId, const std::string &suffix) {
    GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    std::string attributeBody = expandAliases(dictionaryBody(suffix, false));
    operation.attributes = dictionaryFromBodies(
        {operation.properties.empty()
             ? ""
             : operation.properties.substr(1, operation.properties.size() - 2),
         attributeBody});
    parseOperationSignature(suffix, operation.operandTypes,
                            operation.resultTypes);
  }

  void parseOperation(const std::string &line) {
    size_t quote = line.find('"');
    size_t quoteEnd = quote == std::string::npos
                          ? std::string::npos
                          : line.find('"', quote + 1);
    if (quoteEnd == std::string::npos)
      throw std::runtime_error("generic IR parser: malformed operation name");
    size_t equal = line.rfind('=', quote);
    std::vector<std::string> resultNames = genericResultNames(
        equal == std::string::npos ? "" : line.substr(0, equal));
    size_t operandOpen = line.find('(', quoteEnd + 1);
    size_t operandClose = findBalancedClose(line, operandOpen);
    if (operandClose == std::string::npos)
      throw std::runtime_error("generic IR parser: malformed operand list");
    std::vector<std::string> operandNames =
        extractSSAs(line.substr(operandOpen + 1,
                                operandClose - operandOpen - 1));

    GenericOperation operation;
    operation.id = static_cast<int>(module.operations.size());
    operation.name = line.substr(quote + 1, quoteEnd - quote - 1);
    operation.parentId = openRegions.empty()
                             ? -1
                             : openRegions.back().ownerOperation;
    operation.regionId = openRegions.empty() ? -1 : openRegions.back().regionId;
    operation.blockId = ensureBlock();
    if (operation.blockId >= 0) {
      GenericBlock &block =
          module.blocks.at(static_cast<size_t>(operation.blockId));
      operation.ordinal = static_cast<int>(block.operations.size());
      block.operations.push_back(operation.id);
    }
    for (const std::string &name : resultNames) {
      int valueId = nextValue++;
      values[name] = valueId;
      operation.results.push_back(valueId);
    }
    operation.operands = parseIdList(operandNames, values);
    size_t successorOpen = operandClose + 1;
    while (successorOpen < line.size() &&
           std::isspace(static_cast<unsigned char>(line[successorOpen])))
      ++successorOpen;
    if (successorOpen < line.size() && line[successorOpen] == '[') {
      size_t successorClose = findBalancedClose(line, successorOpen);
      if (successorClose == std::string::npos)
        throw std::runtime_error("generic IR parser: malformed successor list");
      std::vector<std::string> names;
      for (const std::string &item : splitTopLevel(
               line.substr(successorOpen + 1,
                           successorClose - successorOpen - 1)))
        names.push_back(trim(item));
      pendingSuccessors[operation.id] = std::move(names);
    }
    std::string propertyBody = expandAliases(dictionaryBody(line, true));
    operation.properties = propertyBody.empty() ? "" : "{" + propertyBody + "}";
    operation.attributes = dictionaryFromBodies(
        {propertyBody, expandAliases(dictionaryBody(line, false))});
    parseOperationSignature(line, operation.operandTypes,
                            operation.resultTypes);
    const int operationId = operation.id;
    module.operations.push_back(std::move(operation));
    if (line.find("({", operandClose) != std::string::npos)
      addRegion(operationId);
  }

  void parseLine(const std::string &line) {
    if (line.empty())
      return;
    if (startsWith(line, "#")) {
      size_t equal = line.find(" = ");
      if (equal != std::string::npos)
        aliases[trim(line.substr(0, equal))] = trim(line.substr(equal + 3));
      return;
    }
    if (startsWith(line, "^")) {
      parseBlockHeader(line);
      return;
    }
    if (startsWith(line, "}, {")) {
      if (openRegions.empty())
        throw std::runtime_error("generic IR parser: region transition without owner");
      int owner = openRegions.back().ownerOperation;
      openRegions.pop_back();
      addRegion(owner);
      return;
    }
    if (startsWith(line, "})")) {
      if (openRegions.empty())
        throw std::runtime_error("generic IR parser: unmatched region close");
      int owner = openRegions.back().ownerOperation;
      openRegions.pop_back();
      updateOperationSuffix(owner, line.substr(2));
      return;
    }
    if (line.find('"') != std::string::npos)
      parseOperation(line);
  }

  GenericModule module;
  std::vector<RegionState> openRegions;
  std::map<std::string, int> values;
  std::map<std::string, std::string> aliases;
  std::map<std::pair<int, std::string>, int> blockLabels;
  std::map<int, std::vector<std::string>> pendingSuccessors;
  int nextValue = 0;
  bool applyOperationSemantics = true;
};

inline GenericModule ParseGenericIR(const fs::path &path,
                                         bool applyOperationSemantics = true) {
  return GenericIRParser(applyOperationSemantics).Parse(path);
}

inline std::string SerializeGenericModule(
    const GenericModule &module,
    const std::string &schema = "GENERIC_IR_SCHEMA") {
  std::ostringstream output;
  output << schema << "\t1\n";
  std::function<void(int)> emit = [&](int operationId) {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    output << "OP\t" << operation.id << '\t' << operation.parentId << '\t'
           << operation.regionId << '\t' << operation.blockId << '\t'
           << operation.ordinal << '\t' << HexEncode(operation.name) << '\t'
           << joinIds(operation.results) << '\t' << joinIds(operation.operands)
           << '\t' << joinHexTypes(operation.resultTypes) << '\t'
           << joinHexTypes(operation.operandTypes) << '\t'
           << HexEncode(operation.properties) << '\t'
           << HexEncode(operation.attributes) << '\t'
           << joinIds(operation.successors) << '\t'
           << HexEncode(operation.effects) << '\t'
           << joinIds(operation.dpsInputs) << '\t'
           << joinIds(operation.dpsInits) << '\n';
    for (int regionId : operation.regions) {
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(regionId));
      output << "REGION\t" << region.id << '\t' << region.parentOperation
             << '\t' << region.ordinal << '\n';
      for (int blockId : region.blocks) {
        const GenericBlock &block =
            module.blocks.at(static_cast<size_t>(blockId));
        output << "BLOCK\t" << block.id << '\t' << block.regionId << '\t'
               << block.ordinal << '\t' << joinIds(block.arguments) << '\t'
               << joinHexTypes(block.argumentTypes) << '\n';
        for (int child : block.operations)
          emit(child);
      }
    }
  };
  if (!module.operations.empty())
    emit(0);
  return output.str();
}

inline std::string
SerializeAfterCVPipeliningSemanticIR(const GenericModule &module) {
  return SerializeGenericModule(module, "AFTER_CVPIPELINING_SEMANTIC_IR");
}

} // namespace cvub

#endif
