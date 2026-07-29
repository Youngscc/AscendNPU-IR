#ifndef UB_OVERFLOW_MODEL_CPP_PRE_CV_HIVM_OPT_SINGLE_POINT_HPP
#define UB_OVERFLOW_MODEL_CPP_PRE_CV_HIVM_OPT_SINGLE_POINT_HPP

#include "../ir/generic_analysis.hpp"
#include "../ir/generic_rewriter.hpp"
#include "../ir/operation_folder.hpp"

#include <map>
#include <optional>
#include <queue>
#include <set>

namespace cvub {

inline bool PreCVSinglePointAllOnes(const std::string &type) {
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
  return parsed &&
         std::all_of(parsed->shape.begin(), parsed->shape.end(),
                     [](const std::optional<int64_t> &dimension) {
                       return dimension && *dimension == 1;
                     });
}

inline bool PreCVSinglePointIsHost(const GenericOperation &function) {
  return function.properties.find("#hacc.function_kind<HOST>") !=
             std::string::npos ||
         function.attributes.find("#hacc.function_kind<HOST>") !=
             std::string::npos;
}

inline bool PreCVSinglePointHasNoIOAlias(
    const GenericOperation &function) {
  return function.properties.find("hacc.no_io_alias") != std::string::npos ||
         function.attributes.find("hacc.no_io_alias") != std::string::npos;
}

inline bool PreCVSinglePointIsStructured(const std::string &name) {
  return IsHIVMStructuredOp(name) || name == "hivm.hir.vbrc" ||
         name == "hivm.hir.copy" || name == "hivm.hir.load" ||
         name == "hivm.hir.store";
}

inline bool PreCVSinglePointHasPureBufferSemantics(
    const GenericOperation &operation) {
  if (!PreCVSinglePointIsStructured(operation.name) ||
      !operation.results.empty() || operation.dpsInits.size() != 1)
    return false;
  for (const std::string &type : operation.operandTypes)
    if (startsWith(type, "tensor<"))
      return false;
  const auto destination = std::find(operation.operands.begin(),
                                     operation.operands.end(),
                                     operation.dpsInits.front());
  return destination != operation.operands.end() &&
         static_cast<size_t>(destination - operation.operands.begin()) <
             operation.operandTypes.size() &&
         startsWith(operation.operandTypes[static_cast<size_t>(
                        destination - operation.operands.begin())],
                    "memref<");
}

inline std::string PreCVSinglePointScalarType(const std::string &type) {
  if (const std::optional<MemRefTypeModel> memref = ParseMemRefType(type))
    return memref->elementType;
  return type;
}

class PreCVHIVMOptSinglePointDriver {
public:
  explicit PreCVHIVMOptSinglePointDriver(GenericModule &input)
      : module(input), rewriter(module) {
    indexDefinitions();
  }

  void run() {
    std::vector<int> functions;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "func.func" && !PreCVSinglePointIsHost(operation))
        functions.push_back(operation.id);
    for (int function : functions)
      runFunction(function);
    ApplyOperationSemanticsToAll(module.operations);
  }

private:
  void indexDefinitions() {
    for (const GenericBlock &block : module.blocks)
      for (int argument : block.arguments)
        blockArgumentOwners[argument] = block.id;
    for (const GenericOperation &operation : module.operations)
      for (size_t index = 0; index < operation.results.size(); ++index) {
        definitions[operation.results[index]] = operation.id;
        resultNumbers[operation.results[index]] = index;
      }
  }

  bool isAttached(int operationId) const {
    const GenericOperation &operation =
        module.operations.at(static_cast<size_t>(operationId));
    if (operation.blockId < 0)
      return false;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(operation.blockId));
    return std::find(block.operations.begin(), block.operations.end(),
                     operationId) != block.operations.end();
  }

  std::vector<int> functionPostOrder(int functionId) const {
    return CollectGreedyOperationFolderPostOrder(module, functionId);
  }

  int createBefore(int original, const std::string &name,
                   const std::vector<std::string> &resultTypes,
                   const std::vector<int> &operands,
                   const std::vector<std::string> &operandTypes,
                   const std::string &properties = "") {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(original));
    const int created = rewriter.createOperation(
        snapshot.parentId, snapshot.regionId, snapshot.blockId, name,
        resultTypes, operands, operandTypes, properties,
        properties.empty() ? "{}" : properties);
    GenericBlock &block =
        module.blocks.at(static_cast<size_t>(snapshot.blockId));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), original);
    rewriter.insertToBlock(
        snapshot.blockId,
        static_cast<size_t>(std::distance(block.operations.begin(), position)),
        created);
    const GenericOperation &record =
        module.operations.at(static_cast<size_t>(created));
    for (size_t index = 0; index < record.results.size(); ++index) {
      const int result = record.results[index];
      definitions[result] = created;
      resultNumbers[result] = index;
    }
    return created;
  }

  int createZero(int original) {
    return createBefore(original, "arith.constant", {"index"}, {}, {},
                        "{value = 0 : index}");
  }

  int createSinglePointLoad(int original, int memref,
                            const std::string &memrefType) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(memrefType);
    if (!type)
      throw std::runtime_error("single-point: load expected memref type");
    const int zero = createZero(original);
    const int zeroValue = module.operations.at(static_cast<size_t>(zero))
                              .results.front();
    std::vector<int> operands = {memref};
    std::vector<std::string> operandTypes = {memrefType};
    for (size_t dimension = 0; dimension < type->shape.size(); ++dimension) {
      operands.push_back(zeroValue);
      operandTypes.push_back("index");
    }
    return createBefore(original, "memref.load", {type->elementType},
                        operands, operandTypes,
                        "{nontemporal = false}");
  }

  void createSinglePointStore(int original, int value,
                              const std::string &valueType, int memref,
                              const std::string &memrefType) {
    const std::optional<MemRefTypeModel> type = ParseMemRefType(memrefType);
    if (!type)
      throw std::runtime_error("single-point: store expected memref type");
    const int zero = createZero(original);
    const int zeroValue = module.operations.at(static_cast<size_t>(zero))
                              .results.front();
    std::vector<int> operands = {value, memref};
    std::vector<std::string> operandTypes = {valueType, memrefType};
    for (size_t dimension = 0; dimension < type->shape.size(); ++dimension) {
      operands.push_back(zeroValue);
      operandTypes.push_back("index");
    }
    createBefore(original, "memref.store", {}, operands, operandTypes,
                 "{nontemporal = false}");
  }

  std::optional<int> scalarValue(int original, int value,
                                 const std::string &type) {
    if (!startsWith(type, "memref<"))
      return value;
    const int load = createSinglePointLoad(original, value, type);
    return module.operations.at(static_cast<size_t>(load)).results.front();
  }

  static std::string scalarOperationProperties(const std::string &name) {
    if (name == "arith.addf" || name == "arith.subf" ||
        name == "arith.mulf" || name == "arith.divf" ||
        name == "arith.maximumf" || name == "arith.minimumf" ||
        name == "math.absf" || name == "math.sqrt")
      return "{fastmath = #arith.fastmath<none>}";
    if (name == "arith.addi" || name == "arith.subi" ||
        name == "arith.muli")
      return "{overflowFlags = #arith.overflow<none>}";
    return "";
  }

  std::optional<std::string> scalarOperationName(
      const GenericOperation &operation, const std::string &elementType) const {
    const bool floating = elementType == "f32";
    const bool unsignedInteger = elementType == "ui64";
    const std::string &name = operation.name;
    if (name == "hivm.hir.vadd")
      return floating ? "arith.addf" : "arith.addi";
    if (name == "hivm.hir.vsub")
      return floating ? "arith.subf" : "arith.subi";
    if (name == "hivm.hir.vmul")
      return floating ? "arith.mulf" : "arith.muli";
    if (name == "hivm.hir.vdiv")
      return floating ? "arith.divf"
                      : (unsignedInteger ? "arith.divui" : "arith.divsi");
    if (name == "hivm.hir.vabs")
      return floating ? "math.absf" : "math.absi";
    if (name == "hivm.hir.vsqrt")
      return "math.sqrt";
    if (name == "hivm.hir.vmax")
      return floating ? "arith.maximumf"
                      : (unsignedInteger ? "arith.maxui" : "arith.maxsi");
    if (name == "hivm.hir.vmin")
      return floating ? "arith.minimumf"
                      : (unsignedInteger ? "arith.minui" : "arith.minsi");
    return std::nullopt;
  }

  bool rewriteElementwise(int operationId) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    const std::optional<std::string> firstType =
        snapshot.operandTypes.empty()
            ? std::nullopt
            : std::optional<std::string>(
                  PreCVSinglePointScalarType(snapshot.operandTypes.front()));
    if (!firstType || (*firstType != "f32" && *firstType != "i64" &&
                       *firstType != "ui64") ||
        !PreCVSinglePointHasPureBufferSemantics(snapshot) ||
        snapshot.dpsInits.size() != 1)
      return false;
    const auto destination = std::find(snapshot.operands.begin(),
                                       snapshot.operands.end(),
                                       snapshot.dpsInits.front());
    const size_t destinationIndex = static_cast<size_t>(
        destination - snapshot.operands.begin());
    if (destination == snapshot.operands.end() ||
        destinationIndex >= snapshot.operandTypes.size() ||
        !PreCVSinglePointAllOnes(snapshot.operandTypes[destinationIndex]))
      return false;
    const std::optional<std::string> scalarName =
        scalarOperationName(snapshot, *firstType);
    if (!scalarName)
      return false;
    // The native pattern selects arith.maxui/minui for unsigned i64, but
    // those scalar operations require signless integer operands.  Native
    // applyPatternsGreedily therefore leaves an invalid IR and the pass
    // manager fails verification.  Preserve that fail-open boundary instead
    // of producing a successful lightweight UB decision.
    if (*firstType == "ui64" &&
        (snapshot.name == "hivm.hir.vmax" ||
         snapshot.name == "hivm.hir.vmin"))
      throw std::runtime_error(
          "single-point: native unsigned i64 max/min scalarization is "
          "rejected by the arith verifier");
    std::vector<int> scalarInputs;
    std::vector<std::string> scalarInputTypes;
    for (size_t index = 0; index < snapshot.operands.size(); ++index) {
      if (index == destinationIndex)
        continue;
      const std::optional<int> scalar = scalarValue(
          operationId, snapshot.operands[index], snapshot.operandTypes[index]);
      if (!scalar)
        return false;
      scalarInputs.push_back(*scalar);
      scalarInputTypes.push_back(
          PreCVSinglePointScalarType(snapshot.operandTypes[index]));
    }
    const int scalar = createBefore(
        operationId, *scalarName, {*firstType}, scalarInputs,
        scalarInputTypes, scalarOperationProperties(*scalarName));
    createSinglePointStore(
        operationId,
        module.operations.at(static_cast<size_t>(scalar)).results.front(),
        *firstType, snapshot.operands[destinationIndex],
        snapshot.operandTypes[destinationIndex]);
    rewriter.removeFromBlock(snapshot.blockId, snapshot.id);
    return true;
  }

  bool rewriteVBrc(int operationId) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (!PreCVSinglePointHasPureBufferSemantics(snapshot) ||
        snapshot.operands.size() < 2 || snapshot.operandTypes.size() < 2 ||
        !PreCVSinglePointAllOnes(snapshot.operandTypes[1]))
      return false;
    const std::optional<int> source = scalarValue(
        operationId, snapshot.operands[0], snapshot.operandTypes[0]);
    if (!source)
      return false;
    const std::string scalarType =
        PreCVSinglePointScalarType(snapshot.operandTypes[0]);
    createSinglePointStore(operationId, *source, scalarType,
                           snapshot.operands[1], snapshot.operandTypes[1]);
    rewriter.removeFromBlock(snapshot.blockId, snapshot.id);
    return true;
  }

  bool isAllocLikeValue(int value) const {
    const auto found = definitions.find(value);
    if (found == definitions.end())
      return false;
    const std::string &name = module.operations.at(
        static_cast<size_t>(found->second)).name;
    return name == "memref.alloc" || name == "memref.alloca" ||
           name == "memref_ext.alloc_workspace";
  }

  std::optional<int> regionYield(int regionId, size_t index,
                                 const std::string &terminatorName,
                                 size_t operandOffset = 0) const {
    if (regionId < 0)
      return std::nullopt;
    const GenericRegion &region =
        module.regions.at(static_cast<size_t>(regionId));
    if (region.blocks.empty())
      return std::nullopt;
    const GenericBlock &block =
        module.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (block.operations.empty())
      return std::nullopt;
    const GenericOperation &terminator = module.operations.at(
        static_cast<size_t>(block.operations.back()));
    const size_t operandIndex = index + operandOffset;
    if (terminator.name != terminatorName ||
        operandIndex >= terminator.operands.size())
      return std::nullopt;
    return terminator.operands[operandIndex];
  }

  std::vector<int> tracebackStep(int value) const {
    const auto argumentOwner = blockArgumentOwners.find(value);
    if (argumentOwner != blockArgumentOwners.end()) {
      const GenericBlock &block = module.blocks.at(
          static_cast<size_t>(argumentOwner->second));
      const auto argument =
          std::find(block.arguments.begin(), block.arguments.end(), value);
      if (argument == block.arguments.end())
        return {};
      const size_t argumentIndex = static_cast<size_t>(
          std::distance(block.arguments.begin(), argument));
      const GenericRegion &region =
          module.regions.at(static_cast<size_t>(block.regionId));
      const GenericOperation &owner = module.operations.at(
          static_cast<size_t>(region.parentOperation));
      if (owner.name == "scf.for" && argumentIndex > 0) {
        const size_t channel = argumentIndex - 1;
        const size_t initIndex = channel + 3;
        const std::optional<int> yielded =
            regionYield(block.regionId, channel, "scf.yield");
        if (initIndex < owner.operands.size() && yielded)
          return {owner.operands[initIndex], *yielded};
      }
      if (owner.name == "scf.while" && !owner.regions.empty() &&
          owner.regions.front() == block.regionId &&
          argumentIndex < owner.operands.size())
        return {owner.operands[argumentIndex]};
      return {};
    }

    const auto definition = definitions.find(value);
    if (definition == definitions.end())
      return {};
    const GenericOperation &operation = module.operations.at(
        static_cast<size_t>(definition->second));
    const size_t resultIndex = resultNumbers.at(value);
    if (operation.name == "builtin.unrealized_conversion_cast")
      return resultIndex < operation.operands.size()
                 ? std::vector<int>{operation.operands[resultIndex]}
                 : std::vector<int>{};
    static const std::set<std::string> castLike = {
        "memref.cast", "memref.collapse_shape", "memref.expand_shape",
        "memref.memory_space_cast", "memref.reinterpret_cast",
        "memref.reshape", "memref.transpose"};
    if (castLike.count(operation.name) != 0 && !operation.operands.empty())
      return {operation.operands.front()};
    if ((operation.name == "memref.view" ||
         operation.name == "memref.subview") &&
        !operation.operands.empty())
      return {operation.operands.front()};
    if (operation.name == "scf.for") {
      const size_t initIndex = resultIndex + 3;
      const std::optional<int> yielded =
          operation.regions.empty()
              ? std::nullopt
              : regionYield(operation.regions.front(), resultIndex,
                            "scf.yield");
      if (initIndex < operation.operands.size() && yielded)
        return {operation.operands[initIndex], *yielded};
      return {};
    }
    if (operation.name == "scf.if" && operation.regions.size() == 2) {
      const std::optional<int> thenValue =
          regionYield(operation.regions[0], resultIndex, "scf.yield");
      const std::optional<int> elseValue =
          regionYield(operation.regions[1], resultIndex, "scf.yield");
      if (thenValue && elseValue)
        return {*thenValue, *elseValue};
    }
    return {};
  }

  int tracebackMemRef(int value) const {
    std::vector<int> values = {value};
    int remaining = 257;
    while (!values.empty() && remaining-- > 0) {
      const auto pending = std::find_if(
          values.begin(), values.end(),
          [&](int candidate) { return !isAllocLikeValue(candidate); });
      if (pending == values.end())
        break;
      const std::vector<int> upward = tracebackStep(*pending);
      if (upward.empty())
        break;
      const size_t position = static_cast<size_t>(
          std::distance(values.begin(), pending));
      values.erase(values.begin() + static_cast<std::ptrdiff_t>(position));
      values.insert(values.end(), upward.begin(), upward.end());
    }
    return values.empty() ? value : values.front();
  }

  bool allMemoryUsersValid(int root) const {
    std::queue<int> worklist;
    std::set<int> visited;
    worklist.push(root);
    visited.insert(root);
    while (!worklist.empty()) {
      const int current = worklist.front();
      worklist.pop();
      for (const GenericOperation &user : module.operations) {
        if (!isAttached(user.id))
          continue;
        for (size_t operandIndex = 0; operandIndex < user.operands.size();
             ++operandIndex) {
          if (user.operands[operandIndex] != current)
            continue;
          if (user.name == "memref.cast" ||
              user.name == "memref.subview" ||
              user.name == "memref.collapse_shape" ||
              user.name == "memref.expand_shape") {
            for (int result : user.results)
              if (visited.insert(result).second)
                worklist.push(result);
            continue;
          }
          if (user.name == "memref.load")
            continue;
          if (!PreCVSinglePointIsStructured(user.name))
            return false;
          const std::vector<size_t> destinations = DpsInitOperandIndices(
              user.name, user.operands.size(), user.properties);
          if (std::find(destinations.begin(), destinations.end(),
                        operandIndex) != destinations.end())
            return false;
        }
      }
    }
    return true;
  }

  bool rewriteCopyLike(int functionId, int operationId) {
    const GenericOperation snapshot =
        module.operations.at(static_cast<size_t>(operationId));
    if (!PreCVSinglePointHasPureBufferSemantics(snapshot) ||
        snapshot.operands.size() < 2 || snapshot.operandTypes.size() < 2 ||
        (snapshot.name != "hivm.hir.copy" &&
         snapshot.name != "hivm.hir.load"))
      return false;
    if (snapshot.name == "hivm.hir.load" &&
        !PreCVSinglePointHasNoIOAlias(
            module.operations.at(static_cast<size_t>(functionId))))
      return false;
    const std::optional<MemRefTypeModel> sourceType =
        ParseMemRefType(snapshot.operandTypes[0]);
    const std::optional<MemRefTypeModel> destinationType =
        ParseMemRefType(snapshot.operandTypes[1]);
    if (!sourceType || !destinationType ||
        sourceType->addressSpace == AddressSpace::Unknown ||
        destinationType->addressSpace == AddressSpace::Unknown ||
        !PreCVSinglePointAllOnes(snapshot.operandTypes[1]))
      return false;
    if (!allMemoryUsersValid(tracebackMemRef(snapshot.operands[0])))
      return false;
    const int load = createSinglePointLoad(operationId, snapshot.operands[0],
                                           snapshot.operandTypes[0]);
    const int scalar =
        module.operations.at(static_cast<size_t>(load)).results.front();
    createSinglePointStore(operationId, scalar, sourceType->elementType,
                           snapshot.operands[1], snapshot.operandTypes[1]);
    rewriter.removeFromBlock(snapshot.blockId, snapshot.id);
    return true;
  }

  void runFunction(int functionId) {
    const std::vector<int> order = functionPostOrder(functionId);
    static const std::set<std::string> elementwise = {
        "hivm.hir.vabs", "hivm.hir.vadd",  "hivm.hir.vdiv",
        "hivm.hir.vmax", "hivm.hir.vmin",  "hivm.hir.vmul",
        "hivm.hir.vsqrt", "hivm.hir.vsub"};
    for (int operationId : order) {
      if (!isAttached(operationId))
        continue;
      const std::string name =
          module.operations.at(static_cast<size_t>(operationId)).name;
      if (name == "hivm.hir.vbrc")
        rewriteVBrc(operationId);
      else if (name == "hivm.hir.copy" || name == "hivm.hir.load")
        rewriteCopyLike(functionId, operationId);
      else if (elementwise.count(name) != 0)
        rewriteElementwise(operationId);
    }
    RunGreedyOperationFolder(module, functionId);
  }

  GenericModule &module;
  GenericRewriter rewriter;
  std::map<int, int> definitions;
  std::map<int, size_t> resultNumbers;
  std::map<int, int> blockArgumentOwners;
};

// Stage entry for the pre-CVPipelining pure-buffer boundary.  This is a
// rewrite of GenericModule itself; the later BufferizedSemanticIR
// HIVMOptSinglePointModel is analysis-only and is deliberately not reused.
inline GenericModule RunPreCVHIVMOptSinglePoint(GenericModule module) {
  ApplyOperationSemanticsToAll(module.operations);
  PreCVHIVMOptSinglePointDriver(module).run();
  return CompactGenericModule(std::move(module));
}

} // namespace cvub

#endif
