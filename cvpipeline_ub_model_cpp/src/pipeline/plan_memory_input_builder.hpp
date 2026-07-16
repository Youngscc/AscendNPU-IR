#ifndef CVPIPELINE_UB_MODEL_CPP_PLAN_MEMORY_INPUT_BUILDER_HPP
#define CVPIPELINE_UB_MODEL_CPP_PLAN_MEMORY_INPUT_BUILDER_HPP

#include "../passes/affine_min_max_canonicalization.hpp"
#include "plan_memory_input_semantic_ir.hpp"
#include "../passes/plan_memory/plan_memory_model.hpp"

namespace cvub {

inline std::string PlanMemoryBufferName(std::string identity) {
  for (char &character : identity)
    if (!std::isalnum(static_cast<unsigned char>(character)))
      character = '_';
  return "%" + identity;
}

inline std::string PlanMemoryValueName(int value) {
  return "%v" + std::to_string(value);
}

inline std::string PlanMemoryMemRefType(const std::string &signature) {
  const size_t shape = signature.find("|shape=");
  const size_t strides = signature.find("|strides=", shape);
  const size_t offset = signature.find("|offset=", strides);
  if (shape == std::string::npos || strides == std::string::npos ||
      offset == std::string::npos)
    throw std::runtime_error("PlanMemory input builder: malformed physical type");
  const std::string element = signature.substr(0, shape);
  const std::vector<std::string> dimensions = split(
      signature.substr(shape + 7, strides - shape - 7), ',');
  const std::vector<std::string> strideValues = split(
      signature.substr(strides + 9, offset - strides - 9), ',');
  const std::string offsetValue = signature.substr(offset + 8);
  std::ostringstream type;
  type << "memref<";
  for (const std::string &dimension : dimensions)
    type << dimension << 'x';
  type << element;
  if (!strideValues.empty()) {
    type << ", strided<[";
    for (size_t index = 0; index < strideValues.size(); ++index) {
      if (index)
        type << ", ";
      type << strideValues[index];
    }
    type << "], offset: " << offsetValue << ">";
  }
  type << ", #hivm.address_space<ub>>";
  return type.str();
}

inline std::string PlanMemoryPreserveAddressSpace(std::string type,
                                                  const std::string &source) {
  const size_t address = source.find("#hivm.address_space<");
  if (address == std::string::npos || type.empty() || type.back() != '>')
    return type;
  const size_t addressEnd = source.find('>', address);
  if (addressEnd == std::string::npos)
    return type;
  type.insert(type.size() - 1,
              ", " + source.substr(address, addressEnd - address + 1));
  return type;
}

inline std::string PlanMemoryFormatMemRefType(const MemRefTypeModel &type) {
  std::ostringstream result;
  result << "memref<";
  for (const std::optional<int64_t> &extent : type.shape)
    result << (extent ? std::to_string(*extent) : "?") << 'x';
  result << type.elementType << ", strided<[";
  for (size_t index = 0; index < type.strides.size(); ++index) {
    if (index)
      result << ", ";
    result << (type.strides[index] ? std::to_string(*type.strides[index])
                                   : "?");
  }
  result << "], offset: ";
  if (type.offset == std::numeric_limits<int64_t>::min())
    result << '?';
  else
    result << type.offset;
  result << ">>";
  return result.str();
}

inline std::string PlanMemoryFormatMemRefTypeWithAddressSpace(
    const MemRefTypeModel &type) {
  const std::string scope = AddressSpaceName(type.addressSpace);
  if (scope == "unknown")
    return PlanMemoryFormatMemRefType(type);
  return PlanMemoryPreserveAddressSpace(
      PlanMemoryFormatMemRefType(type),
      "memref<i8, #hivm.address_space<" + scope + ">>");
}

class PlanMemoryInputBuilder {
public:
  explicit PlanMemoryInputBuilder(const PlanMemoryInputSemanticIR &inputModule,
                                  std::string targetFunction = {})
      : module(inputModule), targetFunction(std::move(targetFunction)),
        logical(inputModule.afterMarkMultiBuffer.afterInlineLoadCopy
                    .afterAllocExtraBuffer.postBufferization.bufferized
                    .logicalModule) {
    const PreBufferizationCSEState &preBufferizationCSE =
        inputModule.afterMarkMultiBuffer.afterInlineLoadCopy
            .afterAllocExtraBuffer.postBufferization.bufferized
            .preBufferizationCSE;
    erasedOperations.insert(preBufferizationCSE.erasedOperations.begin(),
                            preBufferizationCSE.erasedOperations.end());
    indexBuffers();
    indexValues();
    arithToAffine = RunConvertArithToAffine(logical);
    RunAffineMinMaxCanonicalization(logical, arithToAffine);
    RunAffineCanonicalizationCSE(logical, arithToAffine);
    erasedOperations.insert(arithToAffine.erasedOperations.begin(),
                            arithToAffine.erasedOperations.end());
    indexFoldedAffineValues();
    indexFoldedAffineConstants();
    indexEvents();
    indexDeadConstantsAfterHIVMDecomposeOp();
    indexMappedLoops();
  }

  PlanMemoryInput Build() {
    if (!module.accessBlockers.empty())
      throw std::runtime_error("PlanMemory input builder: unresolved access blocker");
    const GenericOperation *function = nullptr;
    for (const GenericOperation &operation : logical.operations) {
      if (operation.name != "func.func" || !IsAIVFunction(operation))
        continue;
      if (!targetFunction.empty() &&
          GenericFunctionName(operation) != targetFunction)
        continue;
      if (function)
        throw std::runtime_error(
            "PlanMemory input builder: multiple AIV functions are not supported");
      function = &operation;
    }
    if (!function && !targetFunction.empty())
      throw std::runtime_error("PlanMemory input builder: unknown AIV function " +
                               targetFunction);
    if (!function)
      return {};
    if (function->regions.size() != 1)
      throw std::runtime_error("PlanMemory input builder: malformed AIV function");
    const GenericRegion &region =
        logical.regions.at(static_cast<size_t>(function->regions.front()));
    emitGeneratedFunctionConstants(*function, region);
    for (int block : region.blocks)
      emitBlock(logical.blocks.at(static_cast<size_t>(block)));
    finalizeOperations();
    std::set<std::string> allocNames;
    std::map<std::string, std::string> allocTypes;
    for (const IRAllocRecord &allocation : result.allocations) {
      allocNames.insert(allocation.name);
      allocTypes[allocation.name] = allocation.memrefType;
    }
    result.operations = ApplyPlanMemoryNormalizePatterns(
        NormalizeIterUseAfterYieldInit(result.operations, allocNames,
                                       allocTypes));
    result.functionArguments = functionArguments;
    return result;
  }

private:
  void emitGeneratedFunctionConstants(const GenericOperation &function,
                                      const GenericRegion &functionRegion) {
    if (functionRegion.blocks.empty())
      return;
    const GenericBlock *entry = nullptr;
    for (int blockId : functionRegion.blocks) {
      const GenericBlock &candidate =
          logical.blocks.at(static_cast<size_t>(blockId));
      if (!candidate.operations.empty()) {
        entry = &candidate;
        break;
      }
    }
    if (!entry)
      return;
    const GenericOperation *placement = nullptr;
    if (!entry->operations.empty())
      placement = &logical.operations.at(
          static_cast<size_t>(entry->operations.front()));
    if (!placement)
      return;

    auto emitConstant = [&](const std::string &key, const std::string &type,
                            const std::string &literal) -> std::string {
      auto existing = generatedFunctionConstants[function.id].find(key);
      if (existing != generatedFunctionConstants[function.id].end())
        return existing->second;
      const std::string name =
          "%decompose_constant_" + std::to_string(nextScalarValue++);
      OperationRecord constant = baseOperation(*placement, "arith.constant");
      constant.blockId = entry->id;
      constant.blockLabel = "^bb" + std::to_string(entry->id);
      constant.text = name + " = arith.constant " + literal + " : " + type;
      result.operations.push_back(std::move(constant));
      namedValueTypes[name] = type;
      generatedFunctionConstants[function.id][key] = name;
      return name;
    };

    struct ExtendedConstant {
      bool isUnsigned = false;
      int operand = -1;
      std::string literal;
    };
    std::vector<ExtendedConstant> extendedConstants;
    std::set<std::pair<bool, int>> seenExtendedConstants;
    for (const GenericOperation &operation : logical.operations) {
      auto events = eventsBySource.find(operation.id);
      const bool decomposedScalarVSub =
          events != eventsBySource.end() &&
          std::any_of(events->second.begin(), events->second.end(),
                      [](const PlanMemoryInputAccessRecord *event) {
                        return event->decomposedScalarVSub;
                      });
      if (!decomposedScalarVSub || operation.operands.size() < 2 ||
          operation.operandTypes.size() < 2)
        continue;
      const GenericOperation *definition =
          DefiningOperation(logical, operation.operands[1]);
      if (!definition || definition->name != "arith.constant")
        continue;
      std::string literal =
          FindDictionaryValue(definition->properties, "value");
      if (literal.empty())
        literal = FindDictionaryValue(definition->attributes, "value");
      const size_t typedSuffix = literal.find(" : ");
      if (typedSuffix != std::string::npos)
        literal = trim(literal.substr(0, typedSuffix));
      try {
        const std::string &type = operation.operandTypes[1];
        std::string negated;
        if (!type.empty() && type.front() == 'i') {
          const int64_t integer = std::stoll(literal, nullptr, 0);
          if (integer == 0)
            continue;
          const unsigned width =
              static_cast<unsigned>(std::stoul(type.substr(1)));
          if (width <= 64) {
            const uint64_t mask =
                width == 64 ? std::numeric_limits<uint64_t>::max()
                            : (uint64_t{1} << width) - 1;
            const uint64_t value = static_cast<uint64_t>(integer) & mask;
            const uint64_t negatedValue = (uint64_t{0} - value) & mask;
            auto typedConstants = integerScalarConstants.find(function.id);
            if (typedConstants != integerScalarConstants.end()) {
              auto typed = typedConstants->second.find(type);
              if (typed != typedConstants->second.end() &&
                  typed->second.count(negatedValue) != 0)
                continue;
            }
          }
          negated = std::to_string(-integer);
        } else {
          const long double floating = std::stold(literal);
          if (floating == 0.0L)
            continue;
          negated = std::to_string(-floating);
        }
        generatedVSubNegatedConstants[operation.id] = emitConstant(
            "vsub-negated:" + std::to_string(operation.id), type, negated);
      } catch (const std::exception &) {
      }
    }
    for (const GenericOperation &operation : logical.operations) {
      const GenericOperation *owner =
          EnclosingFunction(logical, operation);
      if (!owner || owner->id != function.id ||
          (operation.name != "arith.mului_extended" &&
           operation.name != "arith.mulsi_extended") ||
          operation.operandTypes.empty() ||
          operation.operandTypes.front() != "i32")
        continue;
      const bool isUnsigned = operation.name == "arith.mului_extended";
      for (int operand : operation.operands) {
        const GenericOperation *definition =
            DefiningOperation(logical, operand);
        if (!definition || definition->name != "arith.constant" ||
            !seenExtendedConstants.insert({isUnsigned, operand}).second)
          continue;
        std::string literal =
            FindDictionaryValue(definition->properties, "value");
        if (literal.empty())
          literal = FindDictionaryValue(definition->attributes, "value");
        const size_t typedSuffix = literal.find(" : ");
        if (typedSuffix != std::string::npos)
          literal = trim(literal.substr(0, typedSuffix));
        if (isUnsigned) {
          const int64_t signedValue = std::stoll(literal, nullptr, 0);
          literal = std::to_string(
              static_cast<uint64_t>(static_cast<uint32_t>(signedValue)));
        }
        extendedConstants.push_back({isUnsigned, operand, literal});
      }
    }
    auto emitExtendedConstant = [&](const ExtendedConstant &constant) {
      emitConstant((constant.isUnsigned ? "extui:" : "extsi:") +
                       std::to_string(constant.operand),
                   "i64", constant.literal);
    };
    if (!extendedConstants.empty()) {
      emitExtendedConstant(extendedConstants.back());
      emitConstant("extended-multiply-shift", "i64", "32");
      for (size_t index = extendedConstants.size() - 1; index > 0; --index)
        emitExtendedConstant(extendedConstants[index - 1]);
    }

    for (const GenericOperation &operation : logical.operations) {
      const GenericOperation *owner =
          EnclosingFunction(logical, operation);
      if (!owner || owner->id != function.id)
        continue;
      if (operation.name == "hivm.hir.vrec" &&
          !operation.operandTypes.empty()) {
        const std::string type = ShapedElementType(operation.operandTypes[0]);
        if (!type.empty())
          emitConstant("one:" + type, type, "1");
      }
      if (operation.name == "hivm.hir.vcast" &&
          operation.operandTypes.size() >= 2) {
        const std::string sourceType =
            ShapedElementType(operation.operandTypes.front());
        const std::string destinationType =
            ShapedElementType(operation.operandTypes.back());
        if ((sourceType == "i8" || sourceType == "i16") &&
            destinationType == "i1")
          emitConstant("zero:f16", "f16", "0.0");
        else if (sourceType == "i32" && destinationType == "i1")
          emitConstant("zero:f32", "f32", "0.0");
      }
      if (operation.name == "hivm.hir.vcmp" &&
          operation.operandTypes.size() >= 2) {
        const std::string sourceType =
            ShapedElementType(operation.operandTypes.front());
        const std::string destinationType =
            ShapedElementType(operation.operandTypes.back());
        const std::string compareMode = DecomposeEnumValue(
            FindDictionaryValue(operation.properties, "compare_mode"));
        if (!sourceType.empty() && sourceType.front() == 'i' &&
            destinationType == "i1" &&
            !(sourceType == "i32" &&
              (compareMode == "eq" || compareMode == "ne")))
          emitConstant("zero:f16", "f16", "0.0");
      }
      if (operation.name != "hivm.hir.vsub" ||
          operation.operandTypes.size() < 2 ||
          IsTensorType(operation.operandTypes[1]) ||
          IsMemRefType(operation.operandTypes[1]))
        continue;
      const int scalar = operation.operands.at(1);
      const GenericOperation *definition =
          DefiningOperation(logical, scalar);
      if (definition && definition->name == "arith.constant")
        continue;
      const std::string &type = operation.operandTypes[1];
      if (zeroScalarConstants[function.id].count(type) == 0)
        generatedZeroNames[function.id][type] =
            emitConstant("zero:" + type, type, "0");
    }
  }

  void indexBuffers() {
    std::set<size_t> targetOrdinals;
    for (const PlanMemoryInputBufferRecord &buffer : module.buffers) {
      targetOrdinals.insert(buffer.globalOrdinal);
      finalIdentity[buffer.sourceIdentity] = buffer.identity;
      targetSourceIdentities.insert(buffer.sourceIdentity);
      const std::string name = PlanMemoryBufferName(buffer.identity);
      bufferNames[buffer.identity] = name;
      bufferTypes[buffer.identity] = PlanMemoryMemRefType(buffer.physicalType);
      namedValueTypes[name] = bufferTypes[buffer.identity];
      if (const LocalBufferRecord *record =
              FindSourceBuffer(module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.buffers,
                                 buffer.sourceIdentity))
        finalBufferRecords[buffer.identity] = record;
      const AfterAllocExtraBufferState &afterAllocExtraBuffer = module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer;
      auto owner = afterAllocExtraBuffer.bufferOwnerOperations.find(buffer.sourceIdentity);
      if (owner == afterAllocExtraBuffer.bufferOwnerOperations.end())
        throw std::runtime_error("PlanMemory input builder: missing buffer owner");
      buffersByOwner[owner->second].push_back(&buffer);
      bufferByIdentity[buffer.identity] = &buffer;
    }
    const AfterAllocExtraBufferState &afterAllocExtraBuffer = module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer;
    for (size_t ordinal = 0; ordinal < module.afterMarkMultiBuffer.afterInlineLoadCopy.buffers.size(); ++ordinal) {
      if (targetOrdinals.count(ordinal))
        continue;
      const LocalBufferRecord &buffer = module.afterMarkMultiBuffer.afterInlineLoadCopy.buffers[ordinal];
      const int owner = BufferOwnerOperation(afterAllocExtraBuffer, buffer);
      if (owner >= 0)
        nonTargetBuffersByOwner[owner].push_back(ordinal);
    }
  }

  std::string mappedIdentity(const std::string &buffer) const {
    std::set<std::string> candidates;
    for (const std::string &alternative : BufferAlternatives(buffer)) {
      const std::string source = MappedBufferIdentity(
          alternative, module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping);
      if (targetSourceIdentities.count(source))
        candidates.insert(finalIdentity.at(source));
    }
    return candidates.size() == 1 ? *candidates.begin() : std::string();
  }

  std::string preAlignmentType(const LocalBufferRecord &record) const {
    if (startsWith(record.sourceIdentity, "base:")) {
      const size_t ordinal = static_cast<size_t>(
          std::stoull(record.sourceIdentity.substr(5)));
      const auto &allocations =
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.allocations;
      if (ordinal < allocations.size())
        return allocations[ordinal].type;
    }
    if (startsWith(record.sourceIdentity, "decompose:")) {
      const size_t split = record.sourceIdentity.rfind(':');
      if (split != std::string::npos) {
        const int owner = std::stoi(record.sourceIdentity.substr(
            10, split - 10));
        const size_t ordinal = static_cast<size_t>(
            std::stoull(record.sourceIdentity.substr(split + 1)));
        size_t current = 0;
        for (const DecomposeBufferAllocation &allocation :
             module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.decomposeAllocations) {
          if (allocation.ownerOperation != owner)
            continue;
          if (current++ == ordinal)
            return allocation.type;
        }
      }
    }
    return {};
  }

  void indexValues() {
    const std::map<int, std::string> valueTypes = ValueTypes(logical);
    for (const auto &[value, type] : valueTypes)
      namedValueTypes[PlanMemoryValueName(value)] =
          IsTensorType(type) ? ConvertTensorToMemRefType(type) : type;
    for (const GenericOperation &operation : logical.operations) {
      for (int operand : operation.operands)
        valueUsers[operand].insert(operation.id);
      if (operation.name != "arith.constant" || operation.results.size() != 1)
        continue;
      if (operation.resultTypes.size() != 1)
        continue;
      std::string value = FindDictionaryValue(operation.properties, "value");
      if (value.empty())
        value = FindDictionaryValue(operation.attributes, "value");
      const size_t typedSuffix = value.find(" : ");
      if (typedSuffix != std::string::npos)
        value = trim(value.substr(0, typedSuffix));
      try {
        size_t consumed = 0;
        const int64_t integer = std::stoll(value, &consumed, 0);
        if (consumed != value.size())
          throw std::invalid_argument("trailing integer literal text");
        const GenericOperation *function =
            EnclosingFunction(logical, operation);
        if (function) {
          if (operation.resultTypes.front() == "index")
            integerConstants[function->id].emplace(
                integer, operation.results.front());
          const std::string &type = operation.resultTypes.front();
          if (type.size() > 1 && type.front() == 'i') {
            const unsigned width =
                static_cast<unsigned>(std::stoul(type.substr(1)));
            if (width <= 64) {
              const uint64_t mask =
                  width == 64 ? std::numeric_limits<uint64_t>::max()
                              : (uint64_t{1} << width) - 1;
              integerScalarConstants[function->id][type].emplace(
                  static_cast<uint64_t>(integer) & mask,
                  operation.results.front());
            }
          }
          if (integer == 0 && operation.resultTypes.size() == 1)
            zeroScalarConstants[function->id].emplace(
                operation.resultTypes.front(), operation.results.front());
        }
      } catch (const std::exception &) {
        try {
          size_t consumed = 0;
          const long double floating = std::stold(value, &consumed);
          if (consumed != value.size())
            throw std::invalid_argument("trailing float literal text");
          const GenericOperation *function =
              EnclosingFunction(logical, operation);
          if (function && floating == 0.0L &&
              operation.resultTypes.size() == 1)
            zeroScalarConstants[function->id].emplace(
                operation.resultTypes.front(), operation.results.front());
        } catch (const std::exception &) {
        }
      }
    }
    for (const BufferizedValueBinding &binding :
         module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.values) {
      bufferizedBufferIds[binding.valueId] = binding.bufferId;
      bufferizedValueTypes[binding.valueId] = binding.type;
      bufferRepresentativeValues.emplace(binding.bufferId, binding.valueId);
      bufferRepresentativeValuesByType.emplace(
          std::make_pair(binding.bufferId, binding.type), binding.valueId);
      if (BufferAlternatives(binding.bufferId).size() == 1)
        unambiguousBufferizedValues.insert(binding.valueId);
      const std::string identity = mappedIdentity(binding.bufferId);
      if (!identity.empty()) {
        valueBuffers[binding.valueId] = identity;
        const std::string value = PlanMemoryValueName(binding.valueId);
        namedValueTypes[value] = PlanMemoryPreserveAddressSpace(
            namedValueTypes.at(value), bufferTypes.at(identity));
      }
      if (BufferAlternatives(binding.bufferId).size() > 1)
        conditionalValues.insert(binding.valueId);
    }
    const PreBufferizationCSEState &preBufferizationCSE =
        module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer
            .postBufferization.bufferized.preBufferizationCSE;
    for (const auto &[value, alias] : preBufferizationCSE.valueAliases) {
      auto rawBuffer = bufferizedBufferIds.find(alias);
      if (rawBuffer != bufferizedBufferIds.end()) {
        bufferizedBufferIds[value] = rawBuffer->second;
        bufferRepresentativeValues.emplace(rawBuffer->second, value);
        if (BufferAlternatives(rawBuffer->second).size() == 1)
          unambiguousBufferizedValues.insert(value);
      }
      auto rawType = bufferizedValueTypes.find(alias);
      if (rawType != bufferizedValueTypes.end()) {
        bufferizedValueTypes[value] = rawType->second;
        if (rawBuffer != bufferizedBufferIds.end())
          bufferRepresentativeValuesByType.emplace(
              std::make_pair(rawBuffer->second, rawType->second), value);
      }
      auto mapped = valueBuffers.find(alias);
      if (mapped != valueBuffers.end()) {
        valueBuffers[value] = mapped->second;
        const std::string valueName = PlanMemoryValueName(value);
        const auto namedType = namedValueTypes.find(valueName);
        if (namedType != namedValueTypes.end())
          namedType->second = PlanMemoryPreserveAddressSpace(
              namedType->second, bufferTypes.at(mapped->second));
      }
      if (conditionalValues.count(alias) != 0)
        conditionalValues.insert(value);
    }
    for (const BufferizedOperandAccess &access :
         module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.bufferized.accesses) {
      const std::string identity = mappedIdentity(access.bufferId);
      if (identity.empty() || alignedViewTypes.count(identity) != 0)
        continue;
      const GenericOperation &operation = logical.operations.at(
          static_cast<size_t>(access.operationId));
      if (access.operandNumber < 0 ||
          static_cast<size_t>(access.operandNumber) >=
              operation.operandTypes.size())
        continue;
      std::string type = operation.operandTypes[
          static_cast<size_t>(access.operandNumber)];
      if (IsTensorType(type))
        type = ConvertTensorToMemRefType(type);
      const std::optional<MemRefTypeModel> logicalType = ParseMemRefType(type);
      auto record = finalBufferRecords.find(identity);
      if (!logicalType || record == finalBufferRecords.end() ||
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.count(
              record->second->sourceIdentity) == 0)
        continue;
      alignedViewTypes[identity] = PlanMemoryPreserveAddressSpace(
          PlanMemoryFormatMemRefType(AlignedOperandType(
              *record->second, *logicalType,
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage)),
          bufferTypes.at(identity));
    }
    for (const auto &[identity, record] : finalBufferRecords) {
      const auto strideAlignment =
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.find(
              record->sourceIdentity);
      const auto allocationAlignment =
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.allocAlignments.find(
              record->sourceIdentity);
      const bool hasStrideAlignment =
          strideAlignment !=
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.end() &&
          !strideAlignment->second.empty();
      const bool hasAllocationAlignment =
          allocationAlignment !=
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.allocAlignments.end() &&
          !allocationAlignment->second.empty();
      if (!hasStrideAlignment && !hasAllocationAlignment)
        continue;
      const std::string type = preAlignmentType(*record);
      const std::optional<MemRefTypeModel> logicalType = ParseMemRefType(type);
      if (!logicalType)
        continue;
      MemRefTypeModel aligned = *logicalType;
      if (hasStrideAlignment)
        aligned = AlignedOperandType(
            *record, aligned, module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage);
      if (hasAllocationAlignment) {
        const std::optional<MemRefTypeModel> physical =
            ParseMemRefType(bufferTypes.at(identity));
        if (!physical || physical->strides.size() < aligned.shape.size())
          throw std::runtime_error(
              "PlanMemory input builder: malformed size-aligned allocation");
        aligned.strides.assign(physical->strides.begin(),
                               physical->strides.begin() +
                                   static_cast<std::ptrdiff_t>(
                                       aligned.shape.size()));
        aligned.offset = physical->offset;
        aligned.hasStridedLayout = true;
      }
      const bool hasDynamicExtent = std::any_of(
          logicalType->shape.begin(), logicalType->shape.end(),
          [](const std::optional<int64_t> &extent) { return !extent; });
      if (hasDynamicExtent && hasStrideAlignment &&
          aligned.shape.size() == logicalType->shape.size()) {
        aligned.shape.push_back(1);
        aligned.strides.push_back(1);
      } else if (!hasDynamicExtent) {
        aligned.shape.resize(logicalType->shape.size());
        aligned.strides.resize(logicalType->shape.size());
      }
      alignedViewTypes[identity] = PlanMemoryPreserveAddressSpace(
          PlanMemoryFormatMemRefType(aligned),
          bufferTypes.at(identity));
    }
    static const std::set<std::string> aliases = {
        "memref.cast",          "memref.collapse_shape",
        "memref.expand_shape", "memref.reinterpret_cast",
        "memref.subview",      "memref.view",
        "tensor.cast",          "tensor.collapse_shape",
        "tensor.expand_shape", "tensor.extract_slice",
        "hivm.hir.bitcast",
        "scf.for",              "scf.while",
        "scf.if",               "arith.select"};
    for (const GenericOperation &operation : logical.operations) {
      bool preservesSSA = aliases.count(operation.name) != 0;
      if (preservesSSA && operation.name == "tensor.extract_slice" &&
          !operation.operands.empty() && !operation.results.empty()) {
        const auto operandBuffer =
            bufferizedBufferIds.find(operation.operands.front());
        for (int resultValue : operation.results) {
          const auto resultBuffer = bufferizedBufferIds.find(resultValue);
          if (operandBuffer != bufferizedBufferIds.end() &&
              resultBuffer != bufferizedBufferIds.end() &&
              operandBuffer->second != resultBuffer->second) {
            preservesSSA = false;
            break;
          }
        }
      }
      if (preservesSSA)
        preservedSSAValues.insert(operation.results.begin(),
                                  operation.results.end());
    }
    for (const GenericBlock &block : logical.blocks) {
      preservedSSAValues.insert(block.arguments.begin(),
                                block.arguments.end());
      const GenericRegion &region =
          logical.regions.at(static_cast<size_t>(block.regionId));
      const GenericOperation &parent = logical.operations.at(
          static_cast<size_t>(region.parentOperation));
      if (parent.name != "func.func" || !IsAIVFunction(parent))
        continue;
      for (int argument : block.arguments)
        functionArguments.push_back(PlanMemoryValueName(argument));
    }
  }

  void indexFoldedAffineConstants() {
    for (const GenericOperation &operation : logical.operations) {
      auto folded = arithToAffine.foldedConstants.find(operation.id);
      if (folded == arithToAffine.foldedConstants.end() ||
          operation.results.size() != 1)
        continue;
      const int resultValue = operation.results.front();
      foldedScalarConstants[resultValue] = folded->second;
      const GenericOperation *function =
          EnclosingFunction(logical, operation);
      if (!function)
        continue;
      auto &constants = integerConstants[function->id];
      auto existing = constants.find(folded->second);
      if (existing == constants.end()) {
        materializedAffineConstants[operation.id] = folded->second;
        constants.emplace(folded->second, resultValue);
        continue;
      }
      erasedOperations.insert(operation.id);
      erasedValueAliases[resultValue] =
          PlanMemoryValueName(existing->second);
    }
  }

  void indexFoldedAffineValues() {
    for (const GenericOperation &operation : logical.operations) {
      if (operation.results.size() != 1)
        continue;
      auto folded = arithToAffine.foldedValues.find(operation.id);
      if (folded != arithToAffine.foldedValues.end()) {
        erasedOperations.insert(operation.id);
        erasedValueAliases[operation.results.front()] =
            valueName(folded->second);
      }
      auto cse = arithToAffine.cseValueAliases.find(
          operation.results.front());
      if (cse != arithToAffine.cseValueAliases.end()) {
        erasedOperations.insert(operation.id);
        erasedValueAliases[operation.results.front()] =
            valueName(cse->second);
      }
    }
  }

  void indexDeadConstantsAfterHIVMDecomposeOp() {
    for (const GenericOperation &constant : logical.operations) {
      if (constant.name != "arith.constant" || constant.results.size() != 1)
        continue;
      auto users = valueUsers.find(constant.results.front());
      if (users == valueUsers.end() || users->second.empty())
        continue;
      bool allUsesAreDecomposedVSubScalars = true;
      for (int userId : users->second) {
        const GenericOperation &user =
            logical.operations.at(static_cast<size_t>(userId));
        const bool decomposed =
            user.name == "hivm.hir.vsub" && user.operands.size() >= 2 &&
            user.operands[1] == constant.results.front() &&
            user.operandTypes.size() >= 2 &&
            !IsTensorType(user.operandTypes[1]) &&
            !IsMemRefType(user.operandTypes[1]);
        if (!decomposed) {
          allUsesAreDecomposedVSubScalars = false;
          break;
        }
      }
      if (allUsesAreDecomposedVSubScalars)
        erasedOperations.insert(constant.id);
    }
  }

  std::string arithToAffineName(const GenericOperation &operation) const {
    auto replacement = arithToAffine.replacementNames.find(operation.id);
    return replacement == arithToAffine.replacementNames.end()
               ? std::string()
               : replacement->second;
  }

  void indexEvents() {
    for (const PlanMemoryInputAccessRecord &event : module.accesses)
      eventsBySource[event.sourceOperation].push_back(&event);
  }

  bool isMappedFor(const GenericOperation &operation) const {
    return operation.name == "scf.for" &&
           (HasDictionaryEntry(operation.properties,
                               "map_for_to_forall") ||
            HasDictionaryEntry(operation.attributes,
                               "map_for_to_forall"));
  }

  std::optional<int64_t> evaluateLinearAffineExpression(
      std::string expression, int64_t d0) const {
    expression.erase(
        std::remove_if(expression.begin(), expression.end(),
                       [](unsigned char character) {
                         return std::isspace(character) != 0;
                       }),
        expression.end());
    if (expression.empty())
      return std::nullopt;
    int64_t value = 0;
    size_t cursor = 0;
    int64_t sign = 1;
    while (cursor < expression.size()) {
      if (expression[cursor] == '+') {
        sign = 1;
        ++cursor;
        continue;
      }
      if (expression[cursor] == '-') {
        sign = -1;
        ++cursor;
        continue;
      }
      size_t end = cursor;
      while (end < expression.size() && expression[end] != '+' &&
             (expression[end] != '-' ||
              (end > cursor && expression[end - 1] == '*')))
        ++end;
      const std::string term = expression.substr(cursor, end - cursor);
      int64_t termValue = 0;
      if (term == "d0") {
        termValue = d0;
      } else if (startsWith(term, "d0*")) {
        termValue = d0 * std::stoll(term.substr(3));
      } else if (endsWith(term, "*d0")) {
        termValue = std::stoll(term.substr(0, term.size() - 3)) * d0;
      } else {
        try {
          termValue = std::stoll(term);
        } catch (const std::exception &) {
          return std::nullopt;
        }
      }
      value += sign * termValue;
      sign = 1;
      cursor = end;
    }
    return value;
  }

  std::optional<int64_t>
  mappedLoopConstant(int value) const {
    auto folded = foldedScalarConstants.find(value);
    if (folded != foldedScalarConstants.end())
      return folded->second;
    const GenericOperation *definition = DefiningOperation(logical, value);
    if (!definition || definition->resultTypes.size() != 1 ||
        definition->resultTypes.front() != "index")
      return std::nullopt;
    const std::optional<ArithIntegerConstant> constant =
        ParseArithIntegerConstant(*definition);
    return constant
               ? std::optional<int64_t>(static_cast<int64_t>(constant->bits))
                    : std::nullopt;
  }

  std::optional<std::vector<int64_t>>
  mappedLoopDomain(const GenericOperation &operation) const {
    if (!isMappedFor(operation) || operation.operands.size() < 3)
      return std::nullopt;
    const std::optional<int64_t> lower =
        mappedLoopConstant(operation.operands[0]);
    const std::optional<int64_t> upper =
        mappedLoopConstant(operation.operands[1]);
    const std::optional<int64_t> step =
        mappedLoopConstant(operation.operands[2]);
    if (!lower || !upper || !step || *step <= 0)
      return std::nullopt;
    std::vector<int64_t> domain;
    for (int64_t value = *lower; value < *upper;) {
      domain.push_back(value);
      if (domain.size() > 64 ||
          value > std::numeric_limits<int64_t>::max() - *step)
        return std::nullopt;
      value += *step;
    }
    return domain;
  }

  std::optional<int64_t> evaluateMappedAffineValue(
      int value, const std::map<int, int64_t> &inductionValues,
      const std::map<int, const GenericOperation *> &definitions,
      std::set<int> &visiting) const {
    auto induction = inductionValues.find(value);
    if (induction != inductionValues.end())
      return induction->second;
    auto folded = foldedScalarConstants.find(value);
    if (folded != foldedScalarConstants.end())
      return folded->second;
    auto definition = definitions.find(value);
    if (definition == definitions.end() || !visiting.insert(value).second)
      return std::nullopt;
    const GenericOperation &operation = *definition->second;
    const std::optional<ArithIntegerConstant> integerConstant =
        operation.resultTypes.size() == 1 &&
                operation.resultTypes.front() == "index"
            ? ParseArithIntegerConstant(operation)
            : std::nullopt;
    if (integerConstant) {
      visiting.erase(value);
      return static_cast<int64_t>(integerConstant->bits);
    }

    std::function<std::optional<int64_t>(const std::string &)> evaluateExpr;
    evaluateExpr = [&](const std::string &expression)
        -> std::optional<int64_t> {
      if (const std::optional<int64_t> constant =
              AffineConstantValue(expression))
        return constant;
      if (const std::optional<int> operand = AffineValue(expression))
        return evaluateMappedAffineValue(*operand, inductionValues,
                                         definitions, visiting);
      for (const std::string &kind : {"mul", "add", "mod", "floordiv",
                                      "ceildiv"}) {
        const auto operands =
            AffineBinaryExpressionOperands(expression, kind);
        if (!operands)
          continue;
        const std::optional<int64_t> lhs = evaluateExpr(operands->first);
        const std::optional<int64_t> rhs = evaluateExpr(operands->second);
        return lhs && rhs ? CheckedAffineArithmetic(kind, *lhs, *rhs)
                          : std::nullopt;
      }
      return std::nullopt;
    };

    std::optional<int64_t> evaluated;
    if (const std::optional<std::string> expression =
            ExistingAffineApplyExpression(operation)) {
      evaluated = evaluateExpr(*expression);
    } else if (const auto minMaxExpressions =
                   ExistingAffineMinMaxExpressions(operation)) {
      for (const std::string &minMaxExpression : *minMaxExpressions) {
        const std::optional<int64_t> candidate =
            evaluateExpr(minMaxExpression);
        if (!candidate) {
          evaluated.reset();
          break;
        }
        if (!evaluated)
          evaluated = candidate;
        else if (operation.name == "affine.min")
          evaluated = std::min(*evaluated, *candidate);
        else
          evaluated = std::max(*evaluated, *candidate);
      }
    } else if (operation.operands.size() == 2 &&
               (operation.name == "arith.addi" ||
                operation.name == "arith.subi" ||
                operation.name == "arith.muli" ||
                operation.name == "arith.minsi" ||
                operation.name == "arith.minui" ||
                operation.name == "arith.maxsi" ||
                operation.name == "arith.maxui")) {
      const std::optional<int64_t> lhs = evaluateMappedAffineValue(
          operation.operands[0], inductionValues, definitions, visiting);
      const std::optional<int64_t> rhs = evaluateMappedAffineValue(
          operation.operands[1], inductionValues, definitions, visiting);
      if (lhs && rhs) {
        if (operation.name == "arith.addi")
          evaluated = CheckedAffineArithmetic("add", *lhs, *rhs);
        else if (operation.name == "arith.subi") {
          const __int128 difference =
              static_cast<__int128>(*lhs) - static_cast<__int128>(*rhs);
          if (difference >= std::numeric_limits<int64_t>::min() &&
              difference <= std::numeric_limits<int64_t>::max())
            evaluated = static_cast<int64_t>(difference);
        } else if (operation.name == "arith.muli")
          evaluated = CheckedAffineArithmetic("mul", *lhs, *rhs);
        else if (operation.name == "arith.minsi" ||
                 operation.name == "arith.minui")
          evaluated = std::min(*lhs, *rhs);
        else
          evaluated = std::max(*lhs, *rhs);
      }
    }
    visiting.erase(value);
    return evaluated;
  }

  void collectMappedAffineInductionValues(
      int value, const std::map<int, const GenericOperation *> &definitions,
      const std::map<int, std::vector<int64_t>> &domains,
      std::set<int> &inductionValues, std::set<int> &visited) const {
    if (domains.count(value) != 0) {
      inductionValues.insert(value);
      return;
    }
    if (!visited.insert(value).second)
      return;
    auto definition = definitions.find(value);
    if (definition == definitions.end())
      return;
    const GenericOperation &operation = *definition->second;
    if (operation.name != "affine.apply" &&
        operation.name != "affine.min" &&
        operation.name != "affine.max" &&
        operation.name != "arith.addi" &&
        operation.name != "arith.subi" &&
        operation.name != "arith.muli" &&
        operation.name != "arith.minsi" &&
        operation.name != "arith.minui" &&
        operation.name != "arith.maxsi" &&
        operation.name != "arith.maxui")
      return;
    for (int operand : operation.operands)
      collectMappedAffineInductionValues(operand, definitions, domains,
                                         inductionValues, visited);
  }

  std::optional<int64_t> foldMappedAffineMin(
      const GenericOperation &operation,
      const std::map<int, std::vector<int64_t>> &domains,
      const std::map<int, const GenericOperation *> &definitions) const {
    if (operation.name != "affine.min" || operation.results.size() != 1)
      return std::nullopt;

    std::set<int> inductionValues;
    std::set<int> visited;
    for (int operand : operation.operands)
      collectMappedAffineInductionValues(operand, definitions, domains,
                                         inductionValues, visited);
    if (!inductionValues.empty()) {
      std::vector<int> ordered(inductionValues.begin(),
                               inductionValues.end());
      std::map<int, int64_t> assignment;
      std::optional<int64_t> folded;
      bool valid = true;
      std::function<void(size_t)> evaluateDomain = [&](size_t index) {
        if (!valid)
          return;
        if (index != ordered.size()) {
          const int induction = ordered[index];
          for (int64_t value : domains.at(induction)) {
            assignment[induction] = value;
            evaluateDomain(index + 1);
          }
          assignment.erase(induction);
          return;
        }
        std::set<int> evaluating;
        const std::optional<int64_t> value = evaluateMappedAffineValue(
            operation.results.front(), assignment, definitions, evaluating);
        if (!value || (folded && *folded != *value)) {
          valid = false;
          return;
        }
        folded = value;
      };
      evaluateDomain(0);
      if (valid && folded)
        return folded;
    }

    if (operation.operands.size() != 1 ||
        domains.count(operation.operands.front()) == 0)
      return std::nullopt;
    const std::string map =
        FindDictionaryValue(operation.properties, "map");
    const size_t arrow = map.find("-> (");
    const size_t close = map.rfind(")>");
    if (arrow == std::string::npos || close == std::string::npos ||
        close <= arrow + 4)
      return std::nullopt;
    const std::vector<std::string> expressions =
        splitTopLevel(map.substr(arrow + 4, close - arrow - 4));
    std::optional<int64_t> folded;
    for (int64_t d0 : domains.at(operation.operands.front())) {
      std::optional<int64_t> minimum;
      for (const std::string &expression : expressions) {
        const std::optional<int64_t> value =
            evaluateLinearAffineExpression(expression, d0);
        if (!value)
          return std::nullopt;
        minimum = minimum ? std::min(*minimum, *value) : *value;
      }
      if (!minimum || (folded && *folded != *minimum))
        return std::nullopt;
      folded = minimum;
    }
    return folded;
  }

  void indexMappedLoops() {
    std::map<int, std::vector<int64_t>> mappedInductionDomains;
    std::map<int, const GenericOperation *> definitions;
    for (const GenericOperation &operation : logical.operations)
      for (int value : operation.results)
        definitions[value] = &operation;
    for (const GenericOperation &operation : logical.operations) {
      if (!isMappedFor(operation))
        continue;
      erasedMappedLoops.insert(operation.id);
      erasedRegionIds.insert(operation.regions.begin(),
                             operation.regions.end());
      for (int regionId : operation.regions) {
        const GenericRegion &region =
            logical.regions.at(static_cast<size_t>(regionId));
        for (int blockId : region.blocks) {
          const GenericBlock &block =
              logical.blocks.at(static_cast<size_t>(blockId));
          const std::optional<std::vector<int64_t>> domain =
              mappedLoopDomain(operation);
          if (!block.arguments.empty() && domain && !domain->empty())
            mappedInductionDomains[block.arguments.front()] = *domain;
        }
      }
    }
    for (const GenericOperation &operation : logical.operations) {
      const std::optional<int64_t> folded =
          foldMappedAffineMin(operation, mappedInductionDomains, definitions);
      if (!folded || operation.results.size() != 1)
        continue;
      foldedScalarConstants[operation.results.front()] = *folded;
      erasedOperations.insert(operation.id);
    }

    // ConvertToHIVMOpPass uses applyPatternsGreedily. Once a folded affine
    // value is constified in a subview, its now-dead affine producer chain is
    // removed by the greedy driver's region simplification.
    while (true) {
      std::map<int, size_t> uses;
      for (const GenericOperation &operation : logical.operations) {
        if (erasedOperations.count(operation.id) != 0)
          continue;
        for (int operand : operation.operands)
          ++uses[operand];
      }
      bool changed = false;
      for (const GenericOperation &operation : logical.operations) {
        if (erasedOperations.count(operation.id) != 0 ||
            (operation.name != "affine.apply" &&
             operation.name != "affine.min" &&
             operation.name != "affine.max") ||
            operation.results.empty())
          continue;
        if (std::all_of(operation.results.begin(), operation.results.end(),
                        [&](int resultValue) {
                          return uses[resultValue] == 0;
                        })) {
          erasedOperations.insert(operation.id);
          changed = true;
        }
      }
      if (!changed)
        break;
    }
  }

  std::vector<int> regionPath(const GenericOperation &operation) const {
    std::vector<int> path;
    int region = operation.regionId;
    while (region >= 0) {
      path.push_back(region);
      const GenericRegion &record =
          logical.regions.at(static_cast<size_t>(region));
      const GenericOperation &parent = logical.operations.at(
          static_cast<size_t>(record.parentOperation));
      region = parent.regionId;
    }
    std::reverse(path.begin(), path.end());
    if (!path.empty())
      path.erase(path.begin());
    path.erase(std::remove_if(path.begin(), path.end(), [&](int regionId) {
                 return erasedRegionIds.count(regionId) != 0;
               }),
               path.end());
    return path;
  }

  std::string valueName(int value, bool preserveConditional = true) const {
    auto erasedAlias = erasedValueAliases.find(value);
    if (erasedAlias != erasedValueAliases.end())
      return erasedAlias->second;
    auto canonicalView = canonicalViewAliases.find(value);
    if (canonicalView != canonicalViewAliases.end())
      return canonicalView->second;
    auto scalar = scalarValues.find(value);
    if (scalar != scalarValues.end())
      return scalar->second;
    if (preservedSSAValues.count(value))
      return PlanMemoryValueName(value);
    if (preserveConditional && conditionalValues.count(value))
      return PlanMemoryValueName(value);
    auto rawBuffer = bufferizedBufferIds.find(value);
    if (rawBuffer != bufferizedBufferIds.end()) {
      int representativeValue = value;
      auto type = bufferizedValueTypes.find(value);
      if (type != bufferizedValueTypes.end()) {
        auto typedRepresentative = bufferRepresentativeValuesByType.find(
            std::make_pair(rawBuffer->second, type->second));
        if (typedRepresentative != bufferRepresentativeValuesByType.end())
          representativeValue = typedRepresentative->second;
      }
      if (representativeValue == value) {
        auto representative =
            bufferRepresentativeValues.find(rawBuffer->second);
        if (representative != bufferRepresentativeValues.end())
          representativeValue = representative->second;
      }
      if (representativeValue != value)
        return valueName(representativeValue, preserveConditional);
    }
    auto buffer = valueBuffers.find(value);
    if (buffer != valueBuffers.end()) {
      auto allocationView = allocationViewAliases.find(buffer->second);
      if (allocationView != allocationViewAliases.end())
        return allocationView->second;
      return bufferNames.at(buffer->second);
    }
    return PlanMemoryValueName(value);
  }

  std::string scalarValueName(int value) const {
    auto alias = scalarValues.find(value);
    return alias == scalarValues.end() ? valueName(value) : alias->second;
  }

  std::string latestSubviewForValue(int value) const {
    auto buffer = valueBuffers.find(value);
    auto valueTypeIt = bufferizedValueTypes.find(value);
    if (buffer == valueBuffers.end() ||
        valueTypeIt == bufferizedValueTypes.end())
      return {};
    std::string expectedType = valueTypeIt->second;
    if (IsTensorType(expectedType))
      expectedType = ConvertTensorToMemRefType(expectedType);
    const auto expected = ParseMemRefType(expectedType);
    const auto backing = ParseMemRefType(bufferTypes.at(buffer->second));
    if (!expected || !backing || expected->shape == backing->shape)
      return {};
    const std::string &backingName = bufferNames.at(buffer->second);
    for (auto operation = result.operations.rbegin();
         operation != result.operations.rend(); ++operation) {
      if (operation->opName != "memref.subview" ||
          operation->text.find(backingName) == std::string::npos)
        continue;
      const std::vector<std::string> results = operationResultNames(*operation);
      if (results.empty())
        continue;
      auto typeIt = namedValueTypes.find(results.front());
      const auto candidate = typeIt == namedValueTypes.end()
                                 ? std::nullopt
                                 : ParseMemRefType(typeIt->second);
      if (candidate && candidate->shape == expected->shape)
        return results.front();
    }
    return {};
  }

  static std::vector<std::vector<size_t>>
  parseReassociation(const std::string &value) {
    std::vector<std::vector<size_t>> groups;
    int depth = 0;
    for (size_t index = 0; index < value.size();) {
      if (value[index] == '[') {
        ++depth;
        if (depth == 2)
          groups.emplace_back();
        ++index;
        continue;
      }
      if (value[index] == ']') {
        --depth;
        ++index;
        continue;
      }
      if (depth == 2 && std::isdigit(static_cast<unsigned char>(value[index]))) {
        size_t end = index;
        while (end < value.size() &&
               std::isdigit(static_cast<unsigned char>(value[end])))
          ++end;
        groups.back().push_back(
            static_cast<size_t>(std::stoull(value.substr(index, end - index))));
        index = end;
        continue;
      }
      ++index;
    }
    return groups;
  }

  OperationRecord baseOperation(const GenericOperation &source,
                                const std::string &name) {
    OperationRecord operation;
    operation.index = static_cast<int>(result.operations.size());
    operation.line = operation.index + 1;
    operation.indent = static_cast<int>(regionPath(source).size() * 2);
    operation.operationId = nextOperationId++;
    operation.opName = name;
    const GenericOperation *placement = &source;
    if (source.parentId >= 0 &&
        erasedMappedLoops.count(source.parentId) != 0)
      placement = &logical.operations.at(static_cast<size_t>(source.parentId));
    operation.regionPath = regionPath(*placement);
    operation.blockId = placement->blockId;
    operation.blockLabel = "^bb" + std::to_string(placement->blockId);
    if (placement->blockId >= 0) {
      const GenericBlock &block =
          logical.blocks.at(static_cast<size_t>(placement->blockId));
      for (int argument : block.arguments)
        operation.blockArguments.push_back(PlanMemoryValueName(argument));
    }
    return operation;
  }

  void placeInRegion(OperationRecord &operation,
                     const std::vector<int> &path) {
    operation.regionPath = path;
    operation.blockArguments.clear();
    if (path.empty())
      return;
    const int region = path.back();
    auto [block, inserted] = syntheticBlocks.emplace(region, nextSyntheticBlock);
    if (inserted)
      ++nextSyntheticBlock;
    operation.blockId = block->second;
    operation.blockLabel = "^bb" + std::to_string(operation.blockId);
  }

  std::string valueType(int value, std::string type) const {
    if (IsTensorType(type))
      type = ConvertTensorToMemRefType(type);
    auto buffer = valueBuffers.find(value);
    if (buffer != valueBuffers.end() && !preservedSSAValues.count(value))
      return bufferTypes.at(buffer->second);
    if (buffer != valueBuffers.end())
      type = PlanMemoryPreserveAddressSpace(type,
                                            bufferTypes.at(buffer->second));
    return type;
  }

  std::string bufferizedResultType(const GenericOperation &source,
                                   size_t resultIndex) const {
    std::string resultType = valueType(source.results.at(resultIndex),
                                       source.resultTypes.at(resultIndex));
    if (source.name == "hivm.hir.bitcast" && !source.operands.empty() &&
        !source.operandTypes.empty()) {
      const std::string operandName = valueName(source.operands.front());
      std::string operandType =
          valueType(source.operands.front(), source.operandTypes.front());
      auto namedOperand = namedValueTypes.find(operandName);
      if (namedOperand != namedValueTypes.end())
        operandType = namedOperand->second;
      std::optional<MemRefTypeModel> operandMemref =
          ParseMemRefType(operandType);
      const std::optional<MemRefTypeModel> resultMemref =
          ParseMemRefType(resultType);
      if (operandMemref && resultMemref) {
        operandMemref->elementType = resultMemref->elementType;
        return PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(*operandMemref), operandType);
      }
    }
    if ((source.name == "tensor.expand_shape" ||
         source.name == "memref.expand_shape") &&
        !source.operands.empty() && !source.operandTypes.empty()) {
      std::string sourceType =
          valueType(source.operands.front(), source.operandTypes.front());
      const std::string sourceName = valueName(source.operands.front());
      auto namedSource = namedValueTypes.find(sourceName);
      if (namedSource != namedValueTypes.end())
        sourceType = namedSource->second;
      const std::optional<MemRefTypeModel> sourceMemref =
          ParseMemRefType(sourceType);
      std::optional<MemRefTypeModel> expanded = ParseMemRefType(resultType);
      if (sourceMemref && expanded && sourceMemref->shape.size() == 1 &&
          !sourceMemref->strides.empty()) {
        expanded->strides.assign(expanded->shape.size(),
                                 sourceMemref->strides.front());
        for (size_t axis = expanded->shape.size(); axis > 1; --axis) {
          const size_t inner = axis - 1;
          const size_t outer = inner - 1;
          if (expanded->strides[inner] && expanded->shape[inner])
            expanded->strides[outer] =
                *expanded->strides[inner] * *expanded->shape[inner];
          else
            expanded->strides[outer] = std::nullopt;
        }
        expanded->offset = sourceMemref->offset;
        expanded->hasStridedLayout = true;
        return PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(*expanded), sourceType);
      }
    }
    if ((source.name == "tensor.collapse_shape" ||
         source.name == "memref.collapse_shape") &&
        !source.operands.empty() && !source.operandTypes.empty()) {
      if (source.name == "tensor.collapse_shape" &&
          resultIndex < source.results.size()) {
        const auto sourceBuffer =
            bufferizedBufferIds.find(source.operands.front());
        const auto resultBuffer =
            bufferizedBufferIds.find(source.results[resultIndex]);
        if (sourceBuffer != bufferizedBufferIds.end() &&
            resultBuffer != bufferizedBufferIds.end() &&
            sourceBuffer->second != resultBuffer->second)
          return resultType;
      }
      std::string sourceType =
          valueType(source.operands.front(), source.operandTypes.front());
      const std::string sourceName = valueName(source.operands.front());
      auto namedSource = namedValueTypes.find(sourceName);
      if (namedSource != namedValueTypes.end())
        sourceType = namedSource->second;
      const std::optional<MemRefTypeModel> sourceMemref =
          ParseMemRefType(sourceType);
      const std::optional<MemRefTypeModel> resultMemref =
          ParseMemRefType(resultType);
      if (sourceMemref && resultMemref) {
        std::string reassociationValue =
            FindDictionaryValue(source.properties, "reassociation");
        if (reassociationValue.empty())
          reassociationValue =
              FindDictionaryValue(source.attributes, "reassociation");
        std::vector<std::vector<size_t>> reassociation =
            parseReassociation(reassociationValue);
        if (reassociation.empty())
          reassociation = ContinuousReassociation({*sourceMemref});
        if (reassociation.size() == resultMemref->shape.size())
          return PlanMemoryPreserveAddressSpace(
              PlanMemoryFormatMemRefType(
                  CollapseMemRefType(*sourceMemref, reassociation)),
              sourceType);
      }
    }
    if ((source.name != "tensor.extract_slice" &&
         source.name != "memref.subview") ||
        source.operands.empty() || source.operandTypes.empty())
      return resultType;
    const std::string sourceType =
        valueType(source.operands.front(), source.operandTypes.front());
    const std::optional<MemRefTypeModel> sourceMemref =
        ParseMemRefType(sourceType);
    const std::optional<MemRefTypeModel> resultMemref =
        ParseMemRefType(resultType);
    if (!sourceMemref || !resultMemref)
      throw std::runtime_error("PlanMemory input builder: malformed extract_slice");
    MemRefTypeModel subview = *resultMemref;
    subview.hasStridedLayout = true;
    std::vector<std::optional<int64_t>> fullStrides = sourceMemref->strides;
    std::vector<int64_t> staticStrides = DecomposeI64Array(
        FindDictionaryValue(source.properties, "static_strides"));
    while (!staticStrides.empty() &&
           staticStrides.size() < sourceMemref->shape.size())
      staticStrides.push_back(1);
    if (!staticStrides.empty()) {
      if (staticStrides.size() != fullStrides.size())
        throw std::runtime_error(
            "PlanMemory input builder: malformed extract_slice strides");
      for (size_t index = 0; index < staticStrides.size(); ++index) {
        if (staticStrides[index] == std::numeric_limits<int64_t>::min() ||
            !fullStrides[index])
          fullStrides[index] = std::nullopt;
        else
          fullStrides[index] =
              *fullStrides[index] * staticStrides[index];
      }
    }
    std::vector<int64_t> staticSizes = DecomposeI64Array(
        FindDictionaryValue(source.properties, "static_sizes"));
    const std::vector<size_t> segments =
        OperandSegmentSizes(source.properties);
    if (segments.size() >= 3) {
      size_t dynamicSizeOperand = 1 + segments[1];
      for (int64_t &size : staticSizes) {
        if (size != std::numeric_limits<int64_t>::min())
          continue;
        if (dynamicSizeOperand >= source.operands.size())
          break;
        auto folded =
            foldedScalarConstants.find(source.operands[dynamicSizeOperand++]);
        if (folded != foldedScalarConstants.end())
          size = folded->second;
      }
    }
    while (!staticSizes.empty() &&
           staticSizes.size() < sourceMemref->shape.size())
      staticSizes.push_back(1);
    if (staticSizes.size() != sourceMemref->shape.size())
      throw std::runtime_error(
          "PlanMemory input builder: malformed extract_slice sizes for operation " +
          std::to_string(source.id) + " (source=" + sourceType +
          ", result=" + resultType + ", properties=" + source.properties +
          ")");
    subview.strides.clear();
    size_t resultAxis = 0;
    for (size_t sourceAxis = 0; sourceAxis < staticSizes.size(); ++sourceAxis) {
      const size_t sourceAxesLeft = staticSizes.size() - sourceAxis;
      const size_t resultAxesLeft = subview.shape.size() - resultAxis;
      const bool mustKeep = sourceAxesLeft == resultAxesLeft;
      const bool compatible =
          resultAxis < subview.shape.size() &&
          (staticSizes[sourceAxis] == std::numeric_limits<int64_t>::min() ||
           !subview.shape[resultAxis] ||
           staticSizes[sourceAxis] == *subview.shape[resultAxis]);
      if (mustKeep || compatible) {
        if (resultAxis >= subview.shape.size())
          throw std::runtime_error(
              "PlanMemory input builder: invalid extract_slice rank reduction");
        if (!subview.shape[resultAxis] &&
            staticSizes[sourceAxis] !=
                std::numeric_limits<int64_t>::min())
          subview.shape[resultAxis] = staticSizes[sourceAxis];
        subview.strides.push_back(fullStrides[sourceAxis]);
        ++resultAxis;
      } else if (staticSizes[sourceAxis] != 1) {
        throw std::runtime_error(
            "PlanMemory input builder: non-unit extract_slice rank reduction");
      }
    }
    if (resultAxis != subview.shape.size())
      throw std::runtime_error(
          "PlanMemory input builder: incomplete extract_slice rank reduction");
    subview.offset = sourceMemref->offset;
    std::vector<int64_t> staticOffsets = DecomposeI64Array(
        FindDictionaryValue(source.properties, "static_offsets"));
    while (!staticOffsets.empty() &&
           staticOffsets.size() < sourceMemref->shape.size())
      staticOffsets.push_back(0);
    if (subview.offset != std::numeric_limits<int64_t>::min()) {
      if (staticOffsets.size() != sourceMemref->strides.size())
        throw std::runtime_error(
            "PlanMemory input builder: malformed extract_slice offsets");
      for (size_t index = 0; index < staticOffsets.size(); ++index) {
        if (staticOffsets[index] == std::numeric_limits<int64_t>::min() ||
            !sourceMemref->strides[index]) {
          subview.offset = std::numeric_limits<int64_t>::min();
          break;
        }
        subview.offset += staticOffsets[index] * *sourceMemref->strides[index];
      }
    }
    return PlanMemoryPreserveAddressSpace(
        PlanMemoryFormatMemRefType(subview), sourceType);
  }

  std::vector<int> dynamicAllocationExtents(
      const LocalBufferRecord &record) const {
    if (!startsWith(record.sourceIdentity, "base:"))
      return {};
    const size_t ordinal = static_cast<size_t>(
        std::stoull(record.sourceIdentity.substr(5)));
    const auto &allocations = module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.allocations;
    if (ordinal >= allocations.size())
      return {};
    const BufferAllocation &allocation = allocations[ordinal];
    if (allocation.dynamicExtentCount == 0 ||
        allocation.dynamicExtentValues.empty())
      return {};

    const GenericOperation *slice =
        DefiningOperation(logical, allocation.dynamicExtentValues.front());
    if (!slice || (slice->name != "tensor.extract_slice" &&
                   slice->name != "memref.subview"))
      return allocation.dynamicExtentValues;
    const std::vector<int64_t> staticSizes = DecomposeI64Array(
        FindDictionaryValue(slice->properties, "static_sizes"));
    const std::vector<size_t> segments =
        OperandSegmentSizes(slice->properties);
    if (segments.size() < 3)
      return allocation.dynamicExtentValues;
    size_t dynamicSizeOperand = 1 + segments[1];
    std::vector<int> extents;
    for (int64_t size : staticSizes) {
      if (size != std::numeric_limits<int64_t>::min())
        continue;
      if (dynamicSizeOperand >= slice->operands.size())
        throw std::runtime_error(
            "PlanMemory input builder: malformed dynamic allocation extent");
      extents.push_back(slice->operands[dynamicSizeOperand++]);
    }
    return extents;
  }

  std::string emitAlignedDynamicExtent(const GenericOperation &source,
                                       int extent, uint64_t alignUnit) {
    const GenericOperation *definition =
        DefiningOperation(logical, extent);
    std::vector<int> operands = {extent};
    if (definition && definition->name == "affine.apply")
      operands = definition->operands;
    const std::string resultName =
        "%aligned_extent_" + std::to_string(nextScalarValue++);
    OperationRecord apply = baseOperation(source, "affine.apply");
    apply.text = resultName + " = affine.apply";
    for (int operand : operands)
      apply.text += " " + scalarValueName(operand);
    apply.text += " {align_unit = " + std::to_string(alignUnit) + "}";
    result.operations.push_back(std::move(apply));
    namedValueTypes[resultName] = "index";
    return resultName;
  }

  void emitAllocation(const GenericOperation &source,
                      const PlanMemoryInputBufferRecord &buffer) {
    const LocalBufferRecord *sourceBuffer = nullptr;
    auto record = finalBufferRecords.find(buffer.identity);
    if (record != finalBufferRecords.end())
      sourceBuffer = record->second;
    std::vector<int> dynamicExtents;
    std::string alignedDynamicExtent;
    if (sourceBuffer)
      dynamicExtents = dynamicAllocationExtents(*sourceBuffer);
    if (sourceBuffer && dynamicExtents.size() == 1) {
      const auto strideAlignment =
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.find(
              sourceBuffer->sourceIdentity);
      const std::optional<MemRefTypeModel> logicalType =
          ParseMemRefType(preAlignmentType(*sourceBuffer));
      if (strideAlignment !=
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.end() &&
          !strideAlignment->second.empty() && logicalType) {
        const uint64_t bitWidth = GetElementBitWidth(*logicalType);
        if (bitWidth == 0 || 256 % bitWidth != 0)
          throw std::runtime_error(
              "PlanMemory input builder: invalid dynamic stride alignment");
        alignedDynamicExtent = emitAlignedDynamicExtent(
            source, dynamicExtents.front(), 256 / bitWidth);
      }
    }

    OperationRecord operation = baseOperation(source, "memref.alloc");
    const std::string name = bufferNames.at(buffer.identity);
    const std::string type = bufferTypes.at(buffer.identity);
    operation.text = name + " = memref.alloc() : " + type;
    result.allocations.push_back(
        {name, type, buffer.constBits, buffer.constBits, false,
         operation.line});
    result.operations.push_back(std::move(operation));

    if (buffer.multiBufferNum > 1 || buffer.preloadLocalBuffer) {
      OperationRecord mark = baseOperation(source, "annotation.mark");
      std::ostringstream text;
      text << "annotation.mark " << name;
      if (buffer.multiBufferNum > 1)
        text << " {hivm.multi_buffer = " << buffer.multiBufferNum << '}';
      if (buffer.preloadLocalBuffer)
        text << " {hivm.preload_local_buffer}";
      text << " : " << type;
      mark.text = text.str();
      result.operations.push_back(std::move(mark));
    }

    if (!sourceBuffer)
      return;
    const auto strideAlignment =
        module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.find(
            sourceBuffer->sourceIdentity);
    const auto allocationAlignment =
        module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.allocAlignments.find(
            sourceBuffer->sourceIdentity);
    if ((strideAlignment ==
             module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.strideAlignments.end() ||
         strideAlignment->second.empty()) &&
        (allocationAlignment ==
             module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage.allocAlignments.end() ||
         allocationAlignment->second.empty()))
      return;
    auto alignedView = alignedViewTypes.find(buffer.identity);
    if (alignedView == alignedViewTypes.end())
      return;
    const std::string &viewType = alignedView->second;
    if (PhysicalTypeSignature(viewType) == PhysicalTypeSignature(type))
      return;
    const bool sizedDynamicAllocation = !alignedDynamicExtent.empty();
    OperationRecord view = baseOperation(
        source, sizedDynamicAllocation ? "memref.view" : "memref.subview");
    const std::string resultName =
        "%aligned_view_" + std::to_string(nextFlattenValue++);
    view.text = resultName +
                (sizedDynamicAllocation ? " = memref.view "
                                        : " = memref.subview ") +
                name;
    if (sizedDynamicAllocation) {
      const GenericOperation *function = EnclosingFunction(logical, source);
      auto constants = function ? integerConstants.find(function->id)
                                : integerConstants.end();
      if (constants == integerConstants.end() ||
          constants->second.count(0) == 0)
        throw std::runtime_error(
            "PlanMemory input builder: dynamic allocation zero constant is missing");
      view.text += ", " + scalarValueName(constants->second.at(0)) + ", " +
                   alignedDynamicExtent;
    }
    view.text += " : " + type + " -> " + viewType;
    result.operations.push_back(std::move(view));
    namedValueTypes[resultName] = viewType;
    std::string replacementName = resultName;
    if (sizedDynamicAllocation) {
      const std::optional<MemRefTypeModel> alignedType =
          ParseMemRefType(viewType);
      const std::optional<MemRefTypeModel> logicalType =
          ParseMemRefType(preAlignmentType(*sourceBuffer));
      if (!alignedType || !logicalType ||
          alignedType->shape.size() != logicalType->shape.size() + 1 ||
          !alignedType->shape.back() || *alignedType->shape.back() != 1 ||
          alignedType->strides.size() < logicalType->shape.size())
        throw std::runtime_error(
            "PlanMemory input builder: malformed dynamic stride-aligned view");

      MemRefTypeModel rankReducedType = *logicalType;
      rankReducedType.addressSpace = alignedType->addressSpace;
      rankReducedType.strides.assign(
          alignedType->strides.begin(),
          alignedType->strides.begin() +
              static_cast<std::ptrdiff_t>(logicalType->shape.size()));
      rankReducedType.offset = alignedType->offset;
      rankReducedType.hasStridedLayout = true;
      const std::string subviewType = PlanMemoryPreserveAddressSpace(
          PlanMemoryFormatMemRefType(rankReducedType), viewType);

      OperationRecord subview = baseOperation(source, "memref.subview");
      replacementName =
          "%aligned_subview_" + std::to_string(nextFlattenValue++);
      subview.text = replacementName + " = memref.subview " + resultName;
      for (int extent : dynamicExtents)
        subview.text += ", " + scalarValueName(extent);
      subview.text += " : " + viewType + " -> " + subviewType;
      result.operations.push_back(std::move(subview));
      namedValueTypes[replacementName] = subviewType;
    }
    allocationViewAliases[buffer.identity] = replacementName;
    stableBufferAliases[buffer.identity] = replacementName;
  }

  void emitVConcatDestinationSubview(
      const GenericOperation &source,
      const PlanMemoryInputAccessRecord &event) {
    if (source.name != "hivm.hir.vconcat" ||
        event.operationName != "hivm.hir.copy" ||
        event.accesses.size() < 2 ||
        event.generatedOrdinal >= source.operands.size())
      return;
    const std::vector<size_t> destinations = DpsInitOperandIndices(
        source.name, source.operands.size(), source.properties);
    if (destinations.size() != 1 || event.generatedOrdinal >= destinations[0])
      return;

    const std::string &destinationIdentity = event.accesses[1].first;
    std::string destinationName = bufferNames.at(destinationIdentity);
    auto stable = stableBufferAliases.find(destinationIdentity);
    if (stable != stableBufferAliases.end())
      destinationName = stable->second;
    const auto destinationTypeIt = namedValueTypes.find(destinationName);
    const std::string destinationType =
        destinationTypeIt == namedValueTypes.end()
            ? bufferTypes.at(destinationIdentity)
            : destinationTypeIt->second;
    const std::optional<MemRefTypeModel> destination =
        ParseMemRefType(destinationType);

    const size_t input = event.generatedOrdinal;
    const std::string sourceName = valueName(source.operands[input]);
    std::string sourceType = input < source.operandTypes.size()
                                 ? source.operandTypes[input]
                                 : std::string();
    auto namedSourceType = namedValueTypes.find(sourceName);
    if (namedSourceType != namedValueTypes.end())
      sourceType = namedSourceType->second;
    else if (IsTensorType(sourceType))
      sourceType = ConvertTensorToMemRefType(sourceType);
    const std::optional<MemRefTypeModel> sourceMemref =
        ParseMemRefType(sourceType);
    if (!destination || !sourceMemref ||
        destination->shape.size() != sourceMemref->shape.size())
      throw std::runtime_error("PlanMemory input builder: malformed vconcat view");

    const std::string dimAttribute =
        FindDictionaryValue(source.properties, "dim");
    const int64_t dimValue = dimAttribute.empty() ? 0 : std::stoll(dimAttribute);
    if (dimValue < 0 || static_cast<size_t>(dimValue) >= sourceMemref->shape.size())
      throw std::runtime_error("PlanMemory input builder: invalid vconcat dimension");
    const size_t dim = static_cast<size_t>(dimValue);

    auto &sizes = vconcatInputSizes[source.id];
    if (sizes.size() < destinations[0])
      sizes.resize(destinations[0]);
    if (sizes[input].empty()) {
      if (sourceMemref->shape[dim]) {
        sizes[input] = std::to_string(*sourceMemref->shape[dim]);
      } else {
        const GenericOperation *function = EnclosingFunction(logical, source);
        auto constants = function ? integerConstants.find(function->id)
                                  : integerConstants.end();
        if (constants == integerConstants.end() ||
            constants->second.count(static_cast<int64_t>(dim)) == 0)
          throw std::runtime_error(
              "PlanMemory input builder: vconcat dimension constant is missing");
        const std::string resultName =
            "%vconcat_dim_" + std::to_string(nextScalarValue++);
        OperationRecord dimension = baseOperation(source, "memref.dim");
        dimension.text = resultName + " = memref.dim " + sourceName + ", " +
                         scalarValueName(constants->second.at(
                             static_cast<int64_t>(dim))) + " : " +
                         sourceType;
        result.operations.push_back(std::move(dimension));
        namedValueTypes[resultName] = "index";
        sizes[input] = resultName;
      }
    }

    auto &offsets = vconcatOffsets[source.id];
    if (offsets.empty())
      offsets.resize(destinations[0], "0");
    if (input == 0 && destinations[0] > 2 && offsets[2] == "0") {
      bool canCompose = !sizes[0].empty();
      int64_t staticAddend = 0;
      for (size_t index = 1; index < 2 && canCompose; ++index) {
        std::string type = index < source.operandTypes.size()
                               ? source.operandTypes[index]
                               : std::string();
        if (IsTensorType(type))
          type = ConvertTensorToMemRefType(type);
        const auto parsed = ParseMemRefType(type);
        canCompose = parsed && parsed->shape.size() > dim &&
                     parsed->shape[dim].has_value();
        if (canCompose)
          staticAddend += *parsed->shape[dim];
      }
      if (canCompose) {
        offsets[1] = sizes[0];
        const std::string resultName =
            "%vconcat_offset_" + std::to_string(nextScalarValue++);
        OperationRecord apply = baseOperation(source, "affine.apply");
        apply.text = resultName + " = affine.apply " + sizes[0] +
                     " {constant = " + std::to_string(staticAddend) + "}";
        result.operations.push_back(std::move(apply));
        namedValueTypes[resultName] = "index";
        offsets[2] = resultName;
      }
    }

    MemRefTypeModel view = *sourceMemref;
    view.strides = destination->strides;
    view.hasStridedLayout = true;
    view.offset = input == 0 ? destination->offset
                             : std::numeric_limits<int64_t>::min();
    const std::string viewType = PlanMemoryPreserveAddressSpace(
        PlanMemoryFormatMemRefType(view), destinationType);
    const std::string resultName =
        "%vconcat_subview_" + std::to_string(nextFlattenValue++);
    OperationRecord subview = baseOperation(source, "memref.subview");
    subview.text = resultName + " = memref.subview " + destinationName;
    if (input < offsets.size() && offsets[input] != "0")
      subview.text += ", " + offsets[input];
    for (size_t axis = 0; axis < sourceMemref->shape.size(); ++axis)
      if (axis == dim && !sourceMemref->shape[axis])
        subview.text += ", " + sizes[input];
    subview.text += " : " + destinationType + " -> " + viewType;
    result.operations.push_back(std::move(subview));
    namedValueTypes[resultName] = viewType;
    stableBufferAliases[destinationIdentity] = resultName;
  }

  void emitNonTargetAllocation(const GenericOperation &source,
                               size_t ordinal) {
    OperationRecord operation = baseOperation(source, "memref.alloc");
    operation.text = "%nonlocal_" + std::to_string(ordinal) +
                     " = memref.alloc() : memref<1xi8>";
    result.operations.push_back(std::move(operation));
  }

  std::vector<std::string> conditionalOperands(
      const GenericOperation &source) const {
    std::vector<std::string> values;
    for (int operand : source.operands)
      if (conditionalValues.count(operand))
        values.push_back(PlanMemoryValueName(operand));
    return values;
  }

  std::string blockArgumentAlias(const GenericOperation &source,
                                 const std::string &identity) const {
    if (source.blockId < 0)
      return {};
    const GenericBlock &block =
        logical.blocks.at(static_cast<size_t>(source.blockId));
    for (int argument : block.arguments) {
      auto buffer = valueBuffers.find(argument);
      if (buffer != valueBuffers.end() && buffer->second == identity)
        return valueName(argument);
    }
    return {};
  }

  std::string typedGroup(const std::vector<std::string> &values) const {
    std::ostringstream group;
    for (size_t index = 0; index < values.size(); ++index) {
      if (index)
        group << ", ";
      group << values[index] << " : ";
      auto type = namedValueTypes.find(values[index]);
      group << (type == namedValueTypes.end() ? "memref<1xi8>"
                                              : type->second);
    }
    return group.str();
  }

  bool outputValueHasUse(const std::string &value,
                         int definingOperation) const {
    for (const OperationRecord &operation : result.operations) {
      if (operation.index <= definingOperation)
        continue;
      size_t position = operation.text.find(value);
      while (position != std::string::npos) {
        const size_t end = position + value.size();
        const bool leftBoundary =
            position == 0 || (!std::isalnum(static_cast<unsigned char>(
                                  operation.text[position - 1])) &&
                              operation.text[position - 1] != '_' &&
                              operation.text[position - 1] != '.');
        const bool rightBoundary =
            end == operation.text.size() ||
            (!std::isalnum(static_cast<unsigned char>(operation.text[end])) &&
             operation.text[end] != '_' && operation.text[end] != '.');
        if (leftBoundary && rightBoundary)
          return true;
        position = operation.text.find(value, position + 1);
      }
    }
    return false;
  }

  std::string typedValue(const std::string &value,
                         const std::string &fallbackType) const {
    auto type = namedValueTypes.find(value);
    return value + " : " +
           (type == namedValueTypes.end() ? fallbackType : type->second);
  }

  bool emitSourceDestinationStyleAccess(
      OperationRecord &operation, const GenericOperation &source,
      const PlanMemoryInputAccessRecord &event,
      const std::map<std::string, std::string> *operandAliases) {
    if (event.generatedOperation || event.operationName != source.name ||
        !IsDestinationStyleOp(event.operationName))
      return false;
    const std::vector<size_t> destinations = DpsInitOperandIndices(
        source.name, source.operands.size(), source.properties);
    std::set<size_t> destinationSet(destinations.begin(), destinations.end());
    std::set<size_t> inputSet;
    const std::vector<size_t> segments =
        OperandSegmentSizes(source.properties);
    const size_t inputCount =
        !segments.empty() ? segments.front()
                          : (source.operands.empty() ? 0U : 1U);
    std::optional<size_t> loadInitConditionOperand;
    if (source.name == "hivm.hir.load" && segments.size() >= 6 &&
        segments[5] != 0)
      loadInitConditionOperand =
          std::accumulate(segments.begin(), segments.begin() + 5, size_t{0});
    for (size_t index = 0; index < inputCount; ++index)
      inputSet.insert(index);
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<std::string> scalarOperands;
    size_t nextEventAccess = 0;
    for (size_t index = 0; index < source.operands.size(); ++index) {
      if (loadInitConditionOperand && index == *loadInitConditionOperand)
        continue;
      std::string name = valueName(source.operands[index]);
      if (operandAliases) {
        auto valueAlias = operandAliases->find(
            "value:" + std::to_string(source.operands[index]));
        if (valueAlias != operandAliases->end())
          name = valueAlias->second;
      }
      auto buffer = valueBuffers.find(source.operands[index]);
      if (buffer != valueBuffers.end() &&
          bufferNames.count(buffer->second) != 0 &&
          nextEventAccess < event.accesses.size()) {
        const std::string &postBufferIdentity =
            event.accesses[nextEventAccess++].first;
        auto alias = operandAliases
                         ? operandAliases->find(postBufferIdentity)
                         : std::map<std::string, std::string>::const_iterator{};
        if (operandAliases && alias != operandAliases->end())
          name = alias->second;
        else if (preservedSSAValues.count(source.operands[index]) == 0)
          name = stableBufferAliases.count(postBufferIdentity) != 0
                     ? stableBufferAliases.at(postBufferIdentity)
                     : bufferNames.at(postBufferIdentity);
      }
      std::string type = index < source.operandTypes.size()
                             ? source.operandTypes[index]
                             : std::string("i64");
      if (IsTensorType(type))
        type = ConvertTensorToMemRefType(type);
      if (destinationSet.count(index))
        outputs.push_back(typedValue(name, type));
      else if (inputSet.count(index))
        inputs.push_back(typedValue(name, type));
      else
        scalarOperands.push_back(name);
    }
    operation.text = event.operationName + " ins(";
    for (size_t index = 0; index < inputs.size(); ++index) {
      if (index)
        operation.text += ", ";
      operation.text += inputs[index];
    }
    operation.text += ") outs(";
    for (size_t index = 0; index < outputs.size(); ++index) {
      if (index)
        operation.text += ", ";
      operation.text += outputs[index];
    }
    operation.text += ")";
    for (const std::string &operand : scalarOperands)
      operation.text += " " + operand;
    return true;
  }

  void emitAccess(const GenericOperation &source,
                  const PlanMemoryInputAccessRecord &event,
                  const std::vector<int> *regionOverride = nullptr,
                  const std::map<std::string, std::string> *operandAliases =
                      nullptr) {
    OperationRecord operation = baseOperation(source, event.operationName);
    if (regionOverride)
      placeInRegion(operation, *regionOverride);
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    std::vector<std::string> temporaries;
    for (const auto &[identity, effect] : event.accesses) {
      const auto alias = operandAliases
                             ? operandAliases->find(identity)
                             : std::map<std::string, std::string>::const_iterator{};
      const std::string name =
          operandAliases && alias != operandAliases->end()
              ? alias->second
              : stableBufferAliases.count(identity) != 0
                    ? stableBufferAliases.at(identity)
                    : bufferNames.at(identity);
      if (event.temporaryBuffers.count(identity))
        temporaries.push_back(name);
      else if (effect == "r")
        reads.push_back(name);
      else if (effect == "w" || effect == "rw")
        writes.push_back(name);
    }
    const std::vector<std::string> aliases =
        ((source.name == "hivm.hir.atomic_xchg" ||
          source.name == "hivm.hir.atomic_rmw" ||
          source.name == "hivm.hir.atomic_cas") &&
         event.generatedOperation)
            ? std::vector<std::string>{}
            : conditionalOperands(source);
    reads.insert(reads.end(), aliases.begin(), aliases.end());
    if ((source.name == "hivm.hir.atomic_xchg" ||
         source.name == "hivm.hir.atomic_rmw" ||
         source.name == "hivm.hir.atomic_cas") &&
        event.generatedOperation && !source.operands.empty()) {
      const std::string global = valueName(source.operands.back());
      if (event.operationName == "hivm.hir.load")
        reads.insert(reads.begin(), global);
      else if (event.operationName == "hivm.hir.store") {
        if (source.name == "hivm.hir.atomic_xchg" && !reads.empty())
          reads.front() = valueName(source.operands.front());
        writes.push_back(global);
      } else if (event.operationName == "hivm.hir.copy" && !writes.empty()) {
        writes.front() = valueName(source.operands.front());
      } else if (source.name == "hivm.hir.atomic_rmw" &&
                 event.operationName.rfind("hivm.hir.v", 0) == 0) {
        if (reads.empty())
          reads.push_back(valueName(source.operands.front()));
        else
          reads.front() = valueName(source.operands.front());
      } else if (source.name == "hivm.hir.atomic_cas" &&
                 event.operationName == "hivm.hir.vcmp" &&
                 source.operands.size() >= 2) {
        if (!reads.empty())
          reads.back() = valueName(source.operands[0]);
      } else if (source.name == "hivm.hir.atomic_cas" &&
                 event.operationName == "hivm.hir.vsel" &&
                 source.operands.size() >= 2 && reads.size() >= 2) {
        reads[1] = valueName(source.operands[1]);
      }
    }
    if (source.name == "hivm.hir.vrec" &&
        event.operationName == "hivm.hir.vbrc") {
      const GenericOperation *function = EnclosingFunction(logical, source);
      if (!function || source.operandTypes.empty())
        throw std::runtime_error("HIVMDecomposeOp: malformed vrec");
      const std::string type = ShapedElementType(source.operandTypes.front());
      auto constants = generatedFunctionConstants.find(function->id);
      if (constants == generatedFunctionConstants.end() ||
          constants->second.count("one:" + type) == 0)
        throw std::runtime_error("HIVMDecomposeOp: missing vrec constant");
      reads.insert(reads.begin(), constants->second.at("one:" + type));
    }
    if ((source.name == "hivm.hir.vcast" ||
         source.name == "hivm.hir.vcmp") &&
        event.generatedOperation && event.operationName == "hivm.hir.vbrc") {
      const GenericOperation *function = EnclosingFunction(logical, source);
      if (!function || source.operandTypes.size() < 2)
        throw std::runtime_error("HIVMDecomposeOp: malformed vcast");
      const std::string sourceType = ShapedElementType(
          source.operandTypes.front());
      const std::string targetType =
          source.name == "hivm.hir.vcast" && sourceType == "i32" ? "f32"
                                                                  : "f16";
      auto constants = generatedFunctionConstants.find(function->id);
      if (constants == generatedFunctionConstants.end() ||
          constants->second.count("zero:" + targetType) == 0)
        throw std::runtime_error("HIVMDecomposeOp: missing vcast zero");
      reads.insert(reads.begin(),
                   constants->second.at("zero:" + targetType));
    }
    if (event.decomposedScalarVSub) {
      if (source.operands.size() < 2 || source.operandTypes.size() < 2)
        throw std::runtime_error(
            "HIVMDecomposeOp: malformed scalar vsub");
      const std::string &type = source.operandTypes[1];
      const GenericOperation *scalarDefinition =
          DefiningOperation(logical, source.operands[1]);
      auto generatedNegated = generatedVSubNegatedConstants.find(source.id);
      if (generatedNegated != generatedVSubNegatedConstants.end()) {
        reads.push_back(generatedNegated->second);
      } else
      if (scalarDefinition && scalarDefinition->name == "arith.constant") {
        std::string value =
            FindDictionaryValue(scalarDefinition->properties, "value");
        if (value.empty())
          value = FindDictionaryValue(scalarDefinition->attributes, "value");
        const size_t typedSuffix = value.find(" : ");
        if (typedSuffix != std::string::npos)
          value = trim(value.substr(0, typedSuffix));
        bool isZero = false;
        try {
          size_t consumed = 0;
          isZero = std::stold(value, &consumed) == 0.0L &&
                   consumed == value.size();
        } catch (const std::exception &) {
        }
        if (isZero) {
          reads.push_back(scalarValueName(source.operands[1]));
        } else {
          std::optional<int> existingNegated;
          const GenericOperation *function =
              EnclosingFunction(logical, source);
          if (function && type.size() > 1 && type.front() == 'i') {
            try {
              const unsigned width =
                  static_cast<unsigned>(std::stoul(type.substr(1)));
              const int64_t integer = std::stoll(value);
              if (width <= 64) {
                const uint64_t mask =
                    width == 64 ? std::numeric_limits<uint64_t>::max()
                                : (uint64_t{1} << width) - 1;
                const uint64_t negated =
                    (uint64_t{0} -
                     (static_cast<uint64_t>(integer) & mask)) &
                    mask;
                auto functionConstants =
                    integerScalarConstants.find(function->id);
                if (functionConstants != integerScalarConstants.end()) {
                  auto typed = functionConstants->second.find(type);
                  if (typed != functionConstants->second.end()) {
                    auto found = typed->second.find(negated);
                    if (found != typed->second.end())
                      existingNegated = found->second;
                  }
                }
              }
            } catch (const std::exception &) {
            }
          }
          if (existingNegated) {
            reads.push_back(scalarValueName(*existingNegated));
          } else {
            const std::string negated =
                "%vsub_folded_" + std::to_string(nextScalarValue++);
            OperationRecord constant = baseOperation(source, "arith.constant");
            constant.text = negated + " = arith.constant 0 : " + type;
            result.operations.push_back(std::move(constant));
            namedValueTypes[negated] = type;
            reads.push_back(negated);
          }
        }
      } else {
        std::string zero;
        const GenericOperation *function = EnclosingFunction(logical, source);
        if (function) {
          auto functionZeros = zeroScalarConstants.find(function->id);
          if (functionZeros != zeroScalarConstants.end()) {
            auto existing = functionZeros->second.find(type);
            if (existing != functionZeros->second.end())
              zero = scalarValueName(existing->second);
          }
          if (zero.empty()) {
            auto generated = generatedZeroNames.find(function->id);
            if (generated != generatedZeroNames.end()) {
              auto existing = generated->second.find(type);
              if (existing != generated->second.end())
                zero = existing->second;
            }
          }
        }
        if (zero.empty()) {
          zero = "%vsub_zero_" + std::to_string(nextScalarValue++);
          OperationRecord constant = baseOperation(source, "arith.constant");
          constant.text = zero + " = arith.constant 0 : " + type;
          result.operations.push_back(std::move(constant));
          namedValueTypes[zero] = type;
        }
        const std::string negated =
            "%vsub_negated_" + std::to_string(nextScalarValue++);
        const std::string arithmetic =
            !type.empty() && type.front() == 'f' ? "arith.subf" : "arith.subi";
        OperationRecord subtract = baseOperation(source, arithmetic);
        subtract.text = negated + " = " + arithmetic + " " + zero + ", " +
                        scalarValueName(source.operands[1]) + " : " + type;
        result.operations.push_back(std::move(subtract));
        namedValueTypes[negated] = type;
        reads.push_back(negated);
      }
    }
    if (event.operationName == "hivm.hir.copy" && writes.empty() &&
        source.name == "scf.yield" && source.parentId >= 0) {
      const GenericOperation &parent = logical.operations.at(
          static_cast<size_t>(source.parentId));
      if (parent.name == "scf.for" && !parent.regions.empty()) {
        const GenericRegion &region = logical.regions.at(
            static_cast<size_t>(parent.regions.front()));
        const GenericBlock &block = logical.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        for (size_t index = 0; index < source.operands.size() &&
                               index + 1 < block.arguments.size();
             ++index)
          writes.push_back(valueName(block.arguments[index + 1]));
      }
    }
    if (event.operationName == "hivm.hir.vbrc" &&
        source.name == "hivm.hir.load" && source.operands.size() > 2)
      reads.insert(reads.begin(), valueName(source.operands[2]));
    if (event.operationName == "hivm.hir.copy" && !reads.empty()) {
      const std::string &sourceIdentity = event.accesses.front().first;
      const auto operandAlias =
          operandAliases
              ? operandAliases->find(sourceIdentity)
              : std::map<std::string, std::string>::const_iterator{};
      const bool hasDerivedAlias =
          operandAliases && operandAlias != operandAliases->end() &&
          operandAlias->second != bufferNames.at(sourceIdentity);
      if (!hasDerivedAlias)
        if (const std::string blockArgument =
                blockArgumentAlias(source, sourceIdentity);
            !blockArgument.empty())
          reads.front() = blockArgument;
    }
    if (emitSourceDestinationStyleAccess(operation, source, event,
                                         operandAliases)) {
      if (!temporaries.empty())
        operation.text += " temp_buffer(" + typedGroup(temporaries) + ")";
    } else if (event.operationName == "memref.store") {
      std::string memory = writes.empty() ? std::string("%external")
                                          : writes.front();
      if (event.operationName == source.name && source.operands.size() > 1 &&
          preservedSSAValues.count(source.operands[1]) != 0)
        memory = valueName(source.operands[1]);
      size_t scalarOperand = 0;
      if (source.name == "tensor.from_elements")
        scalarOperand = event.generatedOrdinal;
      const std::string scalar =
          scalarOperand < source.operands.size()
              ? scalarValueName(source.operands[scalarOperand])
              : "%scalar";
      operation.text = "memref.store " + scalar + ", " + memory;
      if (source.name == "tensor.from_elements") {
        const GenericOperation *function = EnclosingFunction(logical, source);
        auto functionConstants =
            function ? integerConstants.find(function->id)
                     : integerConstants.end();
        auto index = functionConstants == integerConstants.end()
                         ? std::map<int64_t, int>::const_iterator{}
                         : functionConstants->second.find(
                               static_cast<int64_t>(event.generatedOrdinal));
        if (functionConstants == integerConstants.end() ||
            index == functionConstants->second.end())
          throw std::runtime_error(
              "PlanMemory input builder: tensor.from_elements index constant is missing");
        operation.text += ", " + scalarValueName(index->second);
      } else {
        for (size_t index = 2; index < source.operands.size(); ++index)
          operation.text += ", " + scalarValueName(source.operands[index]);
      }
      if (namedValueTypes.count(memory) != 0)
        operation.text += " : " + namedValueTypes.at(memory);
    } else if (event.operationName == "memref.load") {
      std::string memory = reads.empty() ? std::string("%external")
                                         : reads.front();
      if (!source.operands.empty() &&
          preservedSSAValues.count(source.operands.front()) != 0)
        memory = valueName(source.operands.front());
      const std::string scalar =
          source.results.empty()
              ? "%scalar_load_" + std::to_string(nextScalarValue++)
              : PlanMemoryValueName(source.results.front());
      operation.text = scalar + " = memref.load " + memory;
      for (size_t index = 1; index < source.operands.size(); ++index)
        operation.text += ", " + scalarValueName(source.operands[index]);
      if (namedValueTypes.count(memory) != 0)
        operation.text += " : " + namedValueTypes.at(memory);
      for (int produced : source.results)
        rememberScalar(produced, scalar, source.blockId);
    } else if (event.operationName == "arith.select" &&
               source.operands.size() >= 3 && !source.results.empty()) {
      const std::string resultName =
          PlanMemoryValueName(source.results.front());
      operation.text = resultName + " = arith.select " +
                       scalarValueName(source.operands[0]) + ", " +
                       valueName(source.operands[1]) + ", " +
                       valueName(source.operands[2]);
      std::string type = source.resultTypes.empty()
                             ? source.operandTypes[1]
                             : source.resultTypes.front();
      if (IsTensorType(type))
        type = ConvertTensorToMemRefType(type);
      auto buffer = valueBuffers.find(source.results.front());
      if (buffer != valueBuffers.end())
        type = PlanMemoryPreserveAddressSpace(
            type, bufferTypes.at(buffer->second));
      else if (!event.accesses.empty())
        type = PlanMemoryPreserveAddressSpace(
            type, bufferTypes.at(event.accesses.front().first));
      operation.text += " : " + type;
      namedValueTypes[resultName] = type;
    } else if (IsDestinationStyleOp(event.operationName)) {
      operation.text = event.operationName + " ins(" + typedGroup(reads) +
                       ") outs(" + typedGroup(writes) + ")";
      if (!temporaries.empty())
        operation.text += " temp_buffer(" + typedGroup(temporaries) + ")";
    } else {
      std::vector<std::string> operands = reads;
      operands.insert(operands.end(), writes.begin(), writes.end());
      operation.text = event.operationName + "(";
      for (size_t index = 0; index < operands.size(); ++index) {
        if (index)
          operation.text += ", ";
        auto type = namedValueTypes.find(operands[index]);
        operation.text +=
            type == namedValueTypes.end()
                ? operands[index]
                : typedValue(operands[index], type->second);
      }
      operation.text += ")";
    }
    const bool preserveSourceProperties =
        event.operationName == source.name ||
        event.preserveSourceProperties;
    auto adjustedProperties = [&](std::string dictionary) {
      auto reduced = reducedUnitAxesByEvent.find(&event);
      if (reduced == reducedUnitAxesByEvent.end() || reduced->second.empty())
        return dictionary;
      for (const char *name : {"broadcast", "transpose"}) {
        if (!HasDictionaryEntry(dictionary, name))
          continue;
        std::vector<int64_t> dimensions = DecomposeI64Array(
            FindDictionaryValue(dictionary, name));
        std::vector<int64_t> adjusted;
        for (int64_t dimension : dimensions) {
          if (dimension < 0 ||
              static_cast<size_t>(dimension) >= reduced->second.size() ||
              reduced->second[static_cast<size_t>(dimension)])
            continue;
          const size_t removed = static_cast<size_t>(std::count(
              reduced->second.begin(),
              reduced->second.begin() + dimension, true));
          adjusted.push_back(dimension - static_cast<int64_t>(removed));
        }
        std::string value = "array<i64";
        if (!adjusted.empty()) {
          value += ": ";
          for (size_t index = 0; index < adjusted.size(); ++index) {
            if (index)
              value += ", ";
            value += std::to_string(adjusted[index]);
          }
        }
        value += ">";
        dictionary = SetDictionaryValue(dictionary, name, value);
      }
      return dictionary;
    };
    if (preserveSourceProperties && !source.properties.empty() &&
        operation.text.find(source.properties) == std::string::npos)
      operation.text += " " + adjustedProperties(source.properties);
    if (preserveSourceProperties && !source.attributes.empty() &&
        operation.text.find(source.attributes) == std::string::npos)
      operation.text += " " + adjustedProperties(source.attributes);
    auto limitedDimensions = [&](const char *name) {
      std::vector<int64_t> dimensions = DecomposeI64Array(
          FindDictionaryValue(source.properties, name));
      auto mapping = limitedDimensionMappings.find(&event);
      if (mapping == limitedDimensionMappings.end())
        return dimensions;
      for (int64_t &dimension : dimensions)
        if (dimension >= 0 &&
            static_cast<size_t>(dimension) < mapping->second.size())
          dimension = mapping->second[static_cast<size_t>(dimension)];
      return dimensions;
    };
    if (event.operationName == "hivm.hir.vcumsum" ||
        event.operationName == "hivm.hir.vcumprod") {
      const std::vector<int64_t> dimensions = limitedDimensions("cum_dims");
      operation.text += " cum_dims = [";
      for (size_t index = 0; index < dimensions.size(); ++index) {
        if (index)
          operation.text += ", ";
        operation.text += std::to_string(dimensions[index]);
      }
      operation.text += "]";
    }
    if (event.operationName == "hivm.hir.vreduce") {
      const std::vector<int64_t> dimensions = limitedDimensions("reduce_dims");
      operation.text += " reduce_dims = [";
      for (size_t index = 0; index < dimensions.size(); ++index) {
        if (index)
          operation.text += ", ";
        operation.text += std::to_string(dimensions[index]);
      }
      operation.text += "]";
    }
    if (event.operationName == "hivm.hir.vbrc") {
      const std::vector<int64_t> dimensions =
          limitedDimensions("broadcast_dims");
      operation.text += " broadcast_dims = [";
      for (size_t index = 0; index < dimensions.size(); ++index) {
        if (index)
          operation.text += ", ";
        operation.text += std::to_string(dimensions[index]);
      }
      operation.text += "]";
    }
    result.operations.push_back(std::move(operation));
  }

  bool isFlattenedHIVMOperation(const std::string &name) const {
    return startsWith(name, "hivm.hir.v") || name == "hivm.hir.load" ||
           name == "hivm.hir.store" || name == "hivm.hir.copy";
  }

  bool hasLiftLowestStridePattern(const std::string &name) const {
    if (name == "hivm.hir.copy" || name == "hivm.hir.vbrc" ||
        name == "hivm.hir.vreduce" || name == "hivm.hir.vtranspose" ||
        name == "hivm.hir.vmulextended" || name == "hivm.hir.vcumsum" ||
        name == "hivm.hir.vcumprod")
      return true;
    static const std::set<std::string> unsupportedVectorOps = {
        "hivm.hir.varange",      "hivm.hir.vmulext",
        "hivm.hir.vinterleave", "hivm.hir.vdeinterleave",
        "hivm.hir.vflip",        "hivm.hir.vpad",
        "hivm.hir.vconcat",      "hivm.hir.vgather",
        "hivm.hir.vgathermask",  "hivm.hir.vsort"};
    return startsWith(name, "hivm.hir.v") &&
           unsupportedVectorOps.count(name) == 0;
  }

  std::optional<uint64_t> constantIntegerValue(int value) const {
    const GenericOperation *definition = DefiningOperation(logical, value);
    if (!definition)
      return std::nullopt;
    const std::optional<ArithIntegerConstant> constant =
        ParseArithIntegerConstant(*definition);
    return constant ? std::optional<uint64_t>(constant->bits) : std::nullopt;
  }

  bool shouldSkipFlatteningLoad(const GenericOperation &source) const {
    if (source.name != "hivm.hir.load")
      return false;

    std::vector<size_t> segments = OperandSegmentSizes(source.properties);
    if (segments.empty())
      segments = OperandSegmentSizes(source.attributes);
    if (segments.size() < 4 || segments[3] == 0)
      return false;

    const size_t leftPaddingOperand =
        segments[0] + segments[1] + segments[2];
    if (leftPaddingOperand >= source.operands.size())
      throw std::runtime_error(
          "FlattenOps: malformed hivm.load left_padding_num segment");
    const std::optional<uint64_t> leftPadding =
        constantIntegerValue(source.operands[leftPaddingOperand]);
    if (leftPadding && *leftPadding == 0)
      return false;

    if (segments[0] >= source.operands.size())
      throw std::runtime_error(
          "FlattenOps: malformed hivm.load destination segment");
    const GenericOperation *destination =
        DefiningOperation(logical, source.operands[segments[0]]);
    if (!destination || destination->name != "memref.subview")
      return true;

    std::string staticOffsets =
        FindDictionaryValue(destination->properties, "static_offsets");
    if (staticOffsets.empty())
      staticOffsets =
          FindDictionaryValue(destination->attributes, "static_offsets");
    const std::vector<int64_t> offsets = DecomposeI64Array(staticOffsets);
    if (offsets.empty())
      return true;
    if (offsets.back() != std::numeric_limits<int64_t>::min())
      return offsets.back() != 0;

    std::vector<size_t> subviewSegments =
        OperandSegmentSizes(destination->properties);
    if (subviewSegments.empty())
      subviewSegments = OperandSegmentSizes(destination->attributes);
    if (subviewSegments.size() < 2)
      return true;
    size_t dynamicOffset = 0;
    for (size_t axis = 0; axis + 1 < offsets.size(); ++axis)
      dynamicOffset +=
          offsets[axis] == std::numeric_limits<int64_t>::min() ? 1 : 0;
    const size_t offsetOperand = subviewSegments[0] + dynamicOffset;
    if (dynamicOffset >= subviewSegments[1] ||
        offsetOperand >= destination->operands.size())
      return true;
    const std::optional<uint64_t> lastOffset =
        constantIntegerValue(destination->operands[offsetOperand]);
    return !lastOffset || *lastOffset != 0;
  }

  std::map<std::string, std::string>
  emitFlattenAliases(const GenericOperation &source,
                     const PlanMemoryInputAccessRecord &event,
                     const std::vector<int> *regionOverride = nullptr,
                     const std::map<std::string, std::string> *initialAliases =
                         nullptr) {
    std::map<std::string, std::string> aliases =
        initialAliases ? *initialAliases
                       : std::map<std::string, std::string>{};
    if (!isFlattenedHIVMOperation(event.operationName))
      return aliases;
    if (event.operationName == source.name &&
        shouldSkipFlatteningLoad(source))
      return aliases;

    struct Operand {
      std::string identity;
      std::string name;
      std::string type;
      int sourceValue = -1;
      bool materializeAlignedView = false;
    };
    std::vector<MemRefTypeModel> operandTypes;
    std::vector<Operand> operands;
    const bool generatedStoreNeedsSourceView = [&]() {
      if (source.name != "hivm.hir.store" || source.operands.empty())
        return false;
      if (source.operands.size() < 2)
        return false;
      const GenericOperation *destination =
          DefiningOperation(logical, source.operands[1]);
      if (destination && destination->name == "bufferization.to_tensor" &&
          !destination->operands.empty())
        destination = DefiningOperation(logical,
                                          destination->operands.front());
      if (!destination ||
          destination->name != "memref_ext.alloc_workspace")
        return false;
      return !latestSubviewForValue(source.operands.front()).empty();
    }();
    const bool useSourceOperands =
        (!event.generatedOperation || generatedStoreNeedsSourceView) &&
        (event.operationName == source.name ||
         (source.name == "tensor.insert_slice" &&
          !event.outOfPlaceCopy));
    if (useSourceOperands) {
      size_t accessIndex = 0;
      for (size_t index = 0; index < source.operandTypes.size(); ++index) {
        std::string type = source.operandTypes[index];
        if (IsTensorType(type))
          type = ConvertTensorToMemRefType(type);
        std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
        if (!parsed)
          continue;
        std::string identity;
        std::string name = index < source.operands.size()
                               ? valueName(source.operands[index])
                               : std::string("%external");
        if (source.name == "hivm.hir.store" &&
            index < source.operands.size()) {
          const std::string resultAlias =
              latestSubviewForValue(source.operands[index]);
          if (!resultAlias.empty())
            name = resultAlias;
        }
        if (index < source.operands.size() && initialAliases) {
          auto initial = initialAliases->find(
              "value:" + std::to_string(source.operands[index]));
          if (initial != initialAliases->end())
            name = initial->second;
        }
        bool materializeAlignedView =
            index < source.operands.size() &&
            preservedSSAValues.count(source.operands[index]) == 0 &&
            !(source.name == "hivm.hir.store" &&
              !latestSubviewForValue(source.operands[index]).empty());
        if (index < source.operands.size() &&
            preservedSSAValues.count(source.operands[index]) != 0) {
          auto namedType = namedValueTypes.find(name);
          if (namedType != namedValueTypes.end()) {
            type = namedType->second;
            parsed = ParseMemRefType(type);
          }
        }
        if (index < source.operands.size()) {
          auto buffer = valueBuffers.find(source.operands[index]);
          if (buffer != valueBuffers.end()) {
            bool preserveInsertSourceSubview = false;
            // PlanMemoryInput records the post-bufferization operand order.  The original
            // tensor value binding can name an older alias after DPS conflict
            // copies, so use it only to identify target-buffer positions.
            if (accessIndex >= event.accesses.size())
              throw std::runtime_error(
                  "PlanMemory input builder: missing bufferized operand access");
            identity = event.accesses[accessIndex++].first;
            const bool replacedByOutOfPlaceBuffer =
                std::any_of(
                    module.afterMarkMultiBuffer.afterInlineLoadCopy
                        .afterAllocExtraBuffer.postBufferization.bufferized
                        .allocations.begin(),
                    module.afterMarkMultiBuffer.afterInlineLoadCopy
                        .afterAllocExtraBuffer.postBufferization.bufferized
                        .allocations.end(),
                    [&](const BufferAllocation &allocation) {
                      return allocation.source ==
                             "operand:" + std::to_string(source.id) + ":" +
                                 std::to_string(index);
                    });
            if (source.name == "tensor.insert_slice" && index == 0 &&
                !event.outOfPlaceCopy) {
              for (auto emitted = result.operations.rbegin();
                   emitted != result.operations.rend(); ++emitted) {
                if (emitted->opName != "memref.subview" ||
                    emitted->text.find(bufferNames.at(identity)) ==
                        std::string::npos)
                  continue;
                const std::vector<std::string> results =
                    operationResultNames(*emitted);
                if (results.empty())
                  continue;
                auto typeIt = namedValueTypes.find(results.front());
                const auto candidate =
                    typeIt == namedValueTypes.end()
                        ? std::nullopt
                        : ParseMemRefType(typeIt->second);
                if (candidate && candidate->shape == parsed->shape) {
                  name = results.front();
                  type = typeIt->second;
                  parsed = candidate;
                  materializeAlignedView = false;
                  preserveInsertSourceSubview = true;
                  break;
                }
              }
            }
            const bool preserveOriginalSSA =
                !replacedByOutOfPlaceBuffer &&
                (preservedSSAValues.count(source.operands[index]) != 0 ||
                 preserveInsertSourceSubview ||
                 (source.name == "hivm.hir.store" &&
                  !latestSubviewForValue(source.operands[index]).empty()) ||
                 (source.name == "hivm.hir.vbrc" && index == 0 &&
                  erasedValueAliases.count(source.operands[index]) != 0));
            if (!preserveOriginalSSA)
              name = bufferNames.at(identity);
            auto stable = stableBufferAliases.find(identity);
            bool stableRankCompatible = true;
            if (stable != stableBufferAliases.end() &&
                (event.operationName == "hivm.hir.vreduce" ||
                 event.operationName == "hivm.hir.vbrc" ||
                 event.operationName == "hivm.hir.vcumsum" ||
                 event.operationName == "hivm.hir.vcumprod")) {
              auto stableType = namedValueTypes.find(stable->second);
              const std::optional<MemRefTypeModel> parsedStable =
                  stableType == namedValueTypes.end()
                      ? std::nullopt
                      : ParseMemRefType(stableType->second);
              stableRankCompatible =
                  !parsedStable || parsedStable->shape.size() ==
                                       parsed->shape.size();
            }
            const bool resultFeedsInsertSlice = std::any_of(
                source.results.begin(), source.results.end(), [&](int resultValue) {
                  auto users = valueUsers.find(resultValue);
                  if (users == valueUsers.end())
                    return false;
                  return std::any_of(users->second.begin(), users->second.end(),
                                     [&](int userId) {
                    const GenericOperation &user = logical.operations.at(
                        static_cast<size_t>(userId));
                    return user.name == "tensor.insert_slice" &&
                           !user.operands.empty() &&
                           user.operands.front() == resultValue;
                  });
                });
            if (stable != stableBufferAliases.end() &&
                resultFeedsInsertSlice) {
              auto stableType = namedValueTypes.find(stable->second);
              const auto parsedStable =
                  stableType == namedValueTypes.end()
                      ? std::nullopt
                      : ParseMemRefType(stableType->second);
              stableRankCompatible =
                  !parsedStable || parsedStable->shape == parsed->shape;
            }
            const bool usesStableAlias =
                stable != stableBufferAliases.end() &&
                stableRankCompatible &&
                (!preserveOriginalSSA ||
                 (source.name == "tensor.insert_slice" &&
                  [&]() {
                    const std::vector<size_t> destinations =
                        DpsInitOperandIndices(
                            source.name, source.operands.size(),
                            source.properties);
                    return std::find(destinations.begin(), destinations.end(),
                                     index) != destinations.end();
                  }()));
            if (usesStableAlias) {
              name = stable->second;
              materializeAlignedView = false;
              type = namedValueTypes.at(name);
              parsed = ParseMemRefType(type);
            }
            auto record = finalBufferRecords.find(identity);
            if (record != finalBufferRecords.end() && !usesStableAlias) {
              const bool staticShape = std::all_of(
                  parsed->shape.begin(), parsed->shape.end(),
                  [](const std::optional<int64_t> &extent) {
                    return extent.has_value();
                  });
              if (staticShape) {
                const MemRefTypeModel aligned = AlignedOperandType(
                    *record->second, *parsed,
                    module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.alignStorage);
                type = PlanMemoryPreserveAddressSpace(
                    PlanMemoryFormatMemRefType(aligned),
                    bufferTypes.at(identity));
                parsed = ParseMemRefType(type);
              }
            }
          }
        }
        auto effectiveType = namedValueTypes.find(name);
        if (effectiveType != namedValueTypes.end()) {
          type = effectiveType->second;
          parsed = ParseMemRefType(type);
        }
        operandTypes.push_back(*parsed);
        operands.push_back(
            {std::move(identity), std::move(name), std::move(type),
             index < source.operands.size() ? source.operands[index] : -1,
             materializeAlignedView});
      }
    } else {
      size_t accessIndex = 0;
      for (const auto &[identity, effect] : event.accesses) {
        (void)effect;
        if (event.temporaryBuffers.count(identity) != 0)
          continue;
        std::string type = bufferTypes.at(identity);
        std::string name = bufferNames.at(identity);
        const auto initialAlias = aliases.find(identity);
        const bool hasInitialAlias = initialAlias != aliases.end();
        if (hasInitialAlias) {
          name = initialAlias->second;
          const auto initialType = namedValueTypes.find(name);
          if (initialType != namedValueTypes.end())
            type = initialType->second;
        }
        std::string materializedSourceView;
        if ((source.name == "tensor.extract_slice" ||
             source.name == "tensor.insert_slice") &&
            !source.operandTypes.empty()) {
          std::string expectedType = source.operandTypes.front();
          if (IsTensorType(expectedType))
            expectedType = ConvertTensorToMemRefType(expectedType);
          const auto expected = ParseMemRefType(expectedType);
          for (auto operation = result.operations.rbegin();
               operation != result.operations.rend(); ++operation) {
            if (operation->opName != "memref.subview" ||
                operation->text.find(bufferNames.at(identity)) ==
                    std::string::npos)
              continue;
            const std::vector<std::string> results =
                operationResultNames(*operation);
            if (results.empty())
              continue;
            auto typeIt = namedValueTypes.find(results.front());
            const auto candidate = typeIt == namedValueTypes.end()
                                       ? std::nullopt
                                       : ParseMemRefType(typeIt->second);
            if (expected && candidate &&
                expected->shape == candidate->shape) {
              materializedSourceView = results.front();
              break;
            }
          }
        }
        const bool outOfPlaceSliceSource =
            (source.name == "tensor.extract_slice" ||
             source.name == "tensor.insert_slice") &&
            event.operationName == "hivm.hir.copy" &&
            !event.outOfPlaceCopy &&
            accessIndex == 0 && !source.operands.empty() &&
            !materializedSourceView.empty();
        if (outOfPlaceSliceSource) {
          name = materializedSourceView;
          type = namedValueTypes.at(name);
        }
        const bool vconcatSource =
            source.name == "hivm.hir.vconcat" &&
            event.operationName == "hivm.hir.copy" && accessIndex == 0 &&
            event.generatedOrdinal < source.operands.size();
        if (vconcatSource) {
          name = valueName(source.operands[event.generatedOrdinal]);
          auto sourceType = namedValueTypes.find(name);
          if (sourceType != namedValueTypes.end())
            type = sourceType->second;
          else if (event.generatedOrdinal < source.operandTypes.size()) {
            type = source.operandTypes[event.generatedOrdinal];
            if (IsTensorType(type))
              type = ConvertTensorToMemRefType(type);
          }
        }
        auto stable = stableBufferAliases.find(identity);
        if (stable != stableBufferAliases.end() && !hasInitialAlias &&
            !vconcatSource &&
            !outOfPlaceSliceSource) {
          name = stable->second;
          type = namedValueTypes.at(name);
        }
        std::optional<MemRefTypeModel> parsed = ParseMemRefType(type);
        if (!parsed)
          throw std::runtime_error(
              "PlanMemory input builder: malformed vector operand type");
        operandTypes.push_back(*parsed);
        operands.push_back(
            {identity, std::move(name), std::move(type), -1, false});
        ++accessIndex;
      }
    }
    if (operandTypes.empty())
      return aliases;

    std::vector<size_t> originalRanks;
    originalRanks.reserve(operandTypes.size());
    for (const MemRefTypeModel &type : operandTypes)
      originalRanks.push_back(type.shape.size());
    size_t rank = 0;
    for (const MemRefTypeModel &type : operandTypes)
      rank = std::max(rank, type.shape.size());
    for (MemRefTypeModel &type : operandTypes)
      while (type.shape.size() < rank) {
        std::optional<int64_t> leadingStride = std::nullopt;
        if (!type.shape.empty() && type.shape.front() &&
            !type.strides.empty() && type.strides.front())
          leadingStride = *type.shape.front() * *type.strides.front();
        type.shape.insert(type.shape.begin(), 1);
        type.strides.insert(type.strides.begin(), leadingStride);
        type.hasStridedLayout = true;
      }
    auto emitAlias = [&](const std::string &operationName,
                         const std::string &operandName,
                         const std::string &type,
                         const std::string &prefix,
                         const std::string &sourceType = std::string(),
                         const std::string &semanticProperties =
                             std::string()) {
      std::string effectiveOperandName = operandName;
      std::string effectiveSourceType = sourceType;
      std::string effectiveSemanticProperties = semanticProperties;
      std::string cseKey;
      if (operationName == "memref.collapse_shape") {
        auto priorCollapse = composedCollapseShapes.find(operandName);
        if (priorCollapse != composedCollapseShapes.end()) {
          auto existingType = namedValueTypes.find(operandName);
          const auto priorSourceMemRef =
              ParseMemRefType(priorCollapse->second.sourceType);
          const auto priorResultMemRef =
              existingType == namedValueTypes.end()
                  ? std::nullopt
                  : ParseMemRefType(existingType->second);
          const auto requestedMemRef = ParseMemRefType(type);
          const bool directAllocationSource = std::any_of(
              bufferNames.begin(), bufferNames.end(), [&](const auto &entry) {
                return entry.second == priorCollapse->second.source;
              });
          const bool composableLayouts =
              priorSourceMemRef && priorResultMemRef && requestedMemRef &&
              (directAllocationSource ||
               (!priorSourceMemRef->hasStridedLayout &&
                !priorResultMemRef->hasStridedLayout &&
                !requestedMemRef->hasStridedLayout));
          if (existingType != namedValueTypes.end()) {
            const auto existingMemRef = ParseMemRefType(existingType->second);
            if (PhysicalTypeSignature(existingType->second) ==
                    PhysicalTypeSignature(type) ||
                (initialAliases && existingMemRef && requestedMemRef &&
                 existingMemRef->shape == requestedMemRef->shape &&
                 existingMemRef->elementType ==
                     requestedMemRef->elementType &&
                 existingMemRef->addressSpace ==
                     requestedMemRef->addressSpace))
              return operandName;
          }
          if (composableLayouts) {
            if (!outputValueHasUse(operandName,
                                   priorCollapse->second.operation))
              erasedOutputOperations.insert(priorCollapse->second.operation);
            effectiveOperandName = priorCollapse->second.source;
            effectiveSourceType = priorCollapse->second.sourceType;
          }
        }
        auto expand = composedExpandShapes.find(operandName);
        const std::optional<MemRefTypeModel> expandSource =
            expand == composedExpandShapes.end()
                ? std::nullopt
                : ParseMemRefType(expand->second.sourceType);
        const std::optional<MemRefTypeModel> collapseResult =
            ParseMemRefType(type);
        if (expandSource && collapseResult &&
            expandSource->shape == collapseResult->shape &&
            expandSource->elementType == collapseResult->elementType &&
            expandSource->addressSpace == collapseResult->addressSpace) {
          if ((source.name != "hivm.hir.vconcat" ||
               !expand->second.dynamic) &&
              !outputValueHasUse(operandName, expand->second.operation))
            erasedOutputOperations.insert(expand->second.operation);
          return expand->second.source;
        }
        if (expand != composedExpandShapes.end() && collapseResult &&
            collapseResult->shape.size() == 1) {
          if ((source.name != "hivm.hir.vconcat" ||
               !expand->second.dynamic) &&
              !outputValueHasUse(operandName, expand->second.operation))
            erasedOutputOperations.insert(expand->second.operation);
          effectiveOperandName = expand->second.source;
          effectiveSourceType = expand->second.sourceType;
          effectiveSemanticProperties.clear();
        }
        if (effectiveSemanticProperties.empty()) {
          const std::vector<int> aliasPath =
              regionOverride ? *regionOverride : regionPath(source);
          for (const auto &[existingResult, existing] :
               composedCollapseShapes) {
            if (existing.source != effectiveOperandName ||
                PhysicalTypeSignature(existing.sourceType) !=
                    PhysicalTypeSignature(effectiveSourceType))
              continue;
            auto existingType = namedValueTypes.find(existingResult);
            if (existingType == namedValueTypes.end() ||
                PhysicalTypeSignature(existingType->second) !=
                    PhysicalTypeSignature(type))
              continue;
            const OperationRecord &existingOperation = result.operations.at(
                static_cast<size_t>(existing.operation));
            if (existingOperation.regionPath.size() <= aliasPath.size() &&
                std::equal(existingOperation.regionPath.begin(),
                           existingOperation.regionPath.end(),
                           aliasPath.begin()))
              return existingResult;
          }
        }
        const std::vector<int> aliasPath =
            regionOverride ? *regionOverride : regionPath(source);
        cseKey = operationName + "\n" + effectiveOperandName + "\n" +
                 PhysicalTypeSignature(effectiveSourceType) + "\n" +
                 PhysicalTypeSignature(type);
        if (composedExpandShapes.count(operandName) != 0)
          cseKey += "\n" + effectiveSemanticProperties;
        auto collapseAliases = collapseCseAliases.find(cseKey);
        if (collapseAliases != collapseCseAliases.end()) {
          std::vector<int> regionPrefix = aliasPath;
          while (true) {
            auto existing = collapseAliases->second.find(regionPrefix);
            if (existing != collapseAliases->second.end())
              return existing->second;
            if (regionPrefix.empty())
              break;
            regionPrefix.pop_back();
          }
        }
      }
      OperationRecord operation = baseOperation(source, operationName);
      if (regionOverride)
        placeInRegion(operation, *regionOverride);
      const std::string resultName =
          "%" + prefix + "_" + std::to_string(nextFlattenValue++);
      operation.text = resultName + " = " + operationName + " " +
                       effectiveOperandName + " : ";
      if (!effectiveSourceType.empty())
        operation.text += effectiveSourceType + " -> ";
      operation.text += type;
      if (!effectiveSemanticProperties.empty() &&
          composedExpandShapes.count(operandName) != 0) {
        const size_t equal = operation.text.find('=');
        operation.normalizationKey =
            trim(operation.text.substr(equal + 1)) + "\n" +
            effectiveSemanticProperties;
      }
      result.operations.push_back(std::move(operation));
      if (operationName == "memref.collapse_shape") {
        const std::vector<int> aliasPath =
            regionOverride ? *regionOverride : regionPath(source);
        collapseCseAliases[cseKey][aliasPath] = resultName;
        composedCollapseShapes[resultName] =
            {effectiveOperandName, effectiveSourceType,
             result.operations.back().index, false};
      }
      return resultName;
    };

    for (size_t index = 0; index < operands.size(); ++index) {
      Operand &operand = operands[index];
      if (operand.identity.empty())
        continue;
      const MemRefTypeModel root =
          *ParseMemRefType(bufferTypes.at(operand.identity));
      if (operand.materializeAlignedView &&
          root.shape != operandTypes[index].shape) {
        operand.name = emitAlias("memref.subview", operand.name,
                                 operand.type, "aligned_view",
                                 bufferTypes.at(operand.identity));
        namedValueTypes[operand.name] = operand.type;
        stableBufferAliases[operand.identity] = operand.name;
      }
    }

    std::vector<std::vector<size_t>> reassociation =
        UniformReassociationPipeline(operandTypes);
    const bool dimensionLimitedOperation =
        event.operationName == "hivm.hir.vreduce" ||
        event.operationName == "hivm.hir.vbrc" ||
        event.operationName == "hivm.hir.vcumsum" ||
        event.operationName == "hivm.hir.vcumprod";
    if (dimensionLimitedOperation) {
      const std::string dimensionName =
          event.operationName == "hivm.hir.vreduce"
              ? "reduce_dims"
              : event.operationName == "hivm.hir.vbrc" ? "broadcast_dims"
                                                         : "cum_dims";
      std::string dimensions =
          FindDictionaryValue(source.properties, dimensionName);
      if (dimensions.empty())
        dimensions = FindDictionaryValue(source.attributes, dimensionName);
      reassociation = UniformReassociationPipelineWithBarriers(
          operandTypes, DecomposeI64Array(dimensions));
    }
    if (event.operationName == "hivm.hir.vtranspose") {
      const std::vector<int64_t> permutation = DecomposeI64Array(
          FindDictionaryValue(source.properties, "permutation"));
      if (permutation.size() == rank) {
        std::vector<size_t> outputPosition(rank);
        for (size_t outputAxis = 0; outputAxis < rank; ++outputAxis)
          outputPosition.at(static_cast<size_t>(permutation[outputAxis])) =
              outputAxis;
        std::vector<std::vector<size_t>> refined;
        for (const std::vector<size_t> &group : reassociation) {
          for (size_t axis : group) {
            if (refined.empty() || refined.back().empty() ||
                outputPosition[refined.back().back()] + 1 !=
                    outputPosition[axis])
              refined.push_back({});
            refined.back().push_back(axis);
          }
        }
        reassociation = std::move(refined);
      }
    }
    if (dimensionLimitedOperation) {
      std::vector<int64_t> mapping(rank, -1);
      for (size_t group = 0; group < reassociation.size(); ++group)
        for (size_t originalAxis : reassociation[group])
          if (originalAxis < mapping.size())
            mapping[originalAxis] = static_cast<int64_t>(group);
      limitedDimensionMappings[&event] = std::move(mapping);
    }
    std::vector<bool> reducedUnitAxes;
    if (reassociation.size() == rank && rank > 1 &&
        !dimensionLimitedOperation &&
        event.operationName != "hivm.hir.vtranspose") {
      std::vector<bool> unit(rank, true);
      for (size_t axis = 0; axis < rank; ++axis)
        for (const MemRefTypeModel &type : operandTypes)
          unit[axis] = unit[axis] && type.shape[axis] &&
                       *type.shape[axis] == 1;
      if (std::count(unit.begin(), unit.end(), true) != 0 &&
          std::count(unit.begin(), unit.end(), false) != 0) {
        reducedUnitAxes = unit;
        reassociation.clear();
        std::vector<size_t> leadingUnits;
        for (size_t axis = 0; axis < rank; ++axis) {
          if (unit[axis]) {
            if (reassociation.empty())
              leadingUnits.push_back(axis);
            else
              reassociation.back().push_back(axis);
            continue;
          }
          reassociation.push_back(std::move(leadingUnits));
          leadingUnits.clear();
          reassociation.back().push_back(axis);
        }
      }
    }

    if (reassociation.size() != rank) {
      std::ostringstream reassociationKey;
      reassociationKey << "reassociation=";
      for (size_t groupIndex = 0; groupIndex < reassociation.size();
           ++groupIndex) {
        if (groupIndex)
          reassociationKey << ';';
        for (size_t axisIndex = 0;
             axisIndex < reassociation[groupIndex].size(); ++axisIndex) {
          if (axisIndex)
            reassociationKey << ',';
          reassociationKey << reassociation[groupIndex][axisIndex];
        }
      }
      for (size_t index = 0; index < operands.size(); ++index) {
        Operand &operand = operands[index];
        if (originalRanks[index] == reassociation.size()) {
          operandTypes[index] = *ParseMemRefType(operand.type);
          continue;
        }
        const std::string sourceType = operand.type;
        if (reducedUnitAxes.empty()) {
          operandTypes[index] =
              CollapseMemRefType(operandTypes[index], reassociation);
        } else {
          MemRefTypeModel reduced = operandTypes[index];
          reduced.shape.clear();
          reduced.strides.clear();
          for (size_t axis = 0; axis < reducedUnitAxes.size(); ++axis)
            if (!reducedUnitAxes[axis]) {
              reduced.shape.push_back(operandTypes[index].shape[axis]);
              reduced.strides.push_back(operandTypes[index].strides[axis]);
            }
          reduced.hasStridedLayout = true;
          operandTypes[index] = std::move(reduced);
        }
        operand.type = PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(operandTypes[index]), operand.type);
        const std::string resultName = emitAlias(
            "memref.collapse_shape", operand.name, operand.type, "flatten",
            sourceType, reassociationKey.str());
        operand.name = resultName;
        if (namedValueTypes.count(resultName) == 0)
          namedValueTypes[resultName] = operand.type;
      }
    }
    reducedUnitAxesByEvent[&event] = reducedUnitAxes;

    bool allLastDimensionsContiguous = true;
    for (const MemRefTypeModel &type : operandTypes)
      allLastDimensionsContiguous =
          allLastDimensionsContiguous && IsLastDimContiguous(type);
    const bool liftLowestStride =
        hasLiftLowestStridePattern(event.operationName) &&
        !operandTypes.empty() && !allLastDimensionsContiguous;
    if (liftLowestStride) {
      for (size_t index = 0; index < operands.size(); ++index) {
        Operand &operand = operands[index];
        OperationRecord metadata =
            baseOperation(source, "memref.extract_strided_metadata");
        if (regionOverride)
          placeInRegion(metadata, *regionOverride);
        const size_t arity = 2 + 2 * operandTypes[index].shape.size();
        const std::string metadataName =
            "%metadata_" + std::to_string(nextFlattenValue++);
        const std::string baseType = PlanMemoryPreserveAddressSpace(
            "memref<" + operandTypes[index].elementType + ">", operand.type);
        metadata.text = metadataName + ":" + std::to_string(arity) +
                        " = memref.extract_strided_metadata " + operand.name +
                        " : " + operand.type + " -> " + baseType;
        result.operations.push_back(std::move(metadata));
        operandTypes[index].shape.push_back(1);
        operandTypes[index].strides.push_back(1);
        operandTypes[index].hasStridedLayout = true;
        operand.type = PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(operandTypes[index]), operand.type);
        std::string metadataOperands = metadataName + "#0";
        const size_t originalRank = operandTypes[index].shape.size() - 1;
        if (operandTypes[index].offset ==
            std::numeric_limits<int64_t>::min())
          metadataOperands += ", " + metadataName + "#1";
        for (size_t axis = 0; axis < originalRank; ++axis)
          if (!operandTypes[index].shape[axis])
            metadataOperands += ", " + metadataName + "#" +
                                std::to_string(2 + axis);
        for (size_t axis = 0; axis < originalRank; ++axis)
          if (!operandTypes[index].strides[axis])
            metadataOperands += ", " + metadataName + "#" +
                                std::to_string(2 + originalRank + axis);
        operand.name = emitAlias("memref.reinterpret_cast",
                                 metadataOperands, operand.type,
                                 "lifted", baseType);
        namedValueTypes[operand.name] = operand.type;
      }
    }

    for (size_t index = 0; index < operands.size(); ++index) {
      const Operand &operand = operands[index];
      if (!operand.identity.empty())
        aliases[operand.identity] = operand.name;
      if (useSourceOperands && operand.sourceValue >= 0)
        aliases["value:" + std::to_string(operand.sourceValue)] =
            operand.name;
    }
    return aliases;
  }

  void emitSynthetic(const GenericOperation &source, const std::string &name,
                     const std::vector<int> &path, int operationId) {
    OperationRecord operation = baseOperation(source, name);
    operation.operationId = operationId;
    if (path != regionPath(source))
      placeInRegion(operation, path);
    operation.text = name;
    result.operations.push_back(std::move(operation));
  }

  void emitSyncBlockLowering(const GenericOperation &source) {
    std::string mode =
        DecomposeEnumValue(FindDictionaryValue(source.properties,
                                                "sync_block_mode"));
    if (mode.empty())
      mode = DecomposeEnumValue(
          FindDictionaryValue(source.attributes, "sync_block_mode"));
    emitSynthetic(source, "hivm.hir.pipe_barrier", regionPath(source),
                  nextOperationId++);
    if (mode == "BARRIER_CUBE" || mode == "BARRIER_VECTOR")
      return;
    if (mode != "ALL" && mode != "ALL_CUBE" && mode != "ALL_VECTOR" &&
        mode != "ALL_SUB_VECTOR")
      throw std::runtime_error(
          "PlanMemory input builder: unsupported sync_block mode " + mode);
    emitSynthetic(source, "hivm.hir.sync_block_set", regionPath(source),
                  nextOperationId++);
    emitSynthetic(source, "hivm.hir.sync_block_wait", regionPath(source),
                  nextOperationId++);
  }

  void emitAtomicLockStart(const GenericOperation &source) {
    const GenericOperation *function = EnclosingFunction(logical, source);
    if (!function || function->regions.empty())
      throw std::runtime_error("HIVMDecomposeOp: atomic outside function");
    const GenericRegion &region = logical.regions.at(
        static_cast<size_t>(function->regions.front()));
    if (region.blocks.empty())
      throw std::runtime_error("HIVMDecomposeOp: atomic function has no block");
    const GenericBlock &entry =
        logical.blocks.at(static_cast<size_t>(region.blocks.front()));
    if (entry.arguments.size() < 2)
      throw std::runtime_error("HIVMDecomposeOp: missing sync-lock argument");
    auto constants = integerConstants.find(function->id);
    if (constants == integerConstants.end() ||
        constants->second.count(0) == 0)
      throw std::runtime_error("HIVMDecomposeOp: missing lock offset zero");
    const std::string lock =
        "%atomic_lock_" + std::to_string(nextFlattenValue++);
    const std::string lockType =
        "memref<1xi64, #hivm.address_space<gm>>";
    OperationRecord view = baseOperation(source, "memref.view");
    view.text = lock + " = memref.view " +
                PlanMemoryValueName(entry.arguments[1]) + ", " +
                scalarValueName(constants->second.at(0)) +
                " : memref<?xi8, #hivm.address_space<gm>> -> " + lockType;
    result.operations.push_back(std::move(view));
    namedValueTypes[lock] = lockType;
    atomicLockValues[source.id] = lock;

    OperationRecord acquire =
        baseOperation(source, "hivm.hir.sync_block_lock");
    acquire.text = "hivm.hir.sync_block_lock " + lock + " : " + lockType;
    result.operations.push_back(std::move(acquire));
  }

  void emitAtomicLockEnd(const GenericOperation &source) {
    auto lock = atomicLockValues.find(source.id);
    if (lock == atomicLockValues.end())
      return;
    OperationRecord release =
        baseOperation(source, "hivm.hir.sync_block_unlock");
    release.text = "hivm.hir.sync_block_unlock " + lock->second + " : " +
                   namedValueTypes.at(lock->second);
    result.operations.push_back(std::move(release));
  }

  void emitControlStart(const GenericOperation &source) {
    OperationRecord operation = baseOperation(source, source.name);
    operation.operationId = source.id;
    std::ostringstream text;
    auto copiedInits = controlInitAliases.find(source.id);
    bool hasResult = false;
    for (size_t index = 0; index < source.results.size(); ++index) {
      if (isRemovedIfResult(source, index)) {
        const int resultValue = source.results[index];
        auto buffer = bufferizedBufferIds.find(resultValue);
        if (buffer != bufferizedBufferIds.end()) {
          auto representative =
              bufferRepresentativeValues.find(buffer->second);
          if (representative != bufferRepresentativeValues.end() &&
              representative->second != resultValue)
            erasedValueAliases[resultValue] =
                valueName(representative->second);
        }
        continue;
      }
      const bool shaped = index < source.resultTypes.size() &&
                          (IsTensorType(source.resultTypes[index]) ||
                           IsMemRefType(source.resultTypes[index]));
      const bool canonicalizedResult =
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.canonicalizedIterArgResultKeys.count(
              {source.id, static_cast<int>(index)}) != 0;
      const bool canonicalizedAccess =
          shaped &&
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.canonicalizedIterArgAccesses.count(
              {source.id, static_cast<int>(index + 3)}) != 0;
      if (canonicalizedResult || canonicalizedAccess)
        continue;
      if (hasResult)
        text << ", ";
      text << PlanMemoryValueName(source.results[index]);
      hasResult = true;
    }
    if (hasResult)
      text << " = ";
    if (source.name == "scf.for") {
      text << "scf.for " << PlanMemoryValueName(-source.id - 1)
           << " = "
           << (source.operands.empty() ? "%lb"
                                       : scalarValueName(source.operands[0]))
           << " to "
           << (source.operands.size() < 2
                   ? "%ub"
                   : scalarValueName(source.operands[1]))
           << " step "
           << (source.operands.size() < 3
                   ? "%step"
                   : scalarValueName(source.operands[2]));
      bool hasScalarIterArg = false;
      for (size_t index = 3; index < source.operandTypes.size(); ++index) {
        const bool shaped = IsTensorType(source.operandTypes[index]) ||
                            IsMemRefType(source.operandTypes[index]);
        const bool canonicalizedResult =
            module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                .canonicalizedIterArgResultKeys.count(
                    {source.id, static_cast<int>(index - 3)}) != 0;
        const bool canonicalizedAccess =
            shaped &&
            module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                .canonicalizedIterArgAccesses.count(
                    {source.id, static_cast<int>(index)}) != 0;
        hasScalarIterArg =
            hasScalarIterArg || (!canonicalizedResult &&
                                 !canonicalizedAccess);
      }
      if (hasScalarIterArg && !source.regions.empty()) {
        const GenericRegion &region = logical.regions.at(
            static_cast<size_t>(source.regions.front()));
        const GenericBlock &block = logical.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        text << " iter_args(";
        bool firstIterArg = true;
        for (size_t index = 3; index < source.operands.size(); ++index) {
          const bool shaped = index < source.operandTypes.size() &&
                              (IsTensorType(source.operandTypes[index]) ||
                               IsMemRefType(source.operandTypes[index]));
          const bool canonicalizedResult =
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                  .canonicalizedIterArgResultKeys.count(
                      {source.id, static_cast<int>(index - 3)}) != 0;
          const bool canonicalizedAccess =
              shaped &&
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                  .canonicalizedIterArgAccesses.count(
                      {source.id, static_cast<int>(index)}) != 0;
          if (canonicalizedResult || canonicalizedAccess)
            continue;
          if (!firstIterArg)
            text << ", ";
          const size_t argument = index - 2;
          std::string init = valueName(source.operands[index]);
          if (copiedInits != controlInitAliases.end()) {
            auto copied = copiedInits->second.find(index);
            if (copied != copiedInits->second.end())
              init = copied->second;
          }
          text << PlanMemoryValueName(block.arguments.at(argument)) << " = "
               << init;
          firstIterArg = false;
        }
        text << ')';
        for (size_t index = 3; index < source.operands.size() &&
                               index < source.operandTypes.size();
             ++index) {
          const bool shaped = IsTensorType(source.operandTypes[index]) ||
                              IsMemRefType(source.operandTypes[index]);
          if (!shaped ||
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                      .canonicalizedIterArgResultKeys.count(
                          {source.id, static_cast<int>(index - 3)}) != 0 ||
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                      .canonicalizedIterArgAccesses.count(
                          {source.id, static_cast<int>(index)}) != 0)
            continue;
          text << " : "
               << valueType(source.operands[index], source.operandTypes[index]);
        }
      }
      if (!source.regions.empty()) {
        const GenericRegion &region = logical.regions.at(
            static_cast<size_t>(source.regions.front()));
        const GenericBlock &block = logical.blocks.at(
            static_cast<size_t>(region.blocks.front()));
        for (size_t index = 3; index < source.operands.size(); ++index) {
          const size_t resultIndex = index - 3;
          const size_t argumentIndex = index - 2;
          if (index >= source.operandTypes.size() ||
              (!IsTensorType(source.operandTypes[index]) &&
               !IsMemRefType(source.operandTypes[index])) ||
              argumentIndex >= block.arguments.size() ||
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                      .canonicalizedIterArgResultKeys.count(
                          {source.id, static_cast<int>(resultIndex)}) == 0)
            continue;
          const std::string init = valueName(source.operands[index]);
          canonicalViewAliases[block.arguments[argumentIndex]] = init;
          if (resultIndex < source.results.size())
            canonicalViewAliases[source.results[resultIndex]] = init;
        }
      }
    } else if (source.name == "scf.while") {
      text << "scf.while (";
      for (size_t index = 0; index < source.operands.size(); ++index) {
        if (index)
          text << ", ";
        text << PlanMemoryValueName(-source.id * 1000 -
                                    static_cast<int>(index) - 1)
             << " = " << valueName(source.operands[index]);
      }
      text << ')';
    } else if (source.name == "scope.scope") {
      text << "scope.scope";
    } else {
      text << "scf.if "
           << (source.operands.empty() ? "%condition"
                                       : valueName(source.operands.front()));
      if (hasResult) {
        text << " -> (";
        bool firstType = true;
        for (size_t index = 0; index < source.results.size(); ++index) {
          if (isRemovedIfResult(source, index))
            continue;
          const bool shaped = index < source.resultTypes.size() &&
                              (IsTensorType(source.resultTypes[index]) ||
                               IsMemRefType(source.resultTypes[index]));
          if (shaped &&
              module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                      .canonicalizedIterArgResultKeys.count(
                          {source.id, static_cast<int>(index)}) != 0)
            continue;
          if (!firstType)
            text << ", ";
          std::string resultType = bufferizedResultType(source, index);
          auto buffer = valueBuffers.find(source.results[index]);
          if (buffer != valueBuffers.end()) {
            std::optional<MemRefTypeModel> resultMemref =
                ParseMemRefType(resultType);
            const std::optional<MemRefTypeModel> physicalMemref =
                ParseMemRefType(bufferTypes.at(buffer->second));
            if (resultMemref && physicalMemref) {
              resultMemref->addressSpace = physicalMemref->addressSpace;
              resultType =
                  PlanMemoryFormatMemRefTypeWithAddressSpace(*resultMemref);
            }
          } else {
            auto rawBuffer = bufferizedBufferIds.find(source.results[index]);
            std::set<AddressSpace> candidateSpaces;
            if (rawBuffer != bufferizedBufferIds.end()) {
              for (const std::string &alternative :
                   BufferAlternatives(rawBuffer->second)) {
                const std::string sourceIdentity = MappedBufferIdentity(
                    alternative,
                    module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.bufferMapping);
                if (targetSourceIdentities.count(sourceIdentity) == 0)
                  continue;
                const std::string &identity =
                    finalIdentity.at(sourceIdentity);
                const std::optional<MemRefTypeModel> physicalMemref =
                    ParseMemRefType(bufferTypes.at(identity));
                if (physicalMemref)
                  candidateSpaces.insert(physicalMemref->addressSpace);
              }
            }
            std::optional<MemRefTypeModel> resultMemref =
                ParseMemRefType(resultType);
            if (resultMemref && candidateSpaces.size() == 1) {
              resultMemref->addressSpace = *candidateSpaces.begin();
              resultType =
                  PlanMemoryFormatMemRefTypeWithAddressSpace(*resultMemref);
            }
          }
          if (IsTensorType(source.resultTypes[index])) {
            std::optional<MemRefTypeModel> resultMemref =
                ParseMemRefType(resultType);
            if (resultMemref) {
              resultMemref->addressSpace = AddressSpace::UB;
              resultType =
                  PlanMemoryFormatMemRefTypeWithAddressSpace(*resultMemref);
            }
          }
          namedValueTypes[PlanMemoryValueName(source.results[index])] =
              resultType;
          text << resultType;
          firstType = false;
        }
        text << ')';
      }
    }
    operation.text = text.str();
    result.operations.push_back(std::move(operation));
  }

  void emitTerminator(const GenericOperation &source) {
    OperationRecord operation = baseOperation(source, source.name);
    operation.text = source.name;
    bool emittedOperand = false;
    for (size_t index = 0; index < source.operands.size(); ++index) {
      if (source.parentId >= 0 &&
          isRemovedIfResult(
              logical.operations.at(static_cast<size_t>(source.parentId)),
              index))
        continue;
      const bool canonicalizedResult =
          source.parentId >= 0 &&
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.canonicalizedIterArgResultKeys.count(
              {source.parentId, static_cast<int>(index)}) != 0;
      if (canonicalizedResult ||
          module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.canonicalizedIterArgAccesses.count(
              {source.id, static_cast<int>(index)}) != 0)
        continue;
      std::string name = valueName(source.operands[index]);
      operation.text += " " + name;
      emittedOperand = true;
      auto type = namedValueTypes.find(name);
      if (type != namedValueTypes.end() && ParseMemRefType(type->second))
        operation.text += " : " + type->second;
    }
    if (source.name == "scf.yield" && !emittedOperand && source.parentId >= 0) {
      const std::string &parentName =
          logical.operations.at(static_cast<size_t>(source.parentId)).name;
      if (parentName == "scf.for" || parentName == "scf.if") {
        operation.opName = parentName + ".implicit_yield";
        operation.text = operation.opName;
      }
    }
    result.operations.push_back(std::move(operation));
  }

  std::string scalarArithmeticName(const GenericOperation &source) const {
    const std::string element =
        source.operandTypes.empty()
            ? std::string()
            : (ParseMemRefType(ConvertTensorToMemRefType(
                   source.operandTypes.front())))
                  ->elementType;
    const bool floating = element == "f32";
    const bool unsignedInteger = startsWith(element, "ui");
    if (source.name == "hivm.hir.vadd")
      return floating ? "arith.addf" : "arith.addi";
    if (source.name == "hivm.hir.vsub")
      return floating ? "arith.subf" : "arith.subi";
    if (source.name == "hivm.hir.vmul")
      return floating ? "arith.mulf" : "arith.muli";
    if (source.name == "hivm.hir.vdiv")
      return floating ? "arith.divf"
                      : (unsignedInteger ? "arith.divui" : "arith.divsi");
    if (source.name == "hivm.hir.vabs")
      return floating ? "math.absf" : "math.absi";
    if (source.name == "hivm.hir.vsqrt")
      return "math.sqrt";
    if (source.name == "hivm.hir.vmax")
      return floating ? "arith.maximumf" : "arith.maxsi";
    if (source.name == "hivm.hir.vmin")
      return floating ? "arith.minimumf" : "arith.minsi";
    throw std::runtime_error("PlanMemory input builder: unsupported scalarized op " +
                             source.name);
  }

  void emitScalarStore(const GenericOperation &source,
                       const PlanMemoryInputAccessRecord &event,
                       const std::string &scalar,
                       const std::vector<int> &indices) {
    OperationRecord operation = baseOperation(source, "memref.store");
    std::string destination;
    for (const auto &[identity, effect] : event.accesses)
      if (effect == "w" || effect == "rw") {
        destination = bufferNames.at(identity);
        break;
      }
    operation.text = "memref.store " + scalar + ", " +
                     (destination.empty() ? "%external" : destination);
    for (int index : indices)
      operation.text += ", " + scalarValueName(index);
    if (!destination.empty()) {
      auto type = namedValueTypes.find(destination);
      if (type != namedValueTypes.end())
        operation.text += " : " + type->second;
    }
    result.operations.push_back(std::move(operation));
  }

  std::string emitScalarLoad(const GenericOperation &source,
                             const PlanMemoryInputAccessRecord &event,
                             const std::vector<int> &indices,
                             std::optional<int> memoryValue = std::nullopt) {
    OperationRecord operation = baseOperation(source, "memref.load");
    std::string sourceBuffer;
    for (const auto &[identity, effect] : event.accesses)
      if (effect == "r" || effect == "rw") {
        sourceBuffer = bufferNames.at(identity);
        break;
      }
    if (memoryValue && scalarValues.count(*memoryValue) == 0)
      sourceBuffer = valueName(*memoryValue);
    const std::string scalar =
        "%scalar_load_" + std::to_string(nextScalarValue++);
    operation.text = scalar + " = memref.load " +
                     (sourceBuffer.empty() ? "%external" : sourceBuffer);
    for (int index : indices)
      operation.text += ", " + scalarValueName(index);
    if (!sourceBuffer.empty())
      operation.text += " : " + namedValueTypes.at(sourceBuffer);
    result.operations.push_back(std::move(operation));
    return scalar;
  }

  std::vector<int> singlePointIndices(
      const GenericOperation &source) const {
    const GenericOperation *function = EnclosingFunction(logical, source);
    if (!function)
      throw std::runtime_error(
          "PlanMemory input builder: scalar operation has no enclosing function");
    auto constants = integerConstants.find(function->id);
    if (constants == integerConstants.end() ||
        constants->second.count(0) == 0)
      throw std::runtime_error(
          "PlanMemory input builder: scalar index zero constant is missing");
    return {constants->second.at(0)};
  }

  void rememberScalar(int value, const std::string &scalar, int block) {
    scalarValues[value] = scalar;
    scalarValueBlocks[value] = block;
  }

  void emitScalarizedOperation(const GenericOperation &source) {
    const auto events = eventsBySource.find(source.id);
    if (source.name == "tensor.insert") {
      if (!source.results.empty() && !source.operands.empty())
        rememberScalar(source.results.front(),
                       scalarValueName(source.operands.front()),
                       source.blockId);
      if (events != eventsBySource.end()) {
        std::vector<int> indices;
        for (size_t index = 2; index < source.operands.size(); ++index)
          indices.push_back(source.operands[index]);
        for (const PlanMemoryInputAccessRecord *event : events->second)
          if (event->operationName == "memref.store")
            emitScalarStore(source, *event,
                            scalarValueName(source.operands.front()), indices);
      }
      return;
    }
    if (source.name == "tensor.extract") {
      if (source.results.empty() || source.operands.empty())
        throw std::runtime_error(
            "PlanMemory input builder: malformed scalar tensor.extract");
      std::string scalar = scalarValueName(source.operands.front());
      if (events != eventsBySource.end()) {
        std::vector<int> indices;
        for (size_t index = 1; index < source.operands.size(); ++index)
          indices.push_back(source.operands[index]);
        for (const PlanMemoryInputAccessRecord *event : events->second)
          if (event->operationName == "memref.load")
            scalar = emitScalarLoad(source, *event, indices,
                                    source.operands.front());
      }
      rememberScalar(source.results.front(), scalar, source.blockId);
      return;
    }
    if (source.name == "hivm.hir.vbrc") {
      if (source.operands.empty())
        throw std::runtime_error("PlanMemory input builder: malformed scalarized vbrc");
      const std::string scalar = scalarValueName(source.operands.front());
      for (int produced : source.results)
        rememberScalar(produced, scalar, source.blockId);
      if (events != eventsBySource.end())
        for (const PlanMemoryInputAccessRecord *event : events->second)
          if (event->operationName == "memref.store")
            emitScalarStore(source, *event, scalar,
                            singlePointIndices(source));
      return;
    }
    const std::vector<size_t> destinations = DpsInitOperandIndices(
        source.name, source.operands.size(), source.properties);
    std::set<size_t> destinationSet(destinations.begin(), destinations.end());
    std::vector<const PlanMemoryInputAccessRecord *> inputLoads;
    if (events != eventsBySource.end())
      for (const PlanMemoryInputAccessRecord *event : events->second)
        if (event->operationName == "memref.load")
          inputLoads.push_back(event);
    size_t nextInputLoad = 0;
    std::vector<std::string> operands;
    for (size_t index = 0; index < source.operands.size(); ++index) {
      if (destinationSet.count(index) != 0)
        continue;
      const bool shaped = index < source.operandTypes.size() &&
                          (IsTensorType(source.operandTypes[index]) ||
                           IsMemRefType(source.operandTypes[index]));
      if (shaped && scalarValues.count(source.operands[index]) != 0 &&
          scalarValueBlocks.at(source.operands[index]) == source.blockId)
        operands.push_back(scalarValueName(source.operands[index]));
      else if (shaped && nextInputLoad < inputLoads.size())
        operands.push_back(
            emitScalarLoad(source, *inputLoads[nextInputLoad++],
                           singlePointIndices(source), source.operands[index]));
      else
        operands.push_back(scalarValueName(source.operands[index]));
    }
    if (source.results.empty())
      throw std::runtime_error(
          "PlanMemory input builder: scalarized operation has no result");
    const std::string scalar =
        "%scalarized_" + std::to_string(nextScalarValue++);
    OperationRecord operation =
        baseOperation(source, scalarArithmeticName(source));
    operation.text = scalar + " = " + operation.opName;
    for (const std::string &operand : operands)
      operation.text += " " + operand;
    result.operations.push_back(std::move(operation));
    for (int produced : source.results)
      rememberScalar(produced, scalar, source.blockId);
    if (events != eventsBySource.end()) {
      for (const PlanMemoryInputAccessRecord *event : events->second) {
        if (event->operationName == "memref.store")
          emitScalarStore(source, *event, scalar, singlePointIndices(source));
      }
    }
  }

  std::string bufferizedViewName(const std::string &name) const {
    if (name == "tensor.extract_slice")
      return "memref.subview";
    if (name == "tensor.collapse_shape")
      return "memref.collapse_shape";
    if (name == "tensor.expand_shape")
      return "memref.expand_shape";
    if (name == "tensor.cast")
      return "memref.cast";
    return name;
  }

  bool isErasedTensorOperation(const GenericOperation &source) const {
    static const std::set<std::string> erased = {
        "tensor.empty", "bufferization.alloc_tensor",
        "bufferization.to_tensor", "tensor.insert", "tensor.extract",
        "tensor.from_elements"};
    if (erased.count(source.name) != 0)
      return true;
    static const std::set<std::string> tensorViews = {
        "tensor.cast", "tensor.collapse_shape", "tensor.expand_shape",
        "tensor.extract_slice"};
    return tensorViews.count(source.name) != 0 && !source.results.empty() &&
           preservedSSAValues.count(source.results.front()) == 0;
  }

  bool isRemovedIfResult(const GenericOperation &operation,
                         size_t resultIndex) const {
    if (operation.name != "scf.if" || resultIndex >= operation.results.size())
      return false;
    const int resultValue = operation.results[resultIndex];
    const bool shaped = resultIndex < operation.resultTypes.size() &&
                        (IsTensorType(operation.resultTypes[resultIndex]) ||
                         IsMemRefType(operation.resultTypes[resultIndex]));
    const auto users = valueUsers.find(resultValue);
    if (!shaped)
      return users == valueUsers.end() || users->second.empty();
    // OneShotBufferize rewrites users of an unambiguous tensor result to its
    // unique equivalent buffer.  The resulting memref IfOp result is then
    // removed by scf::RemoveUnusedResults in ExtendedCanonicalizer.
    if (shaped && unambiguousBufferizedValues.count(resultValue) != 0 &&
        conditionalValues.count(resultValue) == 0)
      return true;
    if (users == valueUsers.end() || users->second.empty())
      return true;
    for (int userId : users->second) {
      if (erasedOperations.count(userId) != 0)
        continue;
      const GenericOperation &user =
          logical.operations.at(static_cast<size_t>(userId));
      if (!isErasedTensorOperation(user))
        return false;
    }
    return true;
  }

  void emitPassthrough(const GenericOperation &source) {
    if (isErasedTensorOperation(source)) {
      if ((source.name == "bufferization.to_tensor" ||
           source.name == "bufferization.to_memref") &&
          !source.operands.empty())
        for (int produced : source.results)
          erasedValueAliases[produced] = valueName(source.operands.front());
      return;
    }
    if (source.name == "memref.alloc" || source.name == "memref.alloca")
      return;
    if (source.name == "annotation.mark") {
      const bool strideAlign =
          !FindDictionaryValue(source.attributes,
                               "hivm.stride_align_dims").empty() ||
          !FindDictionaryValue(source.properties,
                               "hivm.stride_align_dims").empty();
      const bool survivesAlignRemoval =
          !FindDictionaryValue(source.attributes,
                               "hivm.multi_buffer").empty() ||
          !FindDictionaryValue(source.properties,
                               "hivm.multi_buffer").empty() ||
          !FindDictionaryValue(source.attributes,
                               "hivm.preload_local_buffer").empty() ||
          !FindDictionaryValue(source.properties,
                               "hivm.preload_local_buffer").empty();
      if (strideAlign && !survivesAlignRemoval)
        return;
    }
    const std::string affineName = arithToAffineName(source);
    const std::string name = source.name == "func.return"
                                 ? "return"
                                 : affineName.empty()
                                       ? bufferizedViewName(source.name)
                                       : affineName;
    const bool cseView = name == "memref.subview" ||
                         name == "memref.collapse_shape" ||
                         name == "memref.expand_shape";
    std::string viewKey;
    if (cseView && source.results.size() == 1) {
      if (name == "memref.collapse_shape" && !source.operands.empty() &&
          !source.operandTypes.empty() && !source.resultTypes.empty()) {
        const std::string operandName = valueName(source.operands.front());
        std::string sourceType =
            valueType(source.operands.front(), source.operandTypes.front());
        auto namedSource = namedValueTypes.find(operandName);
        if (namedSource != namedValueTypes.end())
          sourceType = namedSource->second;
        const std::string targetType = bufferizedResultType(source, 0);
        const std::string collapseKey =
            name + "\n" + operandName + "\n" +
            PhysicalTypeSignature(sourceType) + "\n" +
            PhysicalTypeSignature(targetType);
        auto existing = collapseCseAliases.find(collapseKey);
        if (existing != collapseCseAliases.end()) {
          std::vector<int> prefix = regionPath(source);
          while (true) {
            auto alias = existing->second.find(prefix);
            if (alias != existing->second.end()) {
              canonicalViewAliases[source.results.front()] = alias->second;
              return;
            }
            if (prefix.empty())
              break;
            prefix.pop_back();
          }
        }
      }
      viewKey = name + "\n" + source.properties + "\n" + source.attributes;
      for (int region : regionPath(source))
        viewKey += "\nregion:" + std::to_string(region);
      for (int operand : source.operands)
        viewKey += "\n" + valueName(operand);
      viewKey += "\n" + PhysicalTypeSignature(
                              bufferizedResultType(source, 0));
      auto existing = cseAliases.find(viewKey);
      if (existing != cseAliases.end()) {
        canonicalViewAliases[source.results.front()] = existing->second;
        return;
      }
    }
    OperationRecord operation = baseOperation(source, name);
    auto passthroughOperandName = [&](size_t index) {
      std::string operandName = valueName(source.operands[index]);
      const bool rankSensitiveSubview =
          source.name == "tensor.extract_slice" ||
          source.name == "memref.subview";
      if (!rankSensitiveSubview || index >= source.operandTypes.size())
        return operandName;
      std::string expectedType = source.operandTypes[index];
      if (IsTensorType(expectedType))
        expectedType = ConvertTensorToMemRefType(expectedType);
      const auto expected = ParseMemRefType(expectedType);
      auto named = namedValueTypes.find(operandName);
      const auto actual = named == namedValueTypes.end()
                              ? std::nullopt
                              : ParseMemRefType(named->second);
      if (!expected || !actual || expected->shape.size() == actual->shape.size())
        return operandName;
      auto buffer = valueBuffers.find(source.operands[index]);
      return buffer == valueBuffers.end() ? operandName
                                           : bufferNames.at(buffer->second);
    };
    std::ostringstream text;
    for (size_t index = 0; index < source.results.size(); ++index) {
      if (index)
        text << ", ";
      text << PlanMemoryValueName(source.results[index]);
    }
    if (!source.results.empty())
      text << " = ";
    text << name;
    if (!affineName.empty()) {
      std::vector<std::string> affineOperands;
      const auto replacement =
          arithToAffine.replacementOperands.find(source.id);
      if (replacement == arithToAffine.replacementOperands.end())
        throw std::runtime_error(
            "ConvertArithToAffine: missing replacement operands");
      for (int operand : replacement->second)
        affineOperands.push_back(valueName(operand));
      if (!affineOperands.empty())
        text << ' ' << JoinDelimited(affineOperands, ", ");
    } else if (!source.operands.empty()) {
      text << ' ';
      bool firstOperand = true;
      for (size_t index = 0; index < source.operands.size(); ++index) {
        const bool offsetSizeStrideOperation =
            source.name == "tensor.extract_slice" ||
            source.name == "memref.subview" ||
            source.name == "tensor.insert_slice";
        if (offsetSizeStrideOperation &&
            foldedScalarConstants.count(source.operands[index]) != 0)
          continue;
        if (!firstOperand)
          text << ", ";
        if (source.name == "hivm.hir.bitcast") {
          auto buffer = valueBuffers.find(source.operands[index]);
          if (buffer != valueBuffers.end()) {
            auto stable = stableBufferAliases.find(buffer->second);
            text << (stable == stableBufferAliases.end()
                         ? bufferNames.at(buffer->second)
                         : stable->second);
          } else {
            text << valueName(source.operands[index]);
          }
        } else {
          text << passthroughOperandName(index);
        }
        firstOperand = false;
      }
    }
    if (source.name == "annotation.mark") {
      const std::string multi =
          FindDictionaryValue(source.attributes, "hivm.multi_buffer");
      if (!multi.empty())
        text << " {hivm.multi_buffer = " << multi << '}';
      if (!FindDictionaryValue(source.attributes,
                               "hivm.preload_local_buffer").empty())
        text << " {hivm.preload_local_buffer}";
    }
    if (source.name == "memref.subview" &&
        (source.properties.find("to_be_bubbled_slice") != std::string::npos ||
         source.attributes.find("to_be_bubbled_slice") != std::string::npos))
      text << " {to_be_bubbled_slice}";
    if (startsWith(source.name, "affine.")) {
      if (!source.properties.empty())
        text << ' ' << source.properties;
      if (!source.attributes.empty() &&
          source.attributes != source.properties)
        text << ' ' << source.attributes;
    }
    std::vector<std::string> operandTypes;
    for (size_t index = 0; index < source.operands.size() &&
                           index < source.operandTypes.size();
         ++index)
      {
        const std::string operandName = passthroughOperandName(index);
        auto namedType = namedValueTypes.find(operandName);
        operandTypes.push_back(
            namedType == namedValueTypes.end()
                ? valueType(source.operands[index], source.operandTypes[index])
                : namedType->second);
      }
    std::vector<std::string> resultTypes;
    for (size_t index = 0; index < source.results.size() &&
                           index < source.resultTypes.size();
         ++index)
      resultTypes.push_back(
          bufferizedResultType(source, index));
    if (!operandTypes.empty() || !resultTypes.empty()) {
      text << " : (" << JoinDelimited(operandTypes, ", ") << ") -> ("
           << JoinDelimited(resultTypes, ", ") << ')';
    }
    operation.text = text.str();
    const size_t normalizationEqual = operation.text.find('=');
    operation.normalizationKey =
        (normalizationEqual == std::string::npos
             ? operation.text
             : trim(operation.text.substr(normalizationEqual + 1))) +
        "\nproperties=" + source.properties +
        "\nattributes=" + source.attributes;
    if (!affineName.empty())
      operation.normalizationKey +=
          "\naffine_map=" + AffineMapSemanticKey(source, arithToAffine);
    result.operations.push_back(std::move(operation));
    if (name == "memref.expand_shape" && source.results.size() == 1 &&
        !source.operands.empty()) {
      const std::string resultName =
          PlanMemoryValueName(source.results.front());
      composedExpandShapes[resultName] =
          {valueName(source.operands.front()),
           operandTypes.empty() ? std::string() : operandTypes.front(),
           result.operations.back().index, source.operands.size() > 1};
    }
    if (name == "memref.collapse_shape" && source.results.size() == 1 &&
        !source.operands.empty()) {
      const std::string resultName =
          PlanMemoryValueName(source.results.front());
      composedCollapseShapes[resultName] =
          {valueName(source.operands.front()),
           operandTypes.empty() ? std::string() : operandTypes.front(),
           result.operations.back().index, false};
      auto buffer = valueBuffers.find(source.operands.front());
      if (buffer != valueBuffers.end())
        stableBufferAliases[buffer->second] =
            PlanMemoryValueName(source.results.front());
    }
    if (name == "memref.subview" && source.results.size() == 1) {
      auto buffer = valueBuffers.find(source.results.front());
      if (buffer == valueBuffers.end() && !source.operands.empty())
        buffer = valueBuffers.find(source.operands.front());
      if (buffer != valueBuffers.end())
        latestSubviewAliases[buffer->second] =
            PlanMemoryValueName(source.results.front());
      if (!source.operands.empty())
        latestSubviewAliasesByBackingName[passthroughOperandName(0)] =
            PlanMemoryValueName(source.results.front());
    }
    if (!viewKey.empty())
      cseAliases[viewKey] = PlanMemoryValueName(source.results.front());
    for (size_t index = 0; index < source.results.size() &&
                           index < source.resultTypes.size();
         ++index) {
      std::string type = bufferizedResultType(source, index);
      namedValueTypes[PlanMemoryValueName(source.results[index])] = type;
    }
  }

  void emitBoundary(const GenericOperation &source,
                    const std::string &name, int regionId = -1) {
    OperationRecord operation = baseOperation(source, name);
    operation.operationId = source.id;
    if (regionId >= 0) {
      operation.regionPath = regionPath(source);
    }
    operation.text = "<" + name + ">";
    result.operations.push_back(std::move(operation));
  }

  std::string emitScalarOperation(const GenericOperation &source,
                                  const std::string &name,
                                  const std::vector<std::string> &operands,
                                  const std::string &type) {
    const std::string value =
        "%decompose_scalar_" + std::to_string(nextScalarValue++);
    OperationRecord operation = baseOperation(source, name);
    operation.text = value + " = " + name;
    for (const std::string &operand : operands)
      operation.text += " " + operand;
    operation.text += " : " + type;
    result.operations.push_back(std::move(operation));
    namedValueTypes[value] = type;
    return value;
  }

  void emitExtendedMultiply(const GenericOperation &source) {
    if (source.operands.size() < 2 || source.results.size() != 2 ||
        source.operandTypes.empty() || source.operandTypes.front() != "i32")
      throw std::runtime_error("HIVMDecomposeOp: malformed extended multiply");
    const bool isUnsigned = source.name == "arith.mului_extended";
    const GenericOperation *function = EnclosingFunction(logical, source);
    if (!function)
      throw std::runtime_error("HIVMDecomposeOp: extended multiply outside function");
    auto constants = generatedFunctionConstants.find(function->id);
    if (constants == generatedFunctionConstants.end() ||
        constants->second.count("extended-multiply-shift") == 0)
      throw std::runtime_error("HIVMDecomposeOp: missing shift constant");

    std::vector<std::string> extendedOperands;
    for (int operand : source.operands) {
      const std::string key =
          (isUnsigned ? "extui:" : "extsi:") + std::to_string(operand);
      auto folded = constants->second.find(key);
      if (folded != constants->second.end()) {
        extendedOperands.push_back(folded->second);
        continue;
      }
      extendedOperands.push_back(emitScalarOperation(
          source, isUnsigned ? "arith.extui" : "arith.extsi",
          {scalarValueName(operand)}, "i64"));
    }
    const std::string product = emitScalarOperation(
        source, "arith.muli", extendedOperands, "i64");
    const std::string shift =
        constants->second.at("extended-multiply-shift");
    auto isUsed = [&](int value) {
      auto users = valueUsers.find(value);
      return users != valueUsers.end() && !users->second.empty();
    };
    if (isUsed(source.results[0])) {
      const std::string shiftedLeft = emitScalarOperation(
          source, "arith.shli", {product, shift}, "i64");
      const std::string low = emitScalarOperation(
          source, isUnsigned ? "arith.shrui" : "arith.shrsi",
          {shiftedLeft, shift}, "i64");
      const std::string truncated = emitScalarOperation(
          source, "arith.trunci", {low}, "i32");
      rememberScalar(source.results[0], truncated, source.blockId);
    }
    if (isUsed(source.results[1])) {
      const std::string high = emitScalarOperation(
          source, isUnsigned ? "arith.shrui" : "arith.shrsi",
          {product, shift}, "i64");
      const std::string truncated = emitScalarOperation(
          source, "arith.trunci", {high}, "i32");
      rememberScalar(source.results[1], truncated, source.blockId);
    }
  }

  bool isDeadPureView(const GenericOperation &source) const {
    static const std::set<std::string> pureViews = {
        "tensor.cast",          "tensor.collapse_shape",
        "tensor.expand_shape", "tensor.extract_slice",
        "memref.cast",          "memref.collapse_shape",
        "memref.expand_shape", "memref.subview",
        "memref.reinterpret_cast", "memref.view"};
    if (pureViews.count(source.name) == 0 || source.results.empty())
      return false;
    std::set<int> visiting;
    std::function<bool(int)> valueIsDead = [&](int value) {
      auto users = valueUsers.find(value);
      if (users == valueUsers.end() || users->second.empty())
        return true;
      for (int userId : users->second) {
        if (erasedOperations.count(userId) != 0)
          continue;
        const GenericOperation &user =
            logical.operations.at(static_cast<size_t>(userId));
        if (isErasedTensorOperation(user))
          continue;
        if (pureViews.count(user.name) == 0 ||
            !visiting.insert(userId).second)
          return false;
        bool deadUser = !user.results.empty();
        for (int producedValue : user.results)
          deadUser = deadUser && valueIsDead(producedValue);
        visiting.erase(userId);
        if (!deadUser)
          return false;
      }
      return true;
    };
    for (int resultValue : source.results)
      if (!valueIsDead(resultValue))
        return false;
    return true;
  }

  const GenericOperation *
  findInPlaceInsertSliceForProducer(const GenericOperation &producer) const {
    if (producer.name != "hivm.hir.load" || producer.operands.size() < 2)
      return nullptr;
    for (const GenericOperation &toTensor : logical.operations) {
      if (toTensor.name != "bufferization.to_tensor" ||
          toTensor.operands.size() != 1 || toTensor.results.size() != 1 ||
          toTensor.operands.front() != producer.operands[1])
        continue;
      for (const GenericOperation &insertSlice : logical.operations) {
        if (insertSlice.name != "tensor.insert_slice" ||
            insertSlice.operands.empty() ||
            insertSlice.operands.front() != toTensor.results.front())
          continue;
        auto events = eventsBySource.find(insertSlice.id);
        if (events == eventsBySource.end() || events->second.empty())
          return &insertSlice;
      }
    }
    return nullptr;
  }

  const GenericOperation *
  findInsertSliceDestinationAllocation(const GenericOperation &insertSlice) const {
    const GenericOperation *current = &insertSlice;
    std::set<int> visited;
    while (current && current->name == "tensor.insert_slice" &&
           current->operands.size() >= 2 && visited.insert(current->id).second) {
      current = DefiningOperation(logical, current->operands[1]);
    }
    return current && current->name == "tensor.empty" ? current : nullptr;
  }

  void emitInsertSliceProducerSubview(
      const GenericOperation &insertSlice,
      const GenericOperation &producer,
      const PlanMemoryInputAccessRecord &event,
      std::map<std::string, std::string> &aliases) {
    std::string destinationIdentity;
    for (const auto &[identity, effect] : event.accesses)
      if (effect == "w" || effect == "rw") {
        destinationIdentity = identity;
        break;
      }
    if (destinationIdentity.empty() || insertSlice.operandTypes.empty())
      return;
    std::string sourceTypeText = insertSlice.operandTypes.front();
    if (IsTensorType(sourceTypeText))
      sourceTypeText = ConvertTensorToMemRefType(sourceTypeText);
    const std::optional<MemRefTypeModel> sourceType =
        ParseMemRefType(sourceTypeText);
    const std::optional<MemRefTypeModel> destinationType =
        ParseMemRefType(bufferTypes.at(destinationIdentity));
    if (!sourceType || !destinationType ||
        sourceType->shape.size() > destinationType->strides.size())
      throw std::runtime_error(
          "PlanMemory input builder: malformed producer insert_slice subview");
    MemRefTypeModel view = *sourceType;
    view.addressSpace = destinationType->addressSpace;
    view.hasStridedLayout = true;
    view.strides.assign(destinationType->strides.begin(),
                        destinationType->strides.begin() +
                            static_cast<std::ptrdiff_t>(view.shape.size()));
    std::vector<int64_t> staticSizes = DecomposeI64Array(
        FindDictionaryValue(insertSlice.properties, "static_sizes"));
    const std::vector<size_t> segments =
        OperandSegmentSizes(insertSlice.properties);
    if (segments.size() >= 4) {
      size_t dynamicSizeOperand = 2 + segments[2];
      for (int64_t &size : staticSizes) {
        if (size != std::numeric_limits<int64_t>::min())
          continue;
        if (dynamicSizeOperand >= insertSlice.operands.size())
          break;
        auto folded = foldedScalarConstants.find(
            insertSlice.operands[dynamicSizeOperand++]);
        if (folded != foldedScalarConstants.end())
          size = folded->second;
      }
    }
    if (staticSizes.size() == view.shape.size())
      for (size_t axis = 0; axis < staticSizes.size(); ++axis)
        if (staticSizes[axis] != std::numeric_limits<int64_t>::min())
          view.shape[axis] = staticSizes[axis];
    view.offset = destinationType->offset;
    const std::vector<int64_t> offsets = DecomposeI64Array(
        FindDictionaryValue(insertSlice.properties, "static_offsets"));
    if (offsets.size() == view.shape.size() &&
        view.offset != std::numeric_limits<int64_t>::min()) {
      for (size_t axis = 0; axis < offsets.size(); ++axis) {
        if (offsets[axis] == std::numeric_limits<int64_t>::min() ||
            !destinationType->strides[axis]) {
          view.offset = std::numeric_limits<int64_t>::min();
          break;
        }
        view.offset += offsets[axis] * *destinationType->strides[axis];
      }
    }
    const std::string destinationName = bufferNames.at(destinationIdentity);
    const std::string viewType = PlanMemoryPreserveAddressSpace(
        PlanMemoryFormatMemRefType(view), bufferTypes.at(destinationIdentity));
    OperationRecord operation = baseOperation(producer, "memref.subview");
    const std::string resultName =
        "%insert_slice_view_" + std::to_string(nextFlattenValue++);
    operation.text = resultName + " = memref.subview " + destinationName;
    for (size_t index = 2; index < insertSlice.operands.size(); ++index)
      if (foldedScalarConstants.count(insertSlice.operands[index]) == 0)
        operation.text += ", " + valueName(insertSlice.operands[index]);
    operation.text += " : " + namedValueTypes.at(destinationName) + " -> " +
                      viewType;
    result.operations.push_back(std::move(operation));
    namedValueTypes[resultName] = viewType;
    std::string destinationAlias = resultName;
    auto flattened = aliases.find(destinationIdentity);
    if (flattened == aliases.end())
      flattened = aliases.find(
          "value:" + std::to_string(producer.operands[1]));
    if (flattened != aliases.end()) {
      auto flattenedType = namedValueTypes.find(flattened->second);
      const auto parsedFlattened =
          flattenedType == namedValueTypes.end()
              ? std::optional<MemRefTypeModel>{}
              : ParseMemRefType(flattenedType->second);
      if (parsedFlattened && parsedFlattened->shape.size() < view.shape.size()) {
        std::vector<size_t> axes(view.shape.size());
        std::iota(axes.begin(), axes.end(), 0);
        const std::string collapsedType = PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(
                CollapseMemRefType(view, {std::move(axes)})),
            viewType);
        OperationRecord collapse =
            baseOperation(producer, "memref.collapse_shape");
        destinationAlias = "%insert_slice_flattened_" +
                           std::to_string(nextFlattenValue++);
        collapse.text = destinationAlias + " = memref.collapse_shape " +
                        resultName + " : " + viewType + " -> " +
                        collapsedType;
        result.operations.push_back(std::move(collapse));
        namedValueTypes[destinationAlias] = collapsedType;
      }
    }
    aliases[destinationIdentity] = destinationAlias;
    aliases["value:" + std::to_string(producer.operands[1])] =
        destinationAlias;
  }

  void emitOperation(const GenericOperation &source) {
    if (erasedOperations.count(source.id) != 0)
      return;
    auto materializedConstant = materializedAffineConstants.find(source.id);
    if (materializedConstant != materializedAffineConstants.end()) {
      OperationRecord operation = baseOperation(source, "arith.constant");
      operation.text = PlanMemoryValueName(source.results.front()) +
                       " = arith.constant " +
                       std::to_string(materializedConstant->second) +
                       " : index";
      result.operations.push_back(std::move(operation));
      return;
    }
    if (transformedInsertSlices.count(source.id) != 0)
      return;
    if (source.name == "arith.mului_extended" ||
        source.name == "arith.mulsi_extended") {
      emitExtendedMultiply(source);
      return;
    }
    if (replayedInsertSliceProducers.count(source.id) == 0) {
      if (const GenericOperation *insertSlice =
              findInPlaceInsertSliceForProducer(source)) {
        if (const GenericOperation *allocation =
                findInsertSliceDestinationAllocation(*insertSlice)) {
          insertSliceByProducer[source.id] = insertSlice;
          transformedInsertSlices.insert(insertSlice->id);
          deferredInsertSliceProducers[allocation->id].push_back(&source);
          return;
        }
      }
    }
    if (source.name == "hivm.hir.atomic_xchg" ||
        source.name == "hivm.hir.atomic_rmw" ||
        source.name == "hivm.hir.atomic_cas")
      emitAtomicLockStart(source);
    std::vector<std::pair<size_t, const PlanMemoryInputBufferRecord *>>
        allocations;
    std::set<std::string> deferredControlCopyAllocations;
    auto sourceEvents = eventsBySource.find(source.id);
    if ((source.name == "scf.for" || source.name == "scf.if" ||
         source.name == "scf.while" || source.name == "scope.scope") &&
        sourceEvents != eventsBySource.end())
      for (const PlanMemoryInputAccessRecord *event : sourceEvents->second)
        if (event->operationName == "hivm.hir.copy")
          for (const auto &[identity, effect] : event->accesses)
            if (effect == "w" || effect == "rw")
              deferredControlCopyAllocations.insert(identity);
    auto buffers = buffersByOwner.find(source.id);
    if (buffers != buffersByOwner.end())
      for (const PlanMemoryInputBufferRecord *buffer : buffers->second)
        allocations.push_back({buffer->globalOrdinal, buffer});
    auto nonTargets = nonTargetBuffersByOwner.find(source.id);
    if (nonTargets != nonTargetBuffersByOwner.end())
      for (size_t ordinal : nonTargets->second)
        allocations.push_back({ordinal, nullptr});
    std::sort(allocations.begin(), allocations.end(),
              [](const auto &lhs, const auto &rhs) {
                return lhs.first < rhs.first;
              });
    size_t sourceAllocationIndex = 0;
    for (const auto &[ordinal, buffer] : allocations) {
      if (buffer && !buffer->extraBuffer &&
          deferredControlCopyAllocations.count(buffer->identity) == 0 &&
          source.name != "hivm.hir.atomic_rmw" &&
          !(source.name == "hivm.hir.vcmp" &&
            sourceAllocationIndex >= 1) &&
          !(source.name == "hivm.hir.atomic_cas" &&
            sourceAllocationIndex >= 1)) {
        emitAllocation(source, *buffer);
        emittedAllocations.insert(buffer->identity);
      } else if (!buffer) {
        emitNonTargetAllocation(source, ordinal);
      }
      if (buffer)
        ++sourceAllocationIndex;
    }

    auto deferred = deferredInsertSliceProducers.find(source.id);
    if (deferred != deferredInsertSliceProducers.end()) {
      for (const GenericOperation *producer : deferred->second) {
        replayedInsertSliceProducers.insert(producer->id);
        emitOperation(*producer);
      }
    }

    const GenericOperation *producerInsertSlice = nullptr;
    auto producerSlice = insertSliceByProducer.find(source.id);
    if (producerSlice != insertSliceByProducer.end())
      producerInsertSlice = producerSlice->second;
    else
      producerInsertSlice = findInPlaceInsertSliceForProducer(source);
    if (producerInsertSlice) {
      transformedInsertSlices.insert(producerInsertSlice->id);
      auto producerEvents = eventsBySource.find(source.id);
      if (producerEvents == eventsBySource.end())
        throw std::runtime_error(
            "PlanMemory input builder: missing deferred producer access");
      for (const PlanMemoryInputAccessRecord *event : producerEvents->second) {
        std::map<std::string, std::string> aliases =
            emitFlattenAliases(source, *event);
        emitInsertSliceProducerSubview(*producerInsertSlice, source, *event,
                                       aliases);
        emitAccess(source, *event, nullptr, &aliases);
      }
      return;
    }

    if (source.name == "hivm.hir.sync_block") {
      emitSyncBlockLowering(source);
      return;
    }

    if (module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint.scalarizedOperations.count(source.id)) {
      emitScalarizedOperation(source);
      if (source.name == "hivm.hir.atomic_xchg" ||
          source.name == "hivm.hir.atomic_rmw" ||
          source.name == "hivm.hir.atomic_cas")
        emitAtomicLockEnd(source);
      return;
    }

    if (source.name == "scf.for" || source.name == "scf.if" ||
        source.name == "scf.while" || source.name == "scope.scope") {
      auto controlEvents = eventsBySource.find(source.id);
      if (controlEvents != eventsBySource.end())
        for (const PlanMemoryInputAccessRecord *event :
             controlEvents->second) {
          if (event->operationName == source.name)
            continue;
          for (const auto &[identity, effect] : event->accesses) {
            if (effect != "w" && effect != "rw")
              continue;
            auto buffer = bufferByIdentity.find(identity);
            if (buffer == bufferByIdentity.end() ||
                deferredControlCopyAllocations.count(identity) == 0 ||
                emittedAllocations.count(identity) != 0)
              continue;
            emitAllocation(source, *buffer->second);
            emittedAllocations.insert(identity);
          }
          std::map<std::string, std::string> aliases =
              emitFlattenAliases(source, *event);
          emitInsertSliceDestinationSubview(source, *event, aliases);
          emitEventExtraAllocations(source, *event);
          emitAccess(source, *event, nullptr, &aliases);
          for (const auto &[identity, effect] : event->accesses)
            if ((effect == "w" || effect == "rw") &&
                event->outOfPlaceCopy && event->sourceOperandNumber >= 0)
              controlInitAliases[source.id][static_cast<size_t>(
                  event->sourceOperandNumber)] =
                  stableBufferAliases.count(identity) != 0
                      ? stableBufferAliases.at(identity)
                      : bufferNames.at(identity);
        }
    }

    if (isMappedFor(source)) {
      OperationRecord blockIndexOp =
          baseOperation(source, "hivm.hir.get_sub_block_idx");
      const std::string raw =
          "%sub_block_idx_" + std::to_string(nextScalarValue++);
      blockIndexOp.text =
          raw + " = hivm.hir.get_sub_block_idx : () -> (i64)";
      result.operations.push_back(std::move(blockIndexOp));
      namedValueTypes[raw] = "i64";

      OperationRecord cast = baseOperation(source, "arith.index_cast");
      const std::string index =
          "%sub_block_index_" + std::to_string(nextScalarValue++);
      cast.text = index + " = arith.index_cast " + raw +
                  " : (i64) -> (index)";
      result.operations.push_back(std::move(cast));
      namedValueTypes[index] = "index";

      for (int regionId : source.regions) {
        const GenericRegion &region =
            logical.regions.at(static_cast<size_t>(regionId));
        for (int blockRecordId : region.blocks) {
          const GenericBlock &block =
              logical.blocks.at(static_cast<size_t>(blockRecordId));
          if (!block.arguments.empty())
            rememberScalar(block.arguments.front(), index, block.id);
          emitBlock(block);
        }
      }
      return;
    }
    if (source.name == "scf.for" || source.name == "scf.if" ||
        source.name == "scf.while" || source.name == "scope.scope") {
      emitControlStart(source);
      for (size_t index = 0; index < source.regions.size(); ++index) {
        const GenericRegion &region = logical.regions.at(
            static_cast<size_t>(source.regions[index]));
        bool implicitEmptyRegion = source.name == "scf.if";
        for (int block : region.blocks)
          for (int operationId :
               logical.blocks.at(static_cast<size_t>(block)).operations) {
            const GenericOperation &operation = logical.operations.at(
                static_cast<size_t>(operationId));
            bool emptyYield = operation.name == "scf.yield";
            for (size_t operand = 0; operand < operation.operands.size();
                 ++operand)
              emptyYield =
                  emptyYield &&
                  (isRemovedIfResult(source, operand) ||
                   module.afterMarkMultiBuffer.afterInlineLoadCopy.afterAllocExtraBuffer.postBufferization.singlePoint
                           .canonicalizedIterArgResultKeys.count(
                               {source.id, static_cast<int>(operand)}) != 0);
            implicitEmptyRegion = implicitEmptyRegion && emptyYield;
          }
        if (implicitEmptyRegion && index != 0)
          continue;
        if (source.name == "scf.if" && index != 0)
          emitBoundary(source, "scf.if.else", source.regions[index]);
        if (implicitEmptyRegion) {
          std::vector<int> path = regionPath(source);
          path.push_back(source.regions[index]);
          emitSynthetic(source, "scf.if.implicit_yield", path,
                        nextOperationId++);
          continue;
        }
        for (int block : region.blocks)
          emitBlock(logical.blocks.at(static_cast<size_t>(block)));
      }
      emitBoundary(source,
                   source.name == "scope.scope"
                       ? "scope.scope.end"
                       : source.name + ".end");
      return;
    }
    if (source.name == "scf.yield" || source.name == "scf.condition" ||
        source.name == "scope.return") {
      if (source.parentId >= 0 &&
          erasedMappedLoops.count(source.parentId) != 0)
        return;
      auto terminatorEvents = eventsBySource.find(source.id);
      if (terminatorEvents != eventsBySource.end())
        for (const PlanMemoryInputAccessRecord *event :
             terminatorEvents->second) {
          if (event->operationName == source.name)
            continue;
          std::map<std::string, std::string> aliases =
              emitFlattenAliases(source, *event);
          emitEventExtraAllocations(source, *event);
          emitAccess(source, *event, nullptr, &aliases);
        }
      if (source.name == "scf.yield" && source.operands.empty() &&
          source.parentId >= 0 &&
          logical.operations.at(static_cast<size_t>(source.parentId)).name ==
              "scf.if")
        emitSynthetic(source, "scf.if.implicit_yield", regionPath(source),
                      nextOperationId++);
      else if (source.name == "scf.yield" && source.operands.empty() &&
               source.parentId >= 0 &&
               logical.operations.at(static_cast<size_t>(source.parentId))
                       .name == "scf.for")
        emitSynthetic(source, "scf.for.implicit_yield", regionPath(source),
                      nextOperationId++);
      else
        emitTerminator(source);
      return;
    }
    auto events = eventsBySource.find(source.id);
    if (events != eventsBySource.end()) {
      const bool conditionalFill =
          source.name == "hivm.hir.load" &&
          HasBooleanProperty(source, "init_out_buffer") &&
          [&]() {
            const std::vector<int64_t> segments = DecomposeI64Array(
                FindDictionaryValue(source.properties,
                                    "operandSegmentSizes"));
            return segments.size() >= 6 && segments[5] != 0;
          }() &&
          events->second.size() >= 2 &&
          events->second.front()->operationName == "hivm.hir.vbrc";
      size_t firstEvent = 0;
      if (conditionalFill) {
        const int regionId = 2000000 + source.id;
        const int controlId = 3000000 + source.id;
        const std::vector<int> parentPath = regionPath(source);
        std::vector<int> nestedPath = parentPath;
        nestedPath.push_back(regionId);
        emitSynthetic(source, "scf.if", parentPath, controlId);
        if (!source.operands.empty())
          result.operations.back().text =
              "scf.if " + valueName(source.operands.back());
        std::map<std::string, std::string> aliases =
            emitFlattenAliases(source, *events->second.front(), &nestedPath);
        emitInsertSliceDestinationSubview(source, *events->second.front(),
                                          aliases);
        emitEventExtraAllocations(source, *events->second.front());
        emitAccess(source, *events->second.front(), &nestedPath, &aliases);
        emitSynthetic(source, "scf.if.implicit_yield", nestedPath,
                      nextOperationId++);
        emitSynthetic(source, "scf.if.end", parentPath, controlId);
        firstEvent = 1;
      }
      for (size_t index = firstEvent; index < events->second.size(); ++index) {
        std::optional<std::pair<std::string, std::string>> savedVConcatAlias;
        std::optional<std::pair<std::string, std::string>>
            savedInsertSliceAlias;
        std::map<std::string, std::string> eventAliases;
        if (source.name == "tensor.insert_slice" &&
            !events->second[index]->outOfPlaceCopy &&
            !source.operands.empty()) {
          const int sourceValue = source.operands.front();
          const auto sourceBuffer = bufferizedBufferIds.find(sourceValue);
          const std::string sourceIdentity =
              sourceBuffer == bufferizedBufferIds.end()
                  ? std::string()
                  : mappedIdentity(sourceBuffer->second);
          if (!sourceIdentity.empty()) {
            std::string sourceView = latestSubviewForValue(sourceValue);
            const GenericOperation *sourceDefinition =
                DefiningOperation(logical, sourceValue);
            if (sourceView.empty() && sourceDefinition &&
                sourceDefinition->name == "tensor.extract_slice" &&
                !sourceDefinition->operands.empty()) {
              const auto backing = bufferizedBufferIds.find(
                  sourceDefinition->operands.front());
              const auto resultBuffer = bufferizedBufferIds.find(sourceValue);
              if (backing != bufferizedBufferIds.end() &&
                  resultBuffer != bufferizedBufferIds.end() &&
                  backing->second == resultBuffer->second)
                sourceView = PlanMemoryValueName(sourceValue);
            }
            const bool preserveSource = !sourceView.empty() ||
                                        preservedSSAValues.count(sourceValue) !=
                                            0;
            for (const auto &[identity, effect] :
                 events->second[index]->accesses) {
              if (preserveSource && identity == sourceIdentity &&
                  (effect == "r" || effect == "rw")) {
                const std::string alias =
                    sourceView.empty() ? valueName(sourceValue) : sourceView;
                eventAliases[identity] = alias;
                eventAliases["value:" + std::to_string(sourceValue)] = alias;
                break;
              }
            }
          }
        }
        if (events->second[index]->outOfPlaceCopy &&
            events->second[index]->sourceOperandNumber >= 0) {
          const size_t sourceOperand = static_cast<size_t>(
              events->second[index]->sourceOperandNumber);
          if (sourceOperand >= source.operands.size())
            throw std::runtime_error(
                "PlanMemory input builder: malformed out-of-place copy source");
          const int sourceValue = source.operands[sourceOperand];
          const auto sourceBuffer = bufferizedBufferIds.find(sourceValue);
          const std::string sourceIdentity =
              sourceBuffer == bufferizedBufferIds.end()
                  ? std::string()
                  : mappedIdentity(sourceBuffer->second);
          if (!sourceIdentity.empty()) {
            const std::string alias = valueName(sourceValue);
            eventAliases[sourceIdentity] = alias;
            eventAliases["value:" + std::to_string(sourceValue)] = alias;
          }
        }
        if (source.name == "hivm.hir.vconcat" &&
            events->second[index]->accesses.size() >= 2) {
          const std::string &identity =
              events->second[index]->accesses[1].first;
          auto saved = stableBufferAliases.find(identity);
          if (saved != stableBufferAliases.end())
            savedVConcatAlias = *saved;
          emitVConcatDestinationSubview(source, *events->second[index]);
        }
        if (source.name == "tensor.insert_slice" &&
            !events->second[index]->outOfPlaceCopy) {
          std::string sourceIdentity;
          std::string destinationIdentity;
          for (const auto &[identity, effect] :
               events->second[index]->accesses) {
            if (effect == "r" && sourceIdentity.empty())
              sourceIdentity = identity;
            if ((effect == "w" || effect == "rw") &&
                destinationIdentity.empty())
              destinationIdentity = identity;
          }
          if (!sourceIdentity.empty() && !destinationIdentity.empty()) {
            if (eventAliases.count(sourceIdentity) == 0)
              eventAliases[sourceIdentity] =
                  stableBufferAliases.count(sourceIdentity) != 0
                      ? stableBufferAliases.at(sourceIdentity)
                      : bufferNames.at(sourceIdentity);
            auto saved = stableBufferAliases.find(destinationIdentity);
            if (saved != stableBufferAliases.end())
              savedInsertSliceAlias = *saved;
            emitInsertSliceDestinationSubview(
                source, *events->second[index], eventAliases);
            auto prepared = eventAliases.find(destinationIdentity);
            if (prepared != eventAliases.end())
              stableBufferAliases[destinationIdentity] = prepared->second;
          }
        }
        std::map<std::string, std::string> aliases = emitFlattenAliases(
            source, *events->second[index], nullptr,
            eventAliases.empty() ? nullptr : &eventAliases);
        emitEventExtraAllocations(source, *events->second[index]);
        emitAtomicTemporaryView(source, *events->second[index], aliases);
        if (source.name == "hivm.hir.atomic_rmw" ||
            source.name == "hivm.hir.atomic_cas")
          emitEventExtraAllocations(source, *events->second[index]);
        emitAccess(source, *events->second[index], nullptr, &aliases);
        if (source.name == "tensor.insert_slice" &&
            !events->second[index]->outOfPlaceCopy) {
          std::string destinationIdentity;
          for (const auto &[identity, effect] :
               events->second[index]->accesses)
            if ((effect == "w" || effect == "rw") &&
                destinationIdentity.empty())
              destinationIdentity = identity;
          if (!destinationIdentity.empty()) {
            if (savedInsertSliceAlias)
              stableBufferAliases[destinationIdentity] =
                  savedInsertSliceAlias->second;
            else
              stableBufferAliases.erase(destinationIdentity);
          }
        }
        if (source.name == "hivm.hir.vconcat" &&
            events->second[index]->accesses.size() >= 2) {
          const std::string &identity =
              events->second[index]->accesses[1].first;
          if (savedVConcatAlias)
            stableBufferAliases[identity] = savedVConcatAlias->second;
          else
            stableBufferAliases.erase(identity);
        }
      }
    } else {
      emitPassthrough(source);
    }
    if (source.name == "hivm.hir.atomic_xchg" ||
        source.name == "hivm.hir.atomic_rmw" ||
        source.name == "hivm.hir.atomic_cas")
      emitAtomicLockEnd(source);
  }

  void emitEventExtraAllocations(
      const GenericOperation &source,
      const PlanMemoryInputAccessRecord &event) {
    for (const auto &[identity, effect] : event.accesses) {
      (void)effect;
      if (source.name == "hivm.hir.vcmp" &&
          event.operationName == "hivm.hir.vcmp" &&
          event.temporaryBuffers.count(identity) != 0)
        continue;
      auto buffer = bufferByIdentity.find(identity);
      if (buffer == bufferByIdentity.end() ||
          (!buffer->second->extraBuffer &&
           source.name != "hivm.hir.atomic_rmw" &&
           source.name != "hivm.hir.vcmp" &&
           source.name != "hivm.hir.atomic_cas") ||
          emittedAllocations.count(identity) != 0)
        continue;
      emitAllocation(source, *buffer->second);
      emittedAllocations.insert(identity);
      if (source.name == "hivm.hir.atomic_rmw" ||
          source.name == "hivm.hir.vcmp" ||
          source.name == "hivm.hir.atomic_cas")
        return;
    }
  }

  void emitAtomicTemporaryView(
      const GenericOperation &source,
      const PlanMemoryInputAccessRecord &event,
      std::map<std::string, std::string> &aliases) {
    if (source.name != "hivm.hir.atomic_xchg" &&
        source.name != "hivm.hir.atomic_rmw")
      return;
    auto existing = atomicTemporaryViews.find(source.id);
    if (existing != atomicTemporaryViews.end())
      aliases.insert(existing->second.begin(), existing->second.end());
    if (source.operands.empty() || source.operandTypes.empty())
      return;
    const size_t shapeOperand =
        source.name == "hivm.hir.atomic_rmw" ? 1 : 0;
    if (source.operands.size() <= shapeOperand ||
        source.operandTypes.size() <= shapeOperand)
      return;
    const GenericOperation *shapeSource =
        DefiningOperation(logical, source.operands[shapeOperand]);
    if (!shapeSource || shapeSource->name != "memref.subview")
      return;
    for (const auto &[identity, effect] : event.accesses) {
      (void)effect;
      auto buffer = bufferByIdentity.find(identity);
      if (buffer == bufferByIdentity.end())
        continue;
      const std::optional<MemRefTypeModel> backingType =
          ParseMemRefType(bufferTypes.at(identity));
      if (!backingType || backingType->elementType != "i8")
        continue;
      auto stable = stableBufferAliases.find(identity);
      if (stable != stableBufferAliases.end()) {
        aliases[identity] = stable->second;
        continue;
      }
      std::string viewType = source.operandTypes[shapeOperand];
      if (IsTensorType(viewType))
        viewType = ConvertTensorToMemRefType(viewType);
      viewType = PlanMemoryPreserveAddressSpace(viewType,
                                                bufferTypes.at(identity));
      const GenericOperation *function = EnclosingFunction(logical, source);
      auto constants = function ? integerConstants.find(function->id)
                                : integerConstants.end();
      if (constants == integerConstants.end() ||
          constants->second.count(0) == 0)
        throw std::runtime_error("HIVMDecomposeOp: missing temp-view zero");
      const std::string view =
          "%atomic_temp_view_" + std::to_string(nextFlattenValue++);
      OperationRecord operation = baseOperation(source, "memref.view");
      operation.text = view + " = memref.view " + bufferNames.at(identity) +
                       ", " + scalarValueName(constants->second.at(0));
      for (size_t index = 1; index < shapeSource->operands.size(); ++index)
        operation.text += ", " + scalarValueName(shapeSource->operands[index]);
      operation.text += " : " + bufferTypes.at(identity) + " -> " + viewType;
      result.operations.push_back(std::move(operation));
      namedValueTypes[view] = viewType;
      aliases[identity] = view;
      const std::string &backingName = bufferNames.at(identity);
      for (const auto &[candidateIdentity, candidateName] : bufferNames) {
        if (candidateName == backingName)
          stableBufferAliases[candidateIdentity] = view;
      }
      atomicTemporaryViews[source.id][identity] = view;
    }
  }

  void emitInsertSliceDestinationSubview(
      const GenericOperation &source,
      const PlanMemoryInputAccessRecord &event,
      std::map<std::string, std::string> &aliases) {
    if (source.name != "tensor.insert_slice" || event.outOfPlaceCopy)
      return;
    std::string sourceIdentity;
    std::string destinationIdentity;
    for (const auto &[identity, effect] : event.accesses) {
      if (effect == "r" && sourceIdentity.empty())
        sourceIdentity = identity;
      if ((effect == "w" || effect == "rw") &&
          destinationIdentity.empty())
        destinationIdentity = identity;
    }
    if (destinationIdentity.empty() || sourceIdentity.empty() ||
        aliases.count(sourceIdentity) == 0)
      return;
    std::string sourceMemRefType = source.operandTypes.empty()
                                       ? std::string()
                                       : source.operandTypes.front();
    if (IsTensorType(sourceMemRefType))
      sourceMemRefType = ConvertTensorToMemRefType(sourceMemRefType);
    const std::optional<MemRefTypeModel> sourceType =
        ParseMemRefType(sourceMemRefType);
    const std::optional<MemRefTypeModel> destinationType =
        ParseMemRefType(bufferTypes.at(destinationIdentity));
    if (!sourceType || !destinationType ||
        sourceType->shape.size() > destinationType->strides.size())
      throw std::runtime_error(
          "PlanMemory input builder: malformed insert_slice subview");
    MemRefTypeModel view = *sourceType;
    view.addressSpace = destinationType->addressSpace;
    view.hasStridedLayout = true;
    view.strides.assign(destinationType->strides.begin(),
                        destinationType->strides.begin() +
                            static_cast<std::ptrdiff_t>(view.shape.size()));
    std::vector<int64_t> staticSizes = DecomposeI64Array(
        FindDictionaryValue(source.properties, "static_sizes"));
    const std::vector<size_t> segments =
        OperandSegmentSizes(source.properties);
    if (segments.size() >= 4) {
      size_t dynamicSizeOperand = 2 + segments[2];
      for (int64_t &size : staticSizes) {
        if (size != std::numeric_limits<int64_t>::min())
          continue;
        if (dynamicSizeOperand >= source.operands.size())
          break;
        auto folded =
            foldedScalarConstants.find(source.operands[dynamicSizeOperand++]);
        if (folded != foldedScalarConstants.end())
          size = folded->second;
      }
    }
    if (staticSizes.size() == view.shape.size())
      for (size_t axis = 0; axis < staticSizes.size(); ++axis)
        if (staticSizes[axis] != std::numeric_limits<int64_t>::min())
          view.shape[axis] = staticSizes[axis];
    view.offset = destinationType->offset;
    const std::vector<int64_t> staticOffsets = DecomposeI64Array(
        FindDictionaryValue(source.properties, "static_offsets"));
    for (int64_t offset : staticOffsets)
      if (offset == std::numeric_limits<int64_t>::min())
        view.offset = std::numeric_limits<int64_t>::min();
    if (view.offset != std::numeric_limits<int64_t>::min() &&
        staticOffsets.size() == view.shape.size()) {
      for (size_t axis = 0; axis < staticOffsets.size(); ++axis) {
        if (axis >= destinationType->strides.size() ||
            !destinationType->strides[axis]) {
          view.offset = std::numeric_limits<int64_t>::min();
          break;
        }
        view.offset += staticOffsets[axis] * *destinationType->strides[axis];
      }
    }
    const bool dynamicView =
        view.offset == std::numeric_limits<int64_t>::min() ||
        std::any_of(view.shape.begin(), view.shape.end(),
                    [](const std::optional<int64_t> &extent) {
                      return !extent.has_value();
                    });
    const std::string destinationName =
        !dynamicView &&
                stableBufferAliases.count(destinationIdentity) != 0
            ? stableBufferAliases.at(destinationIdentity)
            : bufferNames.at(destinationIdentity);
    const std::string viewType = PlanMemoryPreserveAddressSpace(
        PlanMemoryFormatMemRefType(view),
        bufferTypes.at(destinationIdentity));
    for (auto emitted = result.operations.rbegin();
         !dynamicView &&
         emitted != result.operations.rend(); ++emitted) {
      if (emitted->opName != "memref.subview" ||
          emitted->text.find(destinationName) == std::string::npos)
        continue;
      const std::vector<std::string> results = operationResultNames(*emitted);
      if (results.empty())
        continue;
      auto typeIt = namedValueTypes.find(results.front());
      if (typeIt != namedValueTypes.end() &&
          PhysicalTypeSignature(typeIt->second) ==
              PhysicalTypeSignature(viewType)) {
        aliases[destinationIdentity] = results.front();
        return;
      }
    }
    OperationRecord operation = baseOperation(source, "memref.subview");
    const std::string resultName =
        "%insert_slice_view_" + std::to_string(nextFlattenValue++);
    operation.text = resultName + " = memref.subview " + destinationName;
    for (size_t index = 2; index < source.operands.size(); ++index)
      if (foldedScalarConstants.count(source.operands[index]) == 0)
        operation.text += ", " + valueName(source.operands[index]);
    operation.text += " : " + namedValueTypes.at(destinationName) +
                      " -> " + viewType;
    result.operations.push_back(std::move(operation));
    namedValueTypes[resultName] = viewType;
    std::string destinationAlias = resultName;
    auto flattened = aliases.find(destinationIdentity);
    if (flattened != aliases.end()) {
      auto flattenedType = namedValueTypes.find(flattened->second);
      const std::optional<MemRefTypeModel> parsedFlattened =
          flattenedType == namedValueTypes.end()
              ? std::nullopt
              : ParseMemRefType(flattenedType->second);
      if (parsedFlattened && parsedFlattened->shape.size() < view.shape.size()) {
        std::vector<size_t> axes(view.shape.size());
        std::iota(axes.begin(), axes.end(), 0);
        const std::string collapsedType = PlanMemoryPreserveAddressSpace(
            PlanMemoryFormatMemRefType(
                CollapseMemRefType(view, {std::move(axes)})),
            viewType);
        OperationRecord collapse =
            baseOperation(source, "memref.collapse_shape");
        destinationAlias =
            "%insert_slice_flattened_" +
            std::to_string(nextFlattenValue++);
        collapse.text = destinationAlias + " = memref.collapse_shape " +
                        resultName + " : " + viewType + " -> " +
                        collapsedType;
        result.operations.push_back(std::move(collapse));
        namedValueTypes[destinationAlias] = collapsedType;
      }
    }
    aliases[destinationIdentity] = destinationAlias;
  }

  void emitBlock(const GenericBlock &block) {
    for (int operation : block.operations)
      emitOperation(logical.operations.at(static_cast<size_t>(operation)));
  }

  void finalizeOperations() {
    if (erasedOutputOperations.empty())
      return;
    std::vector<OperationRecord> compact;
    compact.reserve(result.operations.size() - erasedOutputOperations.size());
    for (OperationRecord &operation : result.operations) {
      if (erasedOutputOperations.count(operation.index) != 0)
        continue;
      operation.index = static_cast<int>(compact.size());
      operation.line = operation.index + 1;
      compact.push_back(std::move(operation));
    }
    result.operations = std::move(compact);
    std::map<std::string, int> allocationLines;
    for (const OperationRecord &operation : result.operations)
      if (operation.opName == "memref.alloc")
        for (const std::string &name : operationResultNames(operation))
          allocationLines[name] = operation.line;
    for (IRAllocRecord &allocation : result.allocations) {
      auto line = allocationLines.find(allocation.name);
      if (line != allocationLines.end())
        allocation.line = line->second;
    }
  }

  const PlanMemoryInputSemanticIR &module;
  std::string targetFunction;
  const GenericModule &logical;
  PlanMemoryInput result;
  int nextOperationId = 1000000;
  int nextFlattenValue = 0;
  int nextSyntheticBlock = 1000000;
  int nextScalarValue = 0;
  std::map<int, int> syntheticBlocks;
  std::map<int, std::string> scalarValues;
  std::map<int, int> scalarValueBlocks;
  std::map<std::string, std::string> finalIdentity;
  std::set<std::string> targetSourceIdentities;
  std::map<std::string, std::string> bufferNames;
  std::map<std::string, std::string> bufferTypes;
  std::map<std::string, const LocalBufferRecord *> finalBufferRecords;
  std::map<std::string, std::string> namedValueTypes;
  std::map<std::string, std::string> allocationViewAliases;
  std::map<std::string, std::string> latestSubviewAliases;
  std::map<std::string, std::string> latestSubviewAliasesByBackingName;
  std::map<std::string, std::string> alignedViewTypes;
  std::map<std::string, std::string> stableBufferAliases;
  std::map<int, std::vector<const PlanMemoryInputBufferRecord *>> buffersByOwner;
  std::map<std::string, const PlanMemoryInputBufferRecord *> bufferByIdentity;
  std::set<std::string> emittedAllocations;
  std::map<int, std::vector<size_t>> nonTargetBuffersByOwner;
  std::map<int, std::string> valueBuffers;
  std::map<int, std::set<int>> valueUsers;
  std::set<int> unambiguousBufferizedValues;
  std::map<int, std::string> bufferizedBufferIds;
  std::map<int, std::string> bufferizedValueTypes;
  std::map<std::string, int> bufferRepresentativeValues;
  std::map<std::pair<std::string, std::string>, int>
      bufferRepresentativeValuesByType;
  std::map<std::string, std::string> cseAliases;
  std::map<std::string, std::map<std::vector<int>, std::string>>
      collapseCseAliases;
  std::map<const PlanMemoryInputAccessRecord *, std::vector<bool>>
      reducedUnitAxesByEvent;
  std::map<const PlanMemoryInputAccessRecord *, std::vector<int64_t>>
      limitedDimensionMappings;
  std::map<int, std::map<size_t, std::string>> controlInitAliases;
  std::map<int, std::vector<const GenericOperation *>>
      deferredInsertSliceProducers;
  std::map<int, const GenericOperation *> insertSliceByProducer;
  std::set<int> transformedInsertSlices;
  std::set<int> replayedInsertSliceProducers;
  std::map<int, std::string> atomicLockValues;
  std::map<int, std::map<std::string, std::string>> atomicTemporaryViews;
  struct ExpandShapeRecord {
    std::string source;
    std::string sourceType;
    int operation = -1;
    bool dynamic = false;
  };
  std::map<std::string, ExpandShapeRecord> composedExpandShapes;
  std::map<std::string, ExpandShapeRecord> composedCollapseShapes;
  std::set<int> erasedOutputOperations;
  std::map<int, std::string> canonicalViewAliases;
  std::map<int, std::string> erasedValueAliases;
  std::set<int> erasedMappedLoops;
  std::set<int> erasedRegionIds;
  std::set<int> erasedOperations;
  ConvertArithToAffineState arithToAffine;
  std::map<int, int64_t> foldedScalarConstants;
  std::map<int, int64_t> materializedAffineConstants;
  std::map<int, std::map<int64_t, int>> integerConstants;
  std::map<int, std::map<std::string, int>> zeroScalarConstants;
  std::map<int, std::map<std::string, std::map<uint64_t, int>>>
      integerScalarConstants;
  std::map<int, std::map<std::string, std::string>>
      generatedFunctionConstants;
  std::map<int, std::string> generatedVSubNegatedConstants;
  std::map<int, std::vector<std::string>> vconcatInputSizes;
  std::map<int, std::vector<std::string>> vconcatOffsets;
  std::map<int, std::map<std::string, std::string>> generatedZeroNames;
  std::set<int> conditionalValues;
  std::set<int> preservedSSAValues;
  std::vector<std::string> functionArguments;
  std::map<int, std::vector<const PlanMemoryInputAccessRecord *>> eventsBySource;
};

inline PlanMemoryInput
BuildPlanMemoryInput(const PlanMemoryInputSemanticIR &module) {
  return PlanMemoryInputBuilder(module).Build();
}

inline PlanMemoryInput
BuildPlanMemoryInput(const PlanMemoryInputSemanticIR &module,
                     const std::string &targetFunction) {
  return PlanMemoryInputBuilder(module, targetFunction).Build();
}

inline std::string SerializePlanMemoryInput(const PlanMemoryInput &input) {
  std::ostringstream output;
  output << "PLANMEMORY_STRUCTURED_INPUT\t1\n";
  for (const IRAllocRecord &allocation : input.allocations)
    output << "ALLOC\t" << HexEncode(allocation.name) << '\t'
           << allocation.constBits << '\t'
           << HexEncode(allocation.memrefType) << '\n';
  for (const OperationRecord &operation : input.operations) {
    output << "OP\t" << operation.index << '\t' << operation.operationId
           << '\t' << HexEncode(operation.opName) << '\t';
    for (size_t index = 0; index < operation.regionPath.size(); ++index) {
      if (index)
        output << ',';
      output << operation.regionPath[index];
    }
    output << '\t' << HexEncode(operation.text) << '\n';
  }
  return output.str();
}

inline std::string
SerializeCanonicalPlanMemoryInput(const PlanMemoryInput &input) {
  std::ostringstream output;
  output << "PLANMEMORY_CANONICAL_INPUT\t1\n";
  std::map<std::string, std::string> valueIds;
  for (size_t index = 0; index < input.allocations.size(); ++index) {
    const IRAllocRecord &allocation = input.allocations[index];
    valueIds[allocation.name] = "b" + std::to_string(index);
    output << "ALLOC\t" << index << '\t' << allocation.constBits << '\t'
           << HexEncode(PhysicalTypeSignature(allocation.memrefType))
           << '\n';
  }
  for (size_t index = 0; index < input.functionArguments.size(); ++index)
    valueIds[input.functionArguments[index]] = "a" + std::to_string(index);

  size_t externalId = 0;
  std::map<int, size_t> regionIds;
  std::map<int, size_t> blockIds;
  std::vector<int> commonRegionPath;
  if (!input.operations.empty()) {
    commonRegionPath = input.operations.front().regionPath;
    for (const OperationRecord &operation : input.operations) {
      size_t common = 0;
      while (common < commonRegionPath.size() &&
             common < operation.regionPath.size() &&
             commonRegionPath[common] == operation.regionPath[common])
        ++common;
      commonRegionPath.resize(common);
    }
  }
  auto canonicalValue = [&](const std::string &value) -> std::string {
    auto found = valueIds.find(value);
    if (found != valueIds.end())
      return found->second;
    const std::string id = "x" + std::to_string(externalId++);
    valueIds[value] = id;
    return id;
  };
  auto canonicalValues = [&](const std::vector<std::string> &values) {
    std::ostringstream result;
    for (size_t index = 0; index < values.size(); ++index) {
      if (index)
        result << ',';
      result << canonicalValue(values[index]);
    }
    return result.str();
  };
  auto canonicalTypes = [&](const std::string &text) {
    std::ostringstream result;
    const std::vector<std::string> types = extractMemrefTypes(text);
    for (size_t index = 0; index < types.size(); ++index) {
      if (index)
        result << ',';
      const std::optional<MemRefTypeModel> type = ParseMemRefType(types[index]);
      const std::string addressSpace =
          type && type->addressSpace != AddressSpace::Unknown
              ? AddressSpaceName(type->addressSpace)
              : "gm";
      result << PhysicalTypeSignature(types[index]) << '@'
             << addressSpace;
    }
    return result.str();
  };
  auto normalizedArray = [](const OperationRecord &operation,
                            const std::string &name) {
    const std::string effectiveName =
        operation.opName == "hivm.hir.vtranspose" && name == "transpose"
            ? "permutation"
            : name;
    std::vector<int64_t> values;
    if (effectiveName == "permutation") {
      const size_t key = operation.text.find("permutation = array<i64");
      const size_t end = key == std::string::npos
                             ? std::string::npos
                             : operation.text.find('>', key);
      if (end != std::string::npos)
        values = DecomposeI64Array(
            operation.text.substr(key, end - key + 1));
    }
    if (values.empty())
      values = getDenseI64Array(operation.text, effectiveName);
    if (values.empty()) {
      std::string raw = FindDictionaryValue(operation.text, effectiveName);
      if (!raw.empty())
        values = DecomposeI64Array(raw);
    }
    std::ostringstream result;
    for (size_t index = 0; index < values.size(); ++index) {
      if (index)
        result << ',';
      result << values[index];
    }
    return result.str();
  };

  for (size_t index = 0; index < input.operations.size(); ++index) {
    const OperationRecord &operation = input.operations[index];
    std::vector<std::string> results = operationResultNames(operation);
    for (size_t result = 0; result < results.size(); ++result)
      if (!valueIds.count(results[result]))
        valueIds[results[result]] =
            "r" + std::to_string(index) + "." + std::to_string(result);
    std::ostringstream path;
    size_t pathSize = 0;
    for (size_t depth = commonRegionPath.size();
         depth < operation.regionPath.size(); ++depth) {
      const int region = operation.regionPath[depth];
      if (!regionIds.count(region))
        regionIds[region] = regionIds.size();
      if (pathSize++)
        path << ',';
      path << regionIds.at(region);
    }
    const std::vector<std::string> inputs =
        extractGroupSSAs(operation.text, "ins");
    const std::vector<std::string> outputs =
        extractGroupSSAs(operation.text, "outs");
    const std::vector<std::string> temporaries =
        extractGroupSSAs(operation.text, "temp_buffer");
    std::map<std::string, size_t> grouped;
    for (const std::string &value : inputs)
      ++grouped[value];
    for (const std::string &value : outputs)
      ++grouped[value];
    for (const std::string &value : temporaries)
      ++grouped[value];
    std::vector<std::string> ungroupedOperands;
    for (const std::string &value : operationOperandNames(operation)) {
      auto found = grouped.find(value);
      if (found != grouped.end() && found->second != 0) {
        --found->second;
        continue;
      }
      ungroupedOperands.push_back(value);
    }
    if (!blockIds.count(operation.blockId))
      blockIds[operation.blockId] = blockIds.size();
    output << "OP\t" << index << '\t' << HexEncode(operation.opName) << '\t'
           << path.str() << '\t' << blockIds.at(operation.blockId) << '\t'
           << canonicalValues(results) << '\t'
           << canonicalValues(ungroupedOperands) << '\t'
           << canonicalValues(inputs) << '\t'
           << canonicalValues(outputs) << '\t'
           << canonicalValues(temporaries)
           << '\t' << HexEncode(canonicalTypes(operation.text)) << '\t'
           << normalizedArray(operation, "broadcast") << '\t'
           << normalizedArray(operation, "transpose") << '\t'
           << normalizedArray(operation, "reduce_dims") << '\t'
           << normalizedArray(operation, "cum_dims") << '\t'
           << (operation.text.find("hivm.preload_local_buffer") !=
                       std::string::npos
                   ? 1
                   : 0)
           << '\t' << FindDictionaryValue(operation.text, "hivm.multi_buffer")
           << '\n';
  }
  return output.str();
}

inline std::string
SerializePlanMemoryOperationSequence(const PlanMemoryInput &input) {
  std::ostringstream output;
  output << "PLANMEMORY_OPERATION_SEQUENCE\t1\n";
  for (const OperationRecord &operation : input.operations)
    output << "OP\t" << HexEncode(operation.opName) << '\n';
  return output.str();
}

} // namespace cvub

#endif
