#ifndef UB_OVERFLOW_MODEL_CPP_MLIR_MODULE_VIEW_HPP
#define UB_OVERFLOW_MODEL_CPP_MLIR_MODULE_VIEW_HPP

#include "generic_ir.hpp"
#include "stable_id.hpp"

#include "mlir/IR/Attributes.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/Types.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cvub {

// Read-only, call-scoped view over the ModuleOp passed by BiSheng. IDs follow
// the original region/block/operation order and remain stable for the entire
// synchronous model evaluation. The view never mutates or owns the MLIR IR.
class MLIRModuleView {
public:
  struct MaterializedModule {
    GenericModule module;
    std::string structuralDigest;
  };

  struct OperationRecord {
    mlir::Operation *base = nullptr;
    OpId id;
    OpId parent;
    RegionId region;
    BlockId block;
    int ordinal = 0;
    bool active = true;
    mlir::OperationName name;
    mlir::Attribute properties;
    mlir::DictionaryAttr attributes;
  };

  struct BlockRecord {
    mlir::Block *base = nullptr;
    BlockId id;
    RegionId region;
    int ordinal = 0;
    bool active = true;
  };

  struct RegionRecord {
    mlir::Region *base = nullptr;
    RegionId id;
    OpId parentOperation;
    int ordinal = 0;
    bool active = true;
  };

  explicit MLIRModuleView(mlir::ModuleOp module) { build(module); }

  const std::vector<OperationRecord> &operations() const {
    return operationRecords;
  }
  const std::vector<BlockRecord> &blocks() const { return blockRecords; }
  const std::vector<RegionRecord> &regions() const { return regionRecords; }

  MaterializedModule materializeLegacyGenericModule() const {
    MaterializedModule materialized;
    GenericModule &result = materialized.module;
    result.operations.reserve(operationRecords.size());
    result.regions.reserve(regionRecords.size());
    result.blocks.reserve(blockRecords.size());
    llvm::DenseMap<mlir::Type, std::string> typeText;
    llvm::DenseMap<mlir::Attribute, CowString> attributeText;
    uint64_t hash = 14695981039346656037ULL;
    auto mix = [&](llvm::StringRef value) {
      for (char raw : value) {
        hash ^= static_cast<unsigned char>(raw);
        hash *= 1099511628211ULL;
      }
      hash ^= 0xffU;
      hash *= 1099511628211ULL;
    };

    auto printType = [&](mlir::Type type) -> const std::string & {
      auto found = typeText.find(type);
      if (found != typeText.end())
        return found->second;
      std::string text;
      llvm::raw_string_ostream output(text);
      type.print(output);
      output.flush();
      return typeText.try_emplace(type, std::move(text)).first->second;
    };
    auto deferredAttribute = [&](mlir::Attribute attribute)
        -> const CowString & {
      static const CowString empty;
      if (!attribute)
        return empty;
      auto found = attributeText.find(attribute);
      if (found != attributeText.end())
        return found->second;
      CowString text = CowString::Deferred([attribute] {
        std::string result;
        llvm::raw_string_ostream output(result);
        attribute.print(output);
        output.flush();
        return result;
      });
      return attributeText.try_emplace(attribute, std::move(text))
          .first->second;
    };

    for (const OperationRecord &view : operationRecords) {
      mlir::Operation *source = view.base;
      const llvm::hash_code operationHash =
          mlir::OperationEquivalence::computeHash(
              source,
              [&](mlir::Value value) {
                return llvm::hash_value(valueId(value));
              },
              [&](mlir::Value value) {
                return llvm::hash_value(valueId(value));
              },
              mlir::OperationEquivalence::IgnoreLocations);
      const uint64_t rawOperationHash =
          static_cast<uint64_t>(static_cast<size_t>(operationHash));
      for (unsigned shift = 0; shift != 64; shift += 8) {
        hash ^= static_cast<unsigned char>(rawOperationHash >> shift);
        hash *= 1099511628211ULL;
      }
      GenericOperation operation;
      operation.id = toLegacy(view.id);
      operation.projectionSourceId = operation.id;
      operation.parentId = toLegacy(view.parent);
      operation.regionId = toLegacy(view.region);
      operation.blockId = toLegacy(view.block);
      operation.ordinal = view.ordinal;
      operation.name = view.name.getStringRef().str();
      mix(view.name.getStringRef());
      operation.results.reserve(source->getNumResults());
      operation.resultTypes.reserve(source->getNumResults());
      for (mlir::Value value : source->getResults()) {
        operation.results.push_back(valueId(value));
        operation.resultTypes.push_back(printType(value.getType()));
      }
      operation.operands.reserve(source->getNumOperands());
      operation.operandTypes.reserve(source->getNumOperands());
      for (mlir::Value value : source->getOperands()) {
        operation.operands.push_back(valueId(value));
        operation.operandTypes.push_back(printType(value.getType()));
      }
      operation.properties = deferredAttribute(view.properties);
      mlir::NamedAttrList mergedAttributes;
      if (auto properties =
              llvm::dyn_cast_if_present<mlir::DictionaryAttr>(view.properties))
        mergedAttributes.append(properties.getValue());
      for (mlir::NamedAttribute attribute : view.attributes)
        mergedAttributes.set(attribute.getName(), attribute.getValue());
      operation.attributes = deferredAttribute(
          mergedAttributes.getDictionary(source->getContext()));
      operation.successors.reserve(source->getNumSuccessors());
      for (mlir::Block *successor : source->getSuccessors()) {
        auto found = blockIds.find(successor);
        if (found == blockIds.end())
          throw std::runtime_error(
              "MLIRModuleView: successor block is outside the module view");
        operation.successors.push_back(toLegacy(found->second));
      }
      operation.regions.reserve(source->getNumRegions());
      for (mlir::Region &region : source->getRegions())
        operation.regions.push_back(toLegacy(regionIds.lookup(&region)));
      result.operations.push_back(std::move(operation));
    }

    for (const RegionRecord &view : regionRecords) {
      GenericRegion region;
      region.id = toLegacy(view.id);
      region.parentOperation = toLegacy(view.parentOperation);
      region.ordinal = view.ordinal;
      region.blocks.reserve(view.base->getBlocks().size());
      for (mlir::Block &block : *view.base)
        region.blocks.push_back(toLegacy(blockIds.lookup(&block)));
      result.regions.push_back(std::move(region));
    }

    for (const BlockRecord &view : blockRecords) {
      GenericBlock block;
      block.id = toLegacy(view.id);
      block.regionId = toLegacy(view.region);
      block.ordinal = view.ordinal;
      block.arguments.reserve(view.base->getNumArguments());
      block.argumentTypes.reserve(view.base->getNumArguments());
      for (mlir::BlockArgument argument : view.base->getArguments()) {
        block.arguments.push_back(valueId(argument));
        block.argumentTypes.push_back(printType(argument.getType()));
      }
      block.operations.reserve(view.base->getOperations().size());
      for (mlir::Operation &operation : *view.base)
        block.operations.push_back(toLegacy(operationIds.lookup(&operation)));
      result.blocks.push_back(std::move(block));
    }

    for (const GenericOperation &operation : result.operations)
      ValidateGenericOperationTypeContract(operation);
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << hash;
    materialized.structuralDigest = output.str();
    return materialized;
  }

private:
  template <typename Id> static int toLegacy(Id id) {
    return id ? static_cast<int>(id.raw()) : -1;
  }

  int valueId(mlir::Value value) const {
    auto found = valueIds.find(value);
    if (found == valueIds.end())
      throw std::runtime_error(
          "MLIRModuleView: operand value is outside the module view");
    return toLegacy(found->second);
  }

  void build(mlir::ModuleOp module) {
    if (!module)
      throw std::runtime_error("MLIRModuleView: null ModuleOp");
    addOperation(module.getOperation(), OpId(), RegionId(), BlockId(), 0);
  }

  OpId addOperation(mlir::Operation *operation, OpId parent, RegionId region,
                    BlockId block, int ordinal) {
    const OpId id = OpId::fromIndex(operationRecords.size());
    operationIds.try_emplace(operation, id);
    for (mlir::Value result : operation->getResults())
      valueIds.try_emplace(result, ValueId::fromIndex(nextValue++));
    operationRecords.push_back({
        operation, id, parent, region, block, ordinal, true,
        operation->getName(),
        operation->getPropertiesAsAttribute(), operation->getAttrDictionary(),
    });

    int regionOrdinal = 0;
    for (mlir::Region &sourceRegion : operation->getRegions()) {
      const RegionId regionId = RegionId::fromIndex(regionRecords.size());
      regionIds.try_emplace(&sourceRegion, regionId);
      regionRecords.push_back(
          {&sourceRegion, regionId, id, regionOrdinal++, true});

      int blockOrdinal = 0;
      for (mlir::Block &sourceBlock : sourceRegion) {
        const BlockId blockId = BlockId::fromIndex(blockRecords.size());
        blockIds.try_emplace(&sourceBlock, blockId);
        blockRecords.push_back(
            {&sourceBlock, blockId, regionId, blockOrdinal++, true});
        for (mlir::BlockArgument argument : sourceBlock.getArguments())
          valueIds.try_emplace(argument, ValueId::fromIndex(nextValue++));
      }
      for (mlir::Block &sourceBlock : sourceRegion) {
        int operationOrdinal = 0;
        const BlockId blockId = blockIds.lookup(&sourceBlock);
        for (mlir::Operation &child : sourceBlock)
          addOperation(&child, id, regionId, blockId, operationOrdinal++);
      }
    }
    return id;
  }

  std::vector<OperationRecord> operationRecords;
  std::vector<BlockRecord> blockRecords;
  std::vector<RegionRecord> regionRecords;
  llvm::DenseMap<mlir::Operation *, OpId> operationIds;
  llvm::DenseMap<mlir::Region *, RegionId> regionIds;
  llvm::DenseMap<mlir::Block *, BlockId> blockIds;
  llvm::DenseMap<mlir::Value, ValueId> valueIds;
  size_t nextValue = 0;
};

} // namespace cvub

#endif
