#include "../src/passes/plan_memory/plan_memory_model.hpp"
#include "../src/passes/infer_and_set_buffer_size.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <typename Callback>
void requireThrows(Callback callback, const std::string &message) {
  try {
    callback();
  } catch (const std::exception &) {
    return;
  }
  throw std::runtime_error(message);
}

template <typename Callback>
void requireThrowsContaining(Callback callback, const std::string &needle,
                             const std::string &message) {
  try {
    callback();
  } catch (const std::exception &error) {
    require(std::string(error.what()).find(needle) != std::string::npos,
            message + ": " + error.what());
    return;
  }
  throw std::runtime_error(message);
}

cvub::fs::path writeFixture(const std::string &name,
                            const std::string &contents) {
  cvub::fs::path directory =
      cvub::fs::temp_directory_path() / "cvpipeline_ub_model_tests";
  cvub::fs::create_directories(directory);
  cvub::fs::path path = directory / name;
  std::ofstream output(path);
  output << contents;
  return path;
}

const char *kAIVAttribute =
    "hivm.func_core_type = #hivm.func_core_type<AIV>";

void testOperationSemantics() {
  require(cvub::IsDestinationStyleOp("hivm.hir.vadd"),
          "vadd must be destination style");
  require(!cvub::IsDestinationStyleOp("hivm.hir.future_op"),
          "unknown HIVM op must not be destination style");

  cvub::OperationRecord copy;
  copy.opName = "hivm.hir.copy";
  copy.text =
      "hivm.hir.copy ins(%src : memref<4xi32, "
      "#hivm.address_space<ub>>) outs(%dst : memref<4xi32, "
      "#hivm.address_space<cbuf>>)";
  require(cvub::GetPipe(copy) == cvub::HIVMPipe::MTE3,
          "UB-to-L1 copy must use MTE3");

  copy.text =
      "hivm.hir.copy ins(%src : memref<4xi32, "
      "#hivm.address_space<ub>>) outs(%dst : memref<4xi32, "
      "#hivm.address_space<ub>>)";
  require(cvub::GetPipe(copy) == cvub::HIVMPipe::Other,
          "UB-to-UB copy must use the vector pipe");

  cvub::OperationRecord scalar;
  scalar.opName = "hivm.hir.vmod";
  scalar.text =
      "hivm.hir.vmod ins(%lhs, %rhs : memref<4xi32, "
      "#hivm.address_space<ub>>, memref<4xi32, "
      "#hivm.address_space<ub>>) outs(%dst : memref<4xi32, "
      "#hivm.address_space<ub>>)";
  require(cvub::shouldLowerToScalarLoops(scalar),
          "i32 vmod must lower to scalar loops");
}

void testFlattenScalarLowering() {
  const std::optional<cvub::MemRefTypeModel> implicitDynamicLayout =
      cvub::ParseMemRefType("memref<128x?xf32>");
  const std::optional<cvub::MemRefTypeModel> explicitDynamicLayout =
      cvub::ParseMemRefType("memref<128x?xf32, strided<[?, 1]>>");
  require(implicitDynamicLayout && explicitDynamicLayout,
          "dynamic flattening test types must parse");
  require(cvub::GetContiguousAxes({*implicitDynamicLayout}) ==
              std::vector<bool>({true, true}),
          "an implicit identity layout must remain uniformly collapsible");
  require(cvub::GetContiguousAxes({*implicitDynamicLayout,
                                   *explicitDynamicLayout}) ==
              std::vector<bool>({true, false}),
          "an explicit dynamic strided layout must block axis collapse");

  std::optional<cvub::MemRefTypeModel> trailingUnitType =
      cvub::ParseMemRefType(
          "memref<1x8x1xi32, strided<[8, 16, 1]>, "
          "#hivm.address_space<ub>>");
  require(trailingUnitType.has_value(), "test memref type must parse");
  cvub::MemRefTypeModel collapsed = cvub::CollapseMemRefType(
      *trailingUnitType, {{0, 1, 2}});
  require(collapsed.strides.size() == 1 && collapsed.strides.front() &&
              *collapsed.strides.front() == 16,
          "collapse must skip a trailing unit dimension when selecting stride");

  cvub::OperationRecord reduce;
  reduce.opName = "hivm.hir.vreduce";
  reduce.text =
      "hivm.hir.vreduce <max_with_index_left> "
      "ins(%src : memref<2x3x4xf32, #hivm.address_space<ub>>) "
      "outs(%dst : memref<1x3x4xf32, #hivm.address_space<ub>>) "
      "reduce_dims = [0]";
  require(!cvub::shouldLowerToScalarLoops(reduce),
          "contiguous rank-3 reduce must flatten to rank 2");

  reduce.text =
      "hivm.hir.vreduce <max_with_index_left> "
      "ins(%src : memref<2x3x4xf32, strided<[24, 8, 1]>, "
      "#hivm.address_space<ub>>) "
      "outs(%dst : memref<1x3x4xf32, #hivm.address_space<ub>>) "
      "reduce_dims = [0]";
  require(cvub::shouldLowerToScalarLoops(reduce),
          "a non-contiguous axis must preserve flattened rank 3");

  reduce.text =
      "hivm.hir.vreduce <max_with_index_left> "
      "ins(%src : memref<2x3x4xf32, #hivm.address_space<ub>>) "
      "outs(%dst : memref<1x3x4xf32, #hivm.address_space<ub>>) "
      "reduce_dims = [0]";
  cvub::StrideAlignmentMap marks;
  marks["%src"].push_back({1, 32});
  require(cvub::shouldLowerToScalarLoops(reduce, marks),
          "stride-alignment marks must participate in flattening");

  cvub::OperationRecord cumsum;
  cumsum.opName = "hivm.hir.vcumsum";
  cumsum.text =
      "hivm.hir.vcumsum "
      "ins(%src : memref<4x8xf32, #hivm.address_space<ub>>) "
      "outs(%dst : memref<4x8xf32, #hivm.address_space<ub>>) "
      "cum_dims = [0] reverse = false";
  require(!cvub::shouldLowerToScalarLoops(cumsum),
          "a non-last cumulative axis must remain vectorized");
  cumsum.text.replace(cumsum.text.find("cum_dims = [0]"), 14,
                      "cum_dims = [1]");
  require(cvub::shouldLowerToScalarLoops(cumsum),
          "a last cumulative axis must lower to scalar loops");
}

void testReusableExtraBufferSemantics() {
  cvub::OperationRecord shift;
  shift.opName = "hivm.hir.vshl";
  shift.text =
      "hivm.hir.vshl ins(%kill, %amount : memref<8xi32, "
      "#hivm.address_space<ub>>, i32) outs(%gen : memref<8xi32, "
      "#hivm.address_space<ub>>) temp_buffer(%tmp : memref<8xi32, "
      "#hivm.address_space<ub>>)";
  const std::map<std::string, std::string> aliases = {
      {"%gen", "%gen"}, {"%kill", "%kill"}, {"%tmp", "%tmp"}};
  const std::set<std::string> allocations = {"%gen", "%kill", "%tmp"};
  require(cvub::IsReuseHIVMOp(shift, "%gen", "%kill", aliases,
                              allocations),
          "vshl scalar temp buffer must follow ExtraBufferOpInterface");
}

void testNormalizeLoopIteratorCopy() {
  const cvub::fs::path input = writeFixture(
      "normalize_loop_iterator.mlir",
      std::string("module {\n  func.func @test() attributes {") +
          kAIVAttribute +
          "} {\n"
          "    %c0 = arith.constant 0 : index\n"
          "    %c1 = arith.constant 1 : index\n"
          "    %c4 = arith.constant 4 : index\n"
          "    %scalar = arith.constant 0.0 : f32\n"
          "    %initial = memref.alloc() : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "    scf.for %i = %c0 to %c4 step %c1 iter_args(%iter = %initial) "
          "-> (memref<4xf32, #hivm.address_space<ub>>) {\n"
          "      %yielded = memref.alloc() : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "      hivm.hir.vbrc ins(%scalar : f32) outs(%yielded : "
          "memref<4xf32, #hivm.address_space<ub>>)\n"
          "      %loaded = memref.load %iter[%c0] : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "      scf.yield %yielded : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "    }\n"
          "    return\n  }\n}\n");
  std::vector<cvub::IRAllocRecord> allocations =
      cvub::qualifyScopedAllocRecords(input);
  std::set<std::string> names;
  std::map<std::string, std::string> types;
  for (const cvub::IRAllocRecord &allocation : allocations) {
    names.insert(allocation.name);
    types[allocation.name] = allocation.memrefType;
  }
  std::vector<cvub::OperationRecord> operations =
      cvub::NormalizeIterUseAfterYieldInit(
          cvub::qualifyScopedSSAValues(
              cvub::parseFunctionOperations(input, "AIV")),
          names, types);
  size_t copyCount = 0;
  int copyBlockId = -1;
  int yieldBlockId = -1;
  for (const cvub::OperationRecord &operation : operations) {
    if (operation.opName == "scf.yield")
      yieldBlockId = operation.blockId;
    if (operation.opName != "hivm.hir.copy")
      continue;
    ++copyCount;
    copyBlockId = operation.blockId;
    require(operation.text.find("normalize-loop-iterator") ==
                std::string::npos,
            "normalized copy must not use a private marker");
    require(cvub::GetPipe(operation) == cvub::HIVMPipe::Other,
            "normalized UB copy must infer the vector pipe from its types");
  }
  require(copyCount == 1,
          "NormalizeIterUseAfterYieldInit must insert exactly one copy");
  require(copyBlockId == yieldBlockId && copyBlockId >= 0,
          "normalized copy must stay in the yield operation's block");
}

void testSingleResultValueIdentity() {
  const cvub::fs::path input = writeFixture(
      "single_result_value_identity.mlir",
      std::string("module {\n  func.func @test() attributes {") +
          kAIVAttribute +
          "} {\n"
          "    %value = test.producer : i32\n"
          "    test.consumer %value#0 : i32\n"
          "    return\n  }\n}\n");
  std::vector<cvub::OperationRecord> operations =
      cvub::qualifyScopedSSAValues(
          cvub::parseFunctionOperations(input, "AIV"));
  bool foundConsumer = false;
  for (const cvub::OperationRecord &operation : operations) {
    if (operation.opName != "test.consumer")
      continue;
    foundConsumer = true;
    std::vector<std::string> operands =
        cvub::operationOperandNames(operation);
    require(operands.size() == 1 && operands.front() == "%value",
            "single-result %value and %value#0 must identify one Value");
  }
  require(foundConsumer, "single-result identity fixture must be parsed");
}

void testUnknownLocalOperationBlocker() {
  const cvub::fs::path input = writeFixture(
      "unknown_local_op.mlir",
      std::string("module {\n  func.func @test(%arg0: memref<4xi32, ") +
          "#hivm.address_space<gm>>) attributes {" + kAIVAttribute +
          "} {\n"
          "    %alloc = memref.alloc() : memref<4xi32, "
          "#hivm.address_space<ub>>\n"
          "    hivm.hir.load ins(%arg0 : memref<4xi32, "
          "#hivm.address_space<gm>>) outs(%alloc : memref<4xi32, "
          "#hivm.address_space<ub>>)\n"
          "    test.unknown %alloc : memref<4xi32, "
          "#hivm.address_space<ub>>\n"
          "    return\n  }\n}\n");
  requireThrows(
      [&] { (void)cvub::PlanLocalMemoryForSeed(input, 0); },
      "unknown operation touching UB must be a blocker");
}

void testTargetFunctionCardinality() {
  const std::string function =
      std::string("  func.func @NAME() attributes {") + kAIVAttribute +
      "} {\n    return\n  }\n";
  std::string twoFunctions = "module {\n";
  std::string first = function;
  first.replace(first.find("NAME"), 4, "first");
  std::string second = function;
  second.replace(second.find("NAME"), 4, "second");
  twoFunctions += first + second + "}\n";
  const cvub::fs::path multiple =
      writeFixture("multiple_aiv.mlir", twoFunctions);
  requireThrows(
      [&] { (void)cvub::PlanLocalMemoryForSeed(multiple, 0); },
      "multiple AIV functions must be a blocker");

  const cvub::fs::path noAIV = writeFixture(
      "no_aiv.mlir",
      "module {\n  func.func @cube() attributes {hivm.func_core_type = "
      "#hivm.func_core_type<AIC>} {\n    return\n  }\n}\n");
  cvub::PlanMemoryModelResult result =
      cvub::PlanLocalMemoryForSeed(noAIV, 0);
  require(result.success && result.peakBits == 0 && result.buffers.empty(),
          "an input without AIV must have exact zero AIV UB peak");
}

void testRestrictInplacePropagation() {
  const cvub::fs::path input = writeFixture(
      "restrict_inplace.mlir",
      std::string("module {\n  func.func @test(%arg0: memref<4xf32, ") +
          "#hivm.address_space<gm>>) attributes {" + kAIVAttribute +
          "} {\n"
          "    %input = memref.alloc() : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "    hivm.hir.load ins(%arg0 : memref<4xf32, "
          "#hivm.address_space<gm>>) outs(%input : memref<4xf32, "
          "#hivm.address_space<ub>>)\n"
          "    %output = memref.alloc() : memref<4xf32, "
          "#hivm.address_space<ub>>\n"
          "    hivm.hir.vexp ins(%input : memref<4xf32, "
          "#hivm.address_space<ub>>) outs(%output : memref<4xf32, "
          "#hivm.address_space<ub>>)\n"
          "    return\n  }\n}\n");
  cvub::LifetimeAnalysis unrestricted =
      cvub::analyzeLifetimes(input, "AIV", 0, false);
  cvub::LifetimeAnalysis restricted =
      cvub::analyzeLifetimes(input, "AIV", 0, true);
  require(!unrestricted.inplacePairList.empty(),
          "unrestricted model must allow the vexp inplace pair");
  require(restricted.inplacePairList.empty(),
          "restrict-inplace-as-isa must remove the vexp inplace pair");
}

void testCheckedArithmetic() {
  requireThrows(
      [] {
        (void)cvub::GetBufferConstBits(
            "memref<18446744073709551615x2xi64, "
            "#hivm.address_space<ub>>");
      },
      "oversized static memref must be a blocker");
  requireThrows(
      [] {
        (void)cvub::AlignUp(std::numeric_limits<uint64_t>::max(), 256);
      },
      "overflowing alignment must be a blocker");
}

void testWhileScopeBranchAndPreloadSemantics() {
  const std::string ubType =
      "memref<4xi32, #hivm.address_space<ub>>";
  const std::string gmType =
      "memref<4xi32, #hivm.address_space<gm>>";
  const cvub::fs::path whileInput = writeFixture(
      "while_alias.mlir",
      "module {\n  func.func @test(%src: " + gmType + ", %dst: " + gmType +
          ") attributes {" + kAIVAttribute +
          "} {\n    %init = memref.alloc() : " + ubType +
          "\n    hivm.hir.load ins(%src : " + gmType + ") outs(%init : " +
          ubType +
          ")\n    %result = scf.while (%iter = %init) : (" + ubType +
          ") -> " + ubType +
          " {\n      %cond = arith.constant true\n      scf.condition(%cond) %iter : " +
          ubType +
          "\n    } do {\n    ^bb0(%after: " + ubType +
          "):\n      %next = memref.alloc() : " + ubType +
          "\n      hivm.hir.vadd ins(%after, %after : " + ubType + ", " +
          ubType + ") outs(%next : " + ubType +
          ")\n      scf.yield %next : " + ubType +
          "\n    }\n    hivm.hir.store ins(%result : " + ubType +
          ") outs(%dst : " + gmType + ")\n    return\n  }\n}\n");
  cvub::LifetimeAnalysis whileAnalysis =
      cvub::analyzeLifetimes(whileInput, "AIV", 0, false);
  require(whileAnalysis.canonicalAllocByValue.count("%result") != 0,
          "while result must alias its after-region argument");
  size_t whilePoints = 0;
  for (const cvub::OperationRecord &operation : whileAnalysis.operations)
    if (operation.opName == "scf.while" ||
        operation.opName == "scf.while.end")
      ++whilePoints;
  require(whilePoints == 2,
          "RecursiveWhileOp must emit begin and end linear points");

  const cvub::fs::path scopePreloadInput = writeFixture(
      "scope_preload.mlir",
      "module {\n  func.func @test(%dst: " + gmType + ") attributes {" +
          kAIVAttribute +
          "} {\n    %c0 = arith.constant 0 : index\n"
          "    %c1 = arith.constant 1 : index\n"
          "    scf.for %i = %c0 to %c1 step %c1 {\n"
          "      %result = scope.scope : () -> " + ubType +
          " {\n        %preload = memref.alloc() : " + ubType +
          "\n        annotation.mark %preload {hivm.multi_buffer = 2 : i32, "
          "hivm.preload_local_buffer = 1 : i32} : " + ubType +
          "\n        %zero = arith.constant 0 : i32\n"
          "        hivm.hir.vbrc ins(%zero : i32) outs(%preload : " +
          ubType +
          ")\n        scope.return %preload : " + ubType +
          "\n      } {no_inline}\n      hivm.hir.store ins(%result : " + ubType +
          ") outs(%dst : " + gmType +
          ")\n    }\n    return\n  }\n}\n");
  cvub::LifetimeAnalysis preloadAnalysis =
      cvub::analyzeLifetimes(scopePreloadInput, "AIV", 0, false);
  const cvub::LifetimeRecord *preloadLife = nullptr;
  for (const cvub::LifetimeRecord &record : preloadAnalysis.records)
    if (record.name == "%preload")
      preloadLife = &record;
  require(preloadLife != nullptr, "preload allocation lifetime must exist");
  int forBegin = -1;
  int forEnd = -1;
  for (size_t i = 0; i < preloadAnalysis.operations.size(); ++i) {
    if (preloadAnalysis.operations[i].opName == "scf.for")
      forBegin = static_cast<int>(i);
    if (preloadAnalysis.operations[i].opName == "scf.for.end")
      forEnd = static_cast<int>(i);
  }
  require(preloadLife->directAllocTime == forBegin &&
              preloadLife->directFreeTime == forEnd,
          "preload gen/kill must move to the parent for begin/end points");
  require(preloadAnalysis.canonicalAllocByValue.count("%result") != 0,
          "scope result must alias scope.return operand");

  const cvub::fs::path branchInput = writeFixture(
      "branch_alias.mlir",
      "module {\n  func.func @test(%src: " + gmType + ", %dst: " + gmType +
          ", %cond: i1) attributes {" + kAIVAttribute +
          "} {\n    %input = memref.alloc() : " + ubType +
          "\n    hivm.hir.load ins(%src : " + gmType + ") outs(%input : " +
          ubType +
          ")\n    cf.cond_br %cond, ^bb1(%input : " + ubType +
          "), ^bb2(%input : " + ubType +
          ")\n  ^bb1(%left: " + ubType +
          "):\n    cf.br ^bb3(%left : " + ubType +
          ")\n  ^bb2(%right: " + ubType +
          "):\n    cf.br ^bb3(%right : " + ubType +
          ")\n  ^bb3(%joined: " + ubType +
          "):\n    hivm.hir.store ins(%joined : " + ubType +
          ") outs(%dst : " + gmType + ")\n    return\n  }\n}\n");
  cvub::LifetimeAnalysis branchAnalysis =
      cvub::analyzeLifetimes(branchInput, "AIV", 0, false);
  require(branchAnalysis.canonicalAllocByValue.count("%joined") == 0,
          "greedy CFG simplification must remove a redundant block argument");
  const cvub::LifetimeRecord *branchInputLife = nullptr;
  for (const cvub::LifetimeRecord &record : branchAnalysis.records)
    if (record.name == "%input")
      branchInputLife = &record;
  require(branchInputLife != nullptr && branchInputLife->directFreeTime >= 0,
          "the replacement allocation must remain live through its final use");
}

void testDynamicExtentMatchesPlanMemoryBoundary() {
  requireThrowsContaining(
      [] {
        (void)cvub::GetBufferConstBits(
            "memref<?xi32, #hivm.address_space<ub>>");
      },
      "buffer shape size which should be static",
      "dynamic local extent must fail at PlanMemory's static-shape boundary");
}

void testSubByteBufferReportsOriginalExtent() {
  const cvub::fs::path input = writeFixture(
      "sub_byte_extent.mlir",
      std::string("module {\n  func.func @test() attributes {") +
          kAIVAttribute +
          "} {\n"
          "    %false = arith.constant false\n"
          "    %buffer = memref.alloc() : memref<3xi1, "
          "#hivm.address_space<ub>>\n"
          "    hivm.hir.vbrc ins(%false : i1) outs(%buffer : "
          "memref<3xi1, #hivm.address_space<ub>>)\n"
          "    return\n  }\n}\n");
  const cvub::PlanMemoryModelResult result =
      cvub::PlanLocalMemoryForSeed(input, 0);
  require(result.success && result.buffers.size() == 1,
          "sub-byte buffer fixture must produce one successful plan entry");
  require(result.buffers.front().constBits == 3 &&
              result.buffers.front().extentBits == 3,
          "planned output must preserve PlanMemory BufferInfo::constBits");
  require(result.peakBits == 8,
          "reported UB peak must still byte-align a sub-byte buffer");
}

void testOverflowReportsCompleteFallbackPlanPeak() {
  const cvub::fs::path input = writeFixture(
      "overflow_fallback_peak.mlir",
      std::string("module {\n  func.func @test() attributes {") +
          kAIVAttribute +
          "} {\n"
          "    %false = arith.constant false\n"
          "    %buffer = memref.alloc() : memref<196609xi8, "
          "#hivm.address_space<ub>>\n"
          "    hivm.hir.vbrc ins(%false : i1) outs(%buffer : "
          "memref<196609xi8, #hivm.address_space<ub>>)\n"
          "    return\n  }\n}\n");
  const cvub::PlanMemoryModelResult result =
      cvub::PlanLocalMemoryForSeed(input, 0);
  require(!result.success && result.overflow,
          "oversized UB fixture must report overflow");
  require(result.buffers.size() == 1 &&
              result.buffers.front().offsetsBytes ==
                  std::vector<uint64_t>{0},
          "overflow fallback must retain the complete buffer placement");
  require(result.requiredBits == 1573120,
          "required capacity must preserve StorageEntry alignment");
  require(result.peakBits == 1572872,
          "overflow peak must use the planned offset and original extent");
}

void testSetBufferSizeErasesAttributeEmptyMark() {
  const cvub::fs::path input = writeFixture(
      "set_buffer_size_mark.mlir",
      R"mlir("builtin.module"() ({
  "func.func"() <{function_type = (memref<?xi8>, index) -> (), sym_name = "test"}> ({
  ^bb0(%workspace: memref<?xi8>, %size: index):
    %alloc = "memref_ext.alloc_workspace"(%workspace, %size) <{operandSegmentSizes = array<i32: 1, 1, 0>}> : (memref<?xi8>, index) -> memref<128x?xf32>
    "annotation.mark"(%alloc) <{effects = ["write"]}> {buffer_size_in_byte = 16384 : i64} : (memref<128x?xf32>) -> ()
    "func.return"() : () -> ()
  }) : () -> ()
}) : () -> ()
)mlir");
  cvub::GenericModule module = cvub::RunInferAndSetBufferSizePipeline(
      cvub::ParseGenericIR(input, false));
  bool foundBacking = false;
  bool foundView = false;
  for (const cvub::GenericOperation &operation : module.operations) {
    require(operation.name != "annotation.mark",
            "SetBufferSize must erase a mark with no discardable attributes");
    if (operation.name == "memref_ext.alloc_workspace" &&
        operation.resultTypes == std::vector<std::string>{"memref<16384xi8>"})
      foundBacking = true;
    if (operation.name == "memref.view" &&
        operation.resultTypes ==
            std::vector<std::string>{"memref<128x?xf32>"})
      foundView = true;
  }
  require(foundBacking && foundView,
          "SetBufferSize must materialize the static byte allocation and view");
}

} // namespace

int main() {
  try {
    testOperationSemantics();
    testFlattenScalarLowering();
    testReusableExtraBufferSemantics();
    testNormalizeLoopIteratorCopy();
    testSingleResultValueIdentity();
    testUnknownLocalOperationBlocker();
    testTargetFunctionCardinality();
    testRestrictInplacePropagation();
    testCheckedArithmetic();
    testWhileScopeBranchAndPreloadSemantics();
    testDynamicExtentMatchesPlanMemoryBoundary();
    testSubByteBufferReportsOriginalExtent();
    testOverflowReportsCompleteFallbackPlanPeak();
    testSetBufferSizeErasesAttributeEmptyMark();
    std::cout << "MODEL_CORE_TESTS=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
