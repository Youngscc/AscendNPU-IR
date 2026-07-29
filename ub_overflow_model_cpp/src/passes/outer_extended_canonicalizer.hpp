#ifndef UB_OVERFLOW_MODEL_CPP_OUTER_EXTENDED_CANONICALIZER_HPP
#define UB_OVERFLOW_MODEL_CPP_OUTER_EXTENDED_CANONICALIZER_HPP

#include "../ir/generic_rewriter.hpp"
#include "../ir/operation_folder.hpp"
#include "one_shot_bufferize.hpp"

namespace cvub {

inline std::string OuterCanonicalizerEnumValue(std::string value) {
  value = trim(std::move(value));
  const size_t open = value.rfind('<');
  const size_t close = value.rfind('>');
  if (open != std::string::npos && close != std::string::npos && open < close)
    return value.substr(open + 1, close - open - 1);
  return value;
}

inline std::string OuterCanonicalizerConstantLiteral(
    const GenericOperation &operation) {
  std::string value = FindDictionaryValue(operation.properties, "value");
  if (value.empty())
    value = FindDictionaryValue(operation.attributes, "value");
  const size_t colon = value.find(':');
  if (colon != std::string::npos)
    value.resize(colon);
  return trim(std::move(value));
}

inline std::vector<int64_t>
OuterCanonicalizerI64Array(const std::string &text) {
  std::vector<int64_t> result;
  const size_t colon = text.find(':');
  const size_t close = text.rfind('>');
  if (colon == std::string::npos || close == std::string::npos ||
      colon >= close)
    return result;
  for (const std::string &item :
       splitTopLevel(text.substr(colon + 1, close - colon - 1)))
    result.push_back(std::stoll(trim(item)));
  return result;
}

inline std::string OuterCanonicalizerI64ArrayText(
    const std::vector<int64_t> &values) {
  std::ostringstream output;
  output << "array<i64:";
  for (size_t index = 0; index < values.size(); ++index)
    output << (index == 0 ? " " : ", ") << values[index];
  return output.str() + ">";
}

inline std::string OuterCanonicalizerSetDictionaryValue(
    const std::string &dictionary, const std::string &name,
    const std::string &value) {
  if (dictionary.size() < 2 || dictionary.front() != '{' ||
      dictionary.back() != '}')
    throw std::runtime_error(
        "canonicalize-ext: malformed operation properties");
  std::vector<std::string> entries =
      splitTopLevel(dictionary.substr(1, dictionary.size() - 2));
  bool replaced = false;
  for (std::string &entry : entries) {
    const size_t equal = entry.find('=');
    if (equal != std::string::npos && trim(entry.substr(0, equal)) == name) {
      entry = name + " = " + value;
      replaced = true;
    }
  }
  if (!replaced)
    entries.push_back(name + " = " + value);
  std::string result = "{";
  for (size_t index = 0; index < entries.size(); ++index) {
    if (index != 0)
      result += ", ";
    result += trim(entries[index]);
  }
  return result + "}";
}

inline void OuterCanonicalizerSetProperty(GenericOperation &operation,
                                          const std::string &name,
                                          const std::string &value) {
  operation.properties = OuterCanonicalizerSetDictionaryValue(
      operation.properties, name, value);
  operation.attributes = OuterCanonicalizerSetDictionaryValue(
      operation.attributes, name, value);
}

inline std::optional<std::string>
OuterCanonicalizerElementType(const std::string &shapedType) {
  const std::string memrefType = startsWith(shapedType, "tensor<")
                                     ? ConvertTensorToMemRefType(shapedType)
                                     : shapedType;
  const std::optional<MemRefTypeModel> parsed = ParseMemRefType(memrefType);
  if (!parsed)
    return std::nullopt;
  return parsed->elementType;
}

inline bool OuterCanonicalizerIntegerLiteralEquals(const std::string &literal,
                                                   int64_t expected) {
  try {
    size_t consumed = 0;
    const int64_t value = std::stoll(literal, &consumed, 0);
    return consumed == literal.size() && value == expected;
  } catch (const std::exception &) {
    return false;
  }
}

inline bool OuterCanonicalizerFloatLiteralEquals(
    const std::string &literal, const std::string &elementType,
    const std::string &kind) {
  if (kind == "zero")
    return literal == "0.000000e+00" || literal == "0.000000e+000" ||
           literal == "0.0" || literal == "0" || literal == "0x0000" ||
           literal == "0x00000000";
  if (kind == "one")
    return literal == "1.000000e+00" || literal == "1.000000e+000" ||
           literal == "1.0" || literal == "1";
  if (kind == "positive_infinity")
    return literal == "inf" || literal == "+inf" ||
           (elementType == "f16" && literal == "0x7C00") ||
           (elementType == "f32" && literal == "0x7F800000");
  if (kind == "negative_infinity")
    return literal == "-inf" ||
           (elementType == "f16" && literal == "0xFC00") ||
           (elementType == "f32" && literal == "0xFF800000");
  return false;
}

// Mirrors VReduceOp::getInit.  The supported integer types are the exact
// i8/i16/i32/i64 set in the native table; unsupported element types do not
// match the canonicalization pattern.
inline bool OuterCanonicalizerMatchesReduceInit(
    const std::string &arith, const std::string &elementType,
    const std::string &literal) {
  const bool isFloat = elementType == "f16" || elementType == "f32";
  if (arith == "sum" || arith == "xori" || arith == "ori")
    return isFloat
               ? OuterCanonicalizerFloatLiteralEquals(literal, elementType,
                                                       "zero")
               : OuterCanonicalizerIntegerLiteralEquals(literal, 0);
  if (arith == "prod")
    return isFloat
               ? OuterCanonicalizerFloatLiteralEquals(literal, elementType,
                                                       "one")
               : OuterCanonicalizerIntegerLiteralEquals(literal, 1);

  const bool minimum = arith == "min" ||
                       arith == "min_with_index_left" ||
                       arith == "min_with_index_right";
  const bool maximum = arith == "max" ||
                       arith == "max_with_index_left" ||
                       arith == "max_with_index_right";
  if (!minimum && !maximum && arith != "andi")
    return false;
  if (isFloat)
    return OuterCanonicalizerFloatLiteralEquals(
        literal, elementType,
        minimum ? "positive_infinity" : "negative_infinity");

  if (elementType.size() < 2 || elementType.front() != 'i')
    return false;
  unsigned width = 0;
  try {
    width = static_cast<unsigned>(std::stoul(elementType.substr(1)));
  } catch (const std::exception &) {
    return false;
  }
  if (width != 8 && width != 16 && width != 32 && width != 64)
    return false;
  if (arith == "andi")
    return OuterCanonicalizerIntegerLiteralEquals(literal, -1);
  const int64_t expected = minimum
                               ? (width == 64
                                      ? std::numeric_limits<int64_t>::max()
                                      : (int64_t{1} << (width - 1)) - 1)
                               : (width == 64
                                      ? std::numeric_limits<int64_t>::min()
                                      : -(int64_t{1} << (width - 1)));
  return OuterCanonicalizerIntegerLiteralEquals(literal, expected);
}

// Mirrors RedudantVBrcOp.  The misspelling is retained in the native pattern
// name; this implementation follows its tensor/buffer branches exactly.
inline bool RunOuterRedundantVBrc(GenericModule &module) {
  GenericRewriter rewriter(module);
  bool changed = false;
  const std::vector<int> order = [&]() {
    std::vector<int> result;
    for (const GenericOperation &operation : module.operations)
      if (operation.name == "func.func") {
        const std::vector<int> functionOrder =
            CollectGreedyOperationFolderPostOrder(module, operation.id);
        result.insert(result.end(), functionOrder.begin(), functionOrder.end());
      }
    return result;
  }();
  for (int operationId : order) {
    if (!IsGreedyOperationFolderAttached(module, operationId))
      continue;
    const GenericOperation &brc =
        module.operations.at(static_cast<size_t>(operationId));
    if (brc.name != "hivm.hir.vbrc" || brc.operands.size() < 2 ||
        brc.operandTypes.size() < 2)
      continue;
    const std::vector<size_t> inits = DpsInitOperandIndices(
        brc.name, brc.operands.size(), brc.properties);
    if (inits.size() != 1 || inits.front() >= brc.operandTypes.size())
      continue;
    const size_t destinationIndex = inits.front();
    const std::string &destinationType = brc.operandTypes[destinationIndex];
    const bool tensorSemantics = startsWith(destinationType, "tensor<") &&
                                 brc.results.size() == 1 &&
                                 brc.resultTypes.size() == 1;
    const bool bufferSemantics = startsWith(destinationType, "memref<") &&
                                 brc.results.empty();
    if (!tensorSemantics && !bufferSemantics)
      continue;
    const std::optional<MemRefTypeModel> destination = ParseMemRefType(
        tensorSemantics ? ConvertTensorToMemRefType(destinationType)
                        : destinationType);
    if (!destination)
      continue;
    const std::vector<int64_t> dimensions = OuterCanonicalizerI64Array(
        FindDictionaryValue(brc.properties, "broadcast_dims"));
    std::vector<int64_t> kept;
    kept.reserve(dimensions.size());
    for (int64_t dimension : dimensions) {
      if (dimension < 0 ||
          static_cast<size_t>(dimension) >= destination->shape.size())
        throw std::runtime_error(
            "canonicalize-ext: invalid vbrc broadcast dimension");
      const std::optional<int64_t> extent =
          destination->shape[static_cast<size_t>(dimension)];
      if (!extent || *extent != 1)
        kept.push_back(dimension);
    }
    if (kept.size() == dimensions.size())
      continue;

    if (!kept.empty()) {
      GenericOperation &modified = rewriter.modifyOperation(operationId);
      OuterCanonicalizerSetProperty(modified, "broadcast_dims",
                                    OuterCanonicalizerI64ArrayText(kept));
      changed = true;
      continue;
    }

    const int source = brc.operands.front();
    if (tensorSemantics) {
      if (brc.resultTypes.front() != brc.operandTypes.front())
        throw std::runtime_error(
            "canonicalize-ext: redundant tensor vbrc type mismatch");
      rewriter.replaceAllUses(brc.results.front(), source);
      rewriter.removeFromBlock(brc.blockId, operationId);
      changed = true;
      continue;
    }

    // The native buffer branch deliberately leaves scalar broadcasts alone.
    if (!startsWith(brc.operandTypes.front(), "memref<") &&
        !startsWith(brc.operandTypes.front(), "tensor<"))
      continue;
    const int parentId = brc.parentId;
    const int regionId = brc.regionId;
    const int blockId = brc.blockId;
    const int destinationValue = brc.operands[destinationIndex];
    const std::string sourceType = brc.operandTypes.front();
    const int copy = rewriter.createOperation(
        parentId, regionId, blockId, "hivm.hir.copy", {},
        {source, destinationValue}, {sourceType, destinationType});
    GenericBlock &block = module.blocks.at(static_cast<size_t>(blockId));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), operationId);
    rewriter.insertToBlock(
        blockId,
        static_cast<size_t>(std::distance(block.operations.begin(), position)),
        copy);
    rewriter.removeFromBlock(blockId, operationId);
    changed = true;
  }
  return changed;
}

inline bool OuterCanonicalizerHasStaticBufferSizeMark(
    const GenericOperation &operation) {
  return operation.name == "annotation.mark" &&
         !FindDictionaryValue(operation.attributes,
                              "buffer_size_in_byte").empty();
}

inline size_t OuterCanonicalizerAnnotationAttributeCount(
    const GenericOperation &operation) {
  if (operation.attributes.size() < 2 ||
      operation.attributes.front() != '{' ||
      operation.attributes.back() != '}')
    return 0;
  size_t count = 0;
  for (const std::string &entry : splitTopLevel(operation.attributes.substr(
           1, operation.attributes.size() - 2))) {
    const size_t equal = entry.find('=');
    if (equal == std::string::npos)
      continue;
    const std::string name = trim(entry.substr(0, equal));
    // These are inherent MarkOp fields, not user annotations counted by
    // MarkOp::getAttrNum().
    if (name != "effects" && name != "keys" && name != "operandSegmentSizes")
      ++count;
  }
  return count;
}

// Mirrors FoldUselessBufferSizeMarkOp, which is registered by MarkOp itself
// and therefore participates in the module-level extended canonicalizer.
inline bool RunOuterFoldUselessBufferSizeMarks(GenericModule &module) {
  GenericRewriter rewriter(module);
  std::map<int, int> definitions;
  for (const GenericOperation &operation : module.operations)
    if (IsGreedyOperationFolderAttached(module, operation.id))
      for (int result : operation.results)
        definitions[result] = operation.id;
  bool changed = false;
  bool localChange = true;
  while (localChange) {
    localChange = false;
    for (GenericOperation &mark : module.operations) {
      if (!IsGreedyOperationFolderAttached(module, mark.id) ||
          !OuterCanonicalizerHasStaticBufferSizeMark(mark) ||
          OuterCanonicalizerAnnotationAttributeCount(mark) != 1 ||
          mark.operands.empty())
        continue;
      const int source = mark.operands.front();
      size_t uses = 0;
      for (const GenericOperation &user : module.operations)
        if (IsGreedyOperationFolderAttached(module, user.id))
          uses += static_cast<size_t>(std::count(user.operands.begin(),
                                                user.operands.end(), source));
      if (uses != 1)
        continue;
      const auto found = definitions.find(source);
      const GenericOperation *definition =
          found == definitions.end()
              ? nullptr
              : &module.operations.at(static_cast<size_t>(found->second));
      if (definition &&
          (definition->name == "tensor.cast" ||
           definition->name == "memref.cast" ||
           definition->name == "tensor.collapse_shape" ||
           definition->name == "tensor.expand_shape" ||
           definition->name == "memref.collapse_shape" ||
           definition->name == "memref.expand_shape") &&
          !definition->operands.empty()) {
        rewriter.replaceOperand(mark.id, 0, definition->operands.front());
      } else {
        rewriter.removeFromBlock(mark.blockId, mark.id);
      }
      changed = localChange = true;
      break;
    }
  }
  return changed;
}

class OuterVReduceInitCanonicalizer {
public:
  explicit OuterVReduceInitCanonicalizer(GenericModule &input)
      : module(input), rewriter(module) {
    for (const GenericOperation &operation : module.operations)
      for (int result : operation.results)
        definitions[result] = operation.id;
    for (const GenericBlock &block : module.blocks)
      for (size_t index = 0; index < block.arguments.size(); ++index)
        blockArguments[block.arguments[index]] = {block.id, index};
  }

  void Run() {
    const std::vector<int> order = [&]() {
      std::vector<int> result;
      for (const GenericOperation &operation : module.operations)
        if (operation.name == "func.func") {
          const std::vector<int> functionOrder =
              CollectGreedyOperationFolderPostOrder(module, operation.id);
          result.insert(result.end(), functionOrder.begin(),
                        functionOrder.end());
        }
      return result;
    }();
    for (int operationId : order) {
      if (!IsGreedyOperationFolderAttached(module, operationId))
        continue;
      const GenericOperation &reduce =
          module.operations.at(static_cast<size_t>(operationId));
      if (reduce.name != "hivm.hir.vreduce" || reduce.operands.empty() ||
          reduce.operandTypes.empty())
        continue;
      const std::vector<size_t> inits = DpsInitOperandIndices(
          reduce.name, reduce.operands.size(), reduce.properties);
      if (inits.empty())
        continue;
      const std::string arith = OuterCanonicalizerEnumValue(
          FindDictionaryValue(reduce.properties, "arith"));
      const std::optional<std::string> elementType =
          OuterCanonicalizerElementType(reduce.operandTypes.front());
      if (!elementType)
        continue;
      rewriteInit(operationId, inits.front(), arith, *elementType,
                  /*matchAnyConstant=*/false);
      if (inits.size() > 1 &&
          (arith == "min_with_index_left" ||
           arith == "min_with_index_right" ||
           arith == "max_with_index_left" ||
           arith == "max_with_index_right"))
        rewriteInit(operationId, inits[1], arith, *elementType,
                    /*matchAnyConstant=*/true);
    }
  }

private:
  const GenericOperation *definition(int value) const {
    const auto found = definitions.find(value);
    return found == definitions.end()
               ? nullptr
               : &module.operations.at(static_cast<size_t>(found->second));
  }

  bool isFillByConstant(int value, const std::string &arith,
                        const std::string &elementType,
                        bool matchAnyConstant) const {
    const auto argument = blockArguments.find(value);
    if (argument != blockArguments.end()) {
      const GenericBlock &block =
          module.blocks.at(static_cast<size_t>(argument->second.first));
      const int parentId = module.regions.at(
          static_cast<size_t>(block.regionId)).parentOperation;
      if (parentId < 0)
        return false;
      const GenericOperation &parent =
          module.operations.at(static_cast<size_t>(parentId));
      const size_t argumentIndex = argument->second.second;
      size_t initIndex = std::numeric_limits<size_t>::max();
      if (parent.name == "scf.for" && argumentIndex > 0)
        initIndex = argumentIndex + 2;
      else if (parent.name == "scf.while")
        initIndex = argumentIndex;
      if (initIndex < parent.operands.size())
        return isFillByConstant(parent.operands[initIndex], arith,
                                elementType, matchAnyConstant);
      return false;
    }

    const GenericOperation *operation = definition(value);
    if (!operation)
      return false;
    if ((operation->name == "tensor.expand_shape" ||
         operation->name == "tensor.collapse_shape") &&
        !operation->operands.empty())
      return isFillByConstant(operation->operands.front(), arith,
                              elementType, matchAnyConstant);
    if (operation->name != "hivm.hir.vbrc" ||
        operation->operands.empty())
      return false;
    const GenericOperation *constant = definition(operation->operands.front());
    if (!constant || constant->name != "arith.constant")
      return false;
    return matchAnyConstant || OuterCanonicalizerMatchesReduceInit(
                                   arith, elementType,
                                   OuterCanonicalizerConstantLiteral(*constant));
  }

  void rewriteInit(int reduceId, size_t initIndex,
                   const std::string &arith, const std::string &elementType,
                   bool matchAnyConstant) {
    const GenericOperation &reduce =
        module.operations.at(static_cast<size_t>(reduceId));
    if (initIndex >= reduce.operands.size() ||
        initIndex >= reduce.operandTypes.size() ||
        !isFillByConstant(reduce.operands[initIndex], arith, elementType,
                          matchAnyConstant))
      return;
    const std::optional<MemRefTypeModel> type = ParseMemRefType(
        ConvertTensorToMemRefType(reduce.operandTypes[initIndex]));
    if (!type || std::any_of(type->shape.begin(), type->shape.end(),
                             [](const std::optional<int64_t> &extent) {
                               return !extent.has_value();
                             }))
      throw std::runtime_error(
          "canonicalize-ext: dynamic vreduce init requires tensor.dim "
          "materialization");
    const int parentId = reduce.parentId;
    const int regionId = reduce.regionId;
    const int blockId = reduce.blockId;
    const std::string resultType = reduce.operandTypes[initIndex];
    const int empty = rewriter.createOperation(parentId, regionId, blockId,
                                               "tensor.empty", {resultType});
    GenericBlock &block =
        module.blocks.at(static_cast<size_t>(blockId));
    const auto position =
        std::find(block.operations.begin(), block.operations.end(), reduceId);
    rewriter.insertToBlock(
        blockId,
        static_cast<size_t>(std::distance(block.operations.begin(), position)),
        empty);
    rewriter.replaceOperand(reduceId, initIndex,
                            module.operations.at(static_cast<size_t>(empty))
                                .results.front());
  }

  GenericModule &module;
  GenericRewriter rewriter;
  std::map<int, int> definitions;
  std::map<int, std::pair<int, size_t>> blockArguments;
};

inline void RunOuterCanonicalizerDeadCodeElimination(GenericModule &module) {
  bool changed = true;
  GenericRewriter rewriter(module);
  while (changed) {
    changed = false;
    for (auto iterator = module.operations.rbegin();
         iterator != module.operations.rend(); ++iterator) {
      GenericOperation &operation = *iterator;
      if (!IsGreedyOperationFolderAttached(module, operation.id) ||
          operation.results.empty() || !operation.regions.empty() ||
          !operation.effects.empty())
        continue;
      bool used = false;
      for (int result : operation.results)
        used = used || GreedyOperationFolderHasUse(module, result);
      if (used)
        continue;
      rewriter.removeFromBlock(operation.blockId, operation.id);
      changed = true;
    }
  }
}

// Projection of the module-level ExtendedCanonicalizer immediately before the
// canonicalizationHIVMPipeline.  At this boundary the supported input contract
// reaches the ordinary Arith folds introduced by AutoBlockify and the greedy
// driver's constant folder.  Later canonicalization passes are deliberately
// not included here.
inline GenericModule RunOuterExtendedCanonicalizer(GenericModule module) {
  ApplyOperationSemanticsToAll(module.operations);
  std::vector<int> functions;
  for (const GenericOperation &operation : module.operations)
    if (operation.name == "func.func")
      functions.push_back(operation.id);
  for (int functionId : functions) {
    // GreedyPatternRewriteDriver registers/CSEs every existing constant before
    // it starts processing the rewrite worklist.  Preserve that ordering: in
    // particular, a dominating constant must win CSE before an identity fold
    // makes one of its uses dead.
    RunGreedyOperationFolder(module, functionId);
    RunGreedyArithIdentityFolds(module, functionId);
  }
  RunOuterRedundantVBrc(module);
  RunOuterFoldUselessBufferSizeMarks(module);
  OuterVReduceInitCanonicalizer(module).Run();
  ApplyOperationSemanticsToAll(module.operations);
  RunOuterCanonicalizerDeadCodeElimination(module);
  module = CompactGenericModule(std::move(module));
  ApplyOperationSemanticsToAll(module.operations);
  return module;
}

} // namespace cvub

#endif
