#include "../src/ir/semantic_ir.hpp"
#include "../src/ir/generic_ir.hpp"
#include "../src/pipeline/bufferized_semantic_ir.hpp"
#include "../src/passes/affine_min_max_canonicalization.hpp"
#include "../src/passes/canonicalization_hivm_pipeline.hpp"
#include "../src/passes/clone_tensor_empty.hpp"
#include "../src/passes/global_workspace_plan.hpp"
#include "../src/passes/loop_invariant_code_motion.hpp"
#include "../src/passes/split_mix_kernel.hpp"
#include "../src/passes/tile_and_bind_sub_block.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

int main() {
  try {
    const cvub::fs::path input =
        cvub::fs::temp_directory_path() / "cvub_semantic_ir_smoke.mlir";
    std::ofstream out(input);
    out << "module {\n"
           "  func.func @test(%arg0: i1) attributes {"
           "hivm.func_core_type = #hivm.func_core_type<AIV>} {\n"
           "    %alloc = memref.alloc() : memref<4xi32, "
           "#hivm.address_space<ub>>\n"
           "    cf.cond_br %arg0, ^bb1(%alloc : memref<4xi32, "
           "#hivm.address_space<ub>>), ^bb2(%alloc : memref<4xi32, "
           "#hivm.address_space<ub>>)\n"
           "  ^bb1(%left: memref<4xi32, #hivm.address_space<ub>>):\n"
           "    return\n"
           "  ^bb2(%right: memref<4xi32, #hivm.address_space<ub>>):\n"
           "    return\n"
           "  }\n"
           "}\n";
    out.close();

    std::vector<cvub::OperationRecord> operations =
        cvub::qualifyScopedSSAValues(
            cvub::parseFunctionOperations(input, "AIV"));
    if (operations.empty())
      throw std::runtime_error("SemanticIR parser returned no operations");
    auto branch = std::find_if(
        operations.begin(), operations.end(), [](const auto &operation) {
          return operation.opName == "cf.cond_br";
        });
    if (branch == operations.end() || branch->branchDestinations.size() != 2)
      throw std::runtime_error("SemanticIR CFG destinations are incomplete");
    if (cvub::operationOperandNames(*branch).size() != 3)
      throw std::runtime_error("SemanticIR operand identity is incomplete");

    const cvub::fs::path genericPath =
        cvub::fs::temp_directory_path() / "cvub_c1_generic_smoke.mlir";
    std::ofstream genericOut(genericPath);
    genericOut << "\"builtin.module\"() ({\n"
                  "  \"func.func\"() <{function_type = (i1) -> (), "
                  "sym_name = \"test\"}> ({\n"
                  "  ^bb0(%arg0: i1):\n"
                  "    %0 = \"arith.constant\"() <{value = 1 : i32}> : "
                  "() -> i32\n"
                  "    \"scf.if\"(%arg0) ({\n"
                  "      \"scf.yield\"() : () -> ()\n"
                  "    }, {\n"
                  "      \"scf.yield\"() : () -> ()\n"
                  "    }) : (i1) -> ()\n"
                  "    \"func.return\"() : () -> ()\n"
                  "  }) : () -> ()\n"
                  "}) {tag = \"smoke\"} : () -> ()\n";
    genericOut.close();
    cvub::GenericModule genericModule = cvub::ParseGenericIR(genericPath);
    if (genericModule.operations.size() != 7 ||
        genericModule.regions.size() != 4 || genericModule.blocks.size() != 4)
      throw std::runtime_error("generic IR hierarchy parsing is incomplete");
    if (genericModule.operations.front().attributes != "{tag = \"smoke\"}" ||
        genericModule.operations.at(3).operands != std::vector<int>{0})
      throw std::runtime_error("generic IR attributes or def-use parsing is incomplete");

    const cvub::fs::path invalidTypePath =
        cvub::fs::temp_directory_path() / "cvub_invalid_generic_type.mlir";
    std::ofstream invalidTypeOut(invalidTypePath);
    invalidTypeOut << "\"builtin.module\"() ({\n"
                      "  \"func.func\"() <{function_type = () -> (), "
                      "sym_name = \"invalid\"}> ({\n"
                      "    %0 = \"memref.alloc\"() : () -> tensor<4xf32>\n"
                      "    \"func.return\"() : () -> ()\n"
                      "  }) : () -> ()\n"
                      "}) : () -> ()\n";
    invalidTypeOut.close();
    bool rejectedInvalidType = false;
    try {
      (void)cvub::ParseGenericIR(invalidTypePath);
    } catch (const std::runtime_error &error) {
      rejectedInvalidType =
          std::string(error.what()).find(
              "memref.alloc expected one memref result") !=
          std::string::npos;
    }
    if (!rejectedInvalidType)
      throw std::runtime_error("generic IR accepted an invalid alloc type");

    const cvub::fs::path splitMixPath =
        cvub::fs::temp_directory_path() / "cvub_split_mix_dead_loop.mlir";
    std::ofstream splitMixOut(splitMixPath);
    splitMixOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = () -> (), sym_name = "
           "\"split_mix\"}> ({\n"
           "    %0 = \"arith.constant\"() <{value = 0 : index}> : () -> "
           "index\n"
           "    %1 = \"arith.constant\"() <{value = 4 : index}> : () -> "
           "index\n"
           "    %2 = \"arith.constant\"() <{value = 1 : index}> : () -> "
           "index\n"
           "    \"scf.for\"(%0, %1, %2) ({\n"
           "    ^bb0(%arg0: index):\n"
           "      %3 = \"arith.addi\"(%arg0, %2) : (index, index) -> "
           "index\n"
           "      \"scf.yield\"() : () -> ()\n"
           "    }) : (index, index, index) -> ()\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) {hivm.func_core_type = #hivm.func_core_type<MIX>} : () -> "
           "()\n"
           "}) : () -> ()\n";
    splitMixOut.close();
    cvub::GenericModule splitMix =
        cvub::RunSplitMixKernel(cvub::ParseGenericIR(splitMixPath));
    if (std::any_of(splitMix.operations.begin(), splitMix.operations.end(),
                    [](const cvub::GenericOperation &operation) {
                      return operation.name == "scf.for" ||
                             operation.name == "arith.addi";
                    }))
      throw std::runtime_error(
          "SplitMixKernel retained a filtered zero-trip CUBE loop");

    const cvub::fs::path tileFolderPath =
        cvub::fs::temp_directory_path() / "cvub_tile_folder_order.mlir";
    std::ofstream tileFolderOut(tileFolderPath);
    tileFolderOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = (memref<?xi8>) -> (), "
           "sym_name = \"tile_folder\"}> ({\n"
           "  ^bb0(%arg0: memref<?xi8>):\n"
           "    %0 = \"arith.constant\"() <{value = 0 : index}> : () -> "
           "index\n"
           "    %1 = \"arith.constant\"() <{value = 1 : index}> : () -> "
           "index\n"
           "    %2 = \"arith.constant\"() <{value = 2 : index}> : () -> "
           "index\n"
           "    \"scf.for\"(%0, %2, %1) ({\n"
           "    ^bb0(%arg1: index):\n"
           "      %3 = \"arith.constant\"() <{value = 32768 : index}> : "
           "() -> index\n"
           "      %4 = \"arith.constant\"() <{value = 49152 : index}> : "
           "() -> index\n"
           "      %5 = \"memref_ext.alloc_workspace\"(%arg0, %3) : "
           "(memref<?xi8>, index) -> memref<64xi8>\n"
           "      %6 = \"memref_ext.alloc_workspace\"(%arg0, %4) : "
           "(memref<?xi8>, index) -> memref<64xi8>\n"
           "      \"scf.yield\"() : () -> ()\n"
           "    }) {map_for_to_forall} : (index, index, index) -> ()\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    tileFolderOut.close();
    cvub::GenericModule tileFolder = cvub::ParseGenericIR(tileFolderPath);
    const auto tileFunction = std::find_if(
        tileFolder.operations.begin(), tileFolder.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "func.func";
        });
    if (tileFunction == tileFolder.operations.end())
      throw std::runtime_error("TileAndBind folder fixture has no function");
    cvub::RunTileAndBindOperationFolder(tileFolder, tileFunction->id);
    const int tileEntry =
        tileFolder.regions
            .at(static_cast<size_t>(tileFunction->regions.front()))
            .blocks.front();
    std::vector<std::string> tileConstants;
    for (int operationId :
         tileFolder.blocks.at(static_cast<size_t>(tileEntry)).operations) {
      const cvub::GenericOperation &operation =
          tileFolder.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "arith.constant")
        tileConstants.push_back(
            cvub::FindDictionaryValue(operation.properties, "value"));
    }
    if (tileConstants.size() < 2 || tileConstants[0] != "49152 : index" ||
        tileConstants[1] != "32768 : index")
      throw std::runtime_error(
          "TileAndBind operation folder did not mirror greedy reverse order");

    const cvub::fs::path workspacePath =
        cvub::fs::temp_directory_path() / "cvub_global_workspace_order.mlir";
    std::ofstream workspaceOut(workspacePath);
    workspaceOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = (memref<?xi8>) -> (), "
           "sym_name = \"workspace\"}> ({\n"
           "  ^bb0(%arg0: memref<?xi8>):\n"
           "    %0 = \"arith.constant\"() <{value = 0 : index}> : () -> "
           "index\n"
           "    %1 = \"memref_ext.alloc_workspace\"(%arg0) "
           "<{operandSegmentSizes = array<i32: 1, 0, 0>}> : "
           "(memref<?xi8>) -> memref<64xi8>\n"
           "    %2 = \"memref_ext.alloc_workspace\"(%arg0) "
           "<{operandSegmentSizes = array<i32: 1, 0, 0>}> : "
           "(memref<?xi8>) -> memref<64xi8>\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    workspaceOut.close();
    cvub::GenericModule workspace = cvub::RunGlobalWorkspacePlan(
        cvub::ParseGenericIR(workspacePath));
    const auto workspaceFunction = std::find_if(
        workspace.operations.begin(), workspace.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "func.func";
        });
    const int workspaceEntry =
        workspace.regions
            .at(static_cast<size_t>(workspaceFunction->regions.front()))
            .blocks.front();
    std::vector<std::string> workspaceConstants;
    for (int operationId :
         workspace.blocks.at(static_cast<size_t>(workspaceEntry)).operations) {
      const cvub::GenericOperation &operation =
          workspace.operations.at(static_cast<size_t>(operationId));
      if (operation.name == "arith.constant")
        workspaceConstants.push_back(
            cvub::FindDictionaryValue(operation.properties, "value"));
    }
    if (workspaceConstants !=
        std::vector<std::string>{"64 : index", "0 : index"}) {
      std::ostringstream values;
      for (const std::string &value : workspaceConstants)
        values << '[' << value << ']';
      throw std::runtime_error(
          "GlobalWorkspacePlan did not reuse zero and hoist new offsets: " +
          values.str());
    }

    const cvub::fs::path replicateEmptyPath =
        cvub::fs::temp_directory_path() / "cvub_replicate_out_empty.mlir";
    std::ofstream replicateEmptyOut(replicateEmptyPath);
    replicateEmptyOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = (i32, index) -> (), "
           "sym_name = \"replicate_empty\"}> ({\n"
           "  ^bb0(%arg0: i32, %arg1: index):\n"
           "    %0 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "    %1 = \"tensor.insert\"(%arg0, %0, %arg1) : "
           "(i32, tensor<4xi32>, index) -> tensor<4xi32>\n"
           "    %2 = \"tensor.insert\"(%arg0, %0, %arg1) : "
           "(i32, tensor<4xi32>, index) -> tensor<4xi32>\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    replicateEmptyOut.close();
    cvub::GenericModule replicated =
        cvub::ParseGenericIR(replicateEmptyPath);
    const auto replicatedFunction = std::find_if(
        replicated.operations.begin(), replicated.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "func.func";
        });
    cvub::RunReplicateOutEmptyTensor(replicated, replicatedFunction->id);
    std::vector<int> emptyValues;
    std::vector<int> insertDestinations;
    for (const cvub::GenericOperation &operation : replicated.operations) {
      if (operation.name == "tensor.empty")
        emptyValues.push_back(operation.results.front());
      if (operation.name == "tensor.insert")
        insertDestinations.push_back(operation.operands.at(1));
    }
    if (emptyValues.size() != 3 || insertDestinations.size() != 2 ||
        insertDestinations[0] == insertDestinations[1] ||
        std::find(insertDestinations.begin(), insertDestinations.end(),
                  emptyValues.front()) != insertDestinations.end())
      throw std::runtime_error(
          "ReplicateOutEmptyTensor did not clone every DPS init use");

    const cvub::fs::path hoistAffinePath =
        cvub::fs::temp_directory_path() / "cvub_hoist_affine.mlir";
    std::ofstream hoistAffineOut(hoistAffinePath);
    hoistAffineOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = (i32) -> (), sym_name = "
           "\"hoist_affine\"}> ({\n"
           "  ^bb0(%arg0: i32):\n"
           "    %0 = \"arith.index_cast\"(%arg0) : (i32) -> index\n"
           "    %1 = \"affine.apply\"(%0) <{map = "
           "affine_map<()[s0] -> (s0)>}> : (index) -> index\n"
           "    %2 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "    %3 = \"affine.apply\"(%0) <{map = "
           "affine_map<()[s0] -> (s0 + 1)>}> : (index) -> index\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    hoistAffineOut.close();
    cvub::GenericModule hoisted = cvub::ParseGenericIR(hoistAffinePath);
    const auto hoistedFunction = std::find_if(
        hoisted.operations.begin(), hoisted.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "func.func";
        });
    cvub::RunTileAndBindHoistAffine(hoisted, hoistedFunction->id);
    const int hoistedEntry =
        hoisted.regions
            .at(static_cast<size_t>(hoistedFunction->regions.front()))
            .blocks.front();
    std::vector<std::string> hoistedNames;
    for (int operationId :
         hoisted.blocks.at(static_cast<size_t>(hoistedEntry)).operations)
      hoistedNames.push_back(
          hoisted.operations.at(static_cast<size_t>(operationId)).name);
    if (hoistedNames !=
        std::vector<std::string>{"arith.index_cast", "affine.apply",
                                 "affine.apply", "tensor.empty",
                                 "func.return"})
      throw std::runtime_error(
          "HIVMBubbleUpExtractSlice HoistAffine cluster is incorrect");

    const cvub::fs::path controlFlow =
        cvub::fs::temp_directory_path() / "cvub_c1_cfg_smoke.mlir";
    std::ofstream controlFlowOut(controlFlow);
    controlFlowOut << "\"builtin.module\"() ({\n"
                      "  \"func.func\"() <{function_type = () -> (), "
                      "sym_name = \"cfg\"}> ({\n"
                      "  ^bb0:\n"
                      "    \"cf.br\"()[^bb1] : () -> ()\n"
                      "  ^bb1:\n"
                      "    \"func.return\"() : () -> ()\n"
                      "  }) : () -> ()\n"
                      "}) : () -> ()\n";
    controlFlowOut.close();
    cvub::GenericModule cfg = cvub::ParseGenericIR(controlFlow);
    if (cfg.operations.size() != 4 ||
        cfg.operations.at(2).successors != std::vector<int>{2})
      throw std::runtime_error("generic IR successor identity is incomplete");

    cvub::GenericModule source;
    cvub::GenericOperation empty;
    empty.id = 0;
    empty.name = "tensor.empty";
    empty.results = {0};
    empty.resultTypes = {"tensor<4xi32>"};
    source.operations.push_back(empty);
    cvub::GenericOperation cast;
    cast.id = 1;
    cast.name = "tensor.cast";
    cast.results = {1};
    cast.operands = {0};
    cast.resultTypes = {"tensor<?xi32>"};
    cast.operandTypes = {"tensor<4xi32>"};
    source.operations.push_back(cast);
    cvub::OneShotBufferizationResult bufferization;
    bufferization.allocations.push_back(
        {"memref<4xi32>", "64 : i64", 0, "result:0:0", {}, {}});
    const cvub::BufferizedSemanticIR bufferized =
        cvub::BuildBufferizedSemanticIR(source, bufferization);
    if (bufferized.values.size() != 2 || bufferized.accesses.size() != 1 ||
        bufferized.values.at(0).bufferId != "local:0" ||
        bufferized.values.at(1).bufferId != "local:0" ||
        bufferized.values.at(1).type != "memref<?xi32>" ||
        bufferized.accesses.front().bufferId != "local:0")
      throw std::runtime_error("bufferized buffer identity is incomplete");
    const std::string serialized =
        cvub::SerializeBufferizedSemanticIR(bufferized);
    if (serialized.find("ACCESS\t1\t0\t6c6f63616c3a30\n") ==
        std::string::npos)
      throw std::runtime_error("bufferized access serialization is unstable");

    const cvub::fs::path licmPath =
        cvub::fs::temp_directory_path() / "cvub_licm_smoke.mlir";
    std::ofstream licmOut(licmPath);
    licmOut << "\"builtin.module\"() ({\n"
               "  \"func.func\"() <{function_type = (index) -> (), "
               "sym_name = \"licm\"}> ({\n"
               "  ^bb0(%arg0: index):\n"
               "    %0 = \"arith.constant\"() <{value = 0 : index}> : "
               "() -> index\n"
               "    \"scf.for\"(%0, %arg0, %arg0) ({\n"
               "    ^bb0(%arg1: index):\n"
               "      %1 = \"arith.index_cast\"(%arg0) : (index) -> i64\n"
               "      %2 = \"affine.max\"(%1) : (i64) -> i64\n"
               "      %3 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
               "      %6 = \"hivm.hir.vbrc\"(%arg0, %3) : "
               "(index, tensor<4xi32>) -> tensor<4xi32>\n"
               "      %4 = \"arith.addi\"(%arg1, %arg0) : "
               "(index, index) -> index\n"
               "      %5 = \"memref.alloc\"() : () -> memref<4xi32>\n"
               "      \"scf.yield\"() : () -> ()\n"
               "    }) : (index, index, index) -> ()\n"
               "    \"func.return\"() : () -> ()\n"
               "  }) : () -> ()\n"
               "}) : () -> ()\n";
    licmOut.close();
    cvub::GenericModule licm = cvub::ParseGenericIR(licmPath);
    const auto loop = std::find_if(
        licm.operations.begin(), licm.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "scf.for";
        });
    if (loop == licm.operations.end())
      throw std::runtime_error("LICM test loop was not parsed");
    const int loopId = loop->id;
    const int parentBlock = loop->blockId;
    const int bodyBlock =
        licm.regions.at(static_cast<size_t>(loop->regions.front()))
            .blocks.front();
    if (cvub::RunLoopInvariantCodeMotion(licm) != 4)
      throw std::runtime_error("LICM did not hoist the invariant worklist");
    std::vector<std::string> parentNames;
    for (int operation :
         licm.blocks.at(static_cast<size_t>(parentBlock)).operations)
      parentNames.push_back(
          licm.operations.at(static_cast<size_t>(operation)).name);
    const std::vector<std::string> expectedParent = {
        "arith.constant", "arith.index_cast", "affine.max", "tensor.empty",
        "hivm.hir.vbrc", "scf.for", "func.return"};
    if (parentNames != expectedParent)
      throw std::runtime_error("LICM changed invariant operation order");
    std::vector<std::string> bodyNames;
    for (int operation :
         licm.blocks.at(static_cast<size_t>(bodyBlock)).operations)
      bodyNames.push_back(
          licm.operations.at(static_cast<size_t>(operation)).name);
    const std::vector<std::string> expectedBody = {
        "arith.addi", "memref.alloc", "scf.yield"};
    if (bodyNames != expectedBody ||
        licm.operations.at(static_cast<size_t>(loopId)).blockId != parentBlock)
      throw std::runtime_error("LICM moved a variant or effecting operation");

    cvub::GenericModule whileLoop = cvub::ParseGenericIR(licmPath);
    auto whileOperation = std::find_if(
        whileLoop.operations.begin(), whileLoop.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "scf.for";
        });
    whileOperation->name = "scf.while";
    if (cvub::RunLoopInvariantCodeMotion(whileLoop) != 4)
      throw std::runtime_error("LICM did not process a LoopLike scf.while op");

    cvub::GenericModule affineModule;
    auto appendAffineOperation = [&](std::string name, std::vector<int> results,
                                     std::vector<int> operands,
                                     std::vector<std::string> resultTypes,
                                     std::vector<std::string> operandTypes) {
      cvub::GenericOperation operation;
      operation.id = static_cast<int>(affineModule.operations.size());
      operation.name = std::move(name);
      operation.results = std::move(results);
      operation.operands = std::move(operands);
      operation.resultTypes = std::move(resultTypes);
      operation.operandTypes = std::move(operandTypes);
      affineModule.operations.push_back(std::move(operation));
    };
    appendAffineOperation("arith.constant", {0}, {}, {"index"}, {});
    appendAffineOperation("arith.constant", {1}, {}, {"index"}, {});
    appendAffineOperation("arith.index_cast", {2}, {20}, {"index"}, {"i32"});
    appendAffineOperation("arith.subi", {3}, {2, 1}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.maxsi", {4}, {3, 1}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.minsi", {5}, {4, 0}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.minsi", {6}, {2, 5}, {"index"},
                          {"index", "index"});
    cvub::ConvertArithToAffineState affineState =
        cvub::RunConvertArithToAffine(affineModule);
    cvub::RunAffineMinMaxCanonicalization(affineModule, affineState);
    if (affineState.replacementOperands.at(6) != std::vector<int>({2, 4}))
      throw std::runtime_error(
          "affine min canonicalization did not merge same-kind producer");

    const std::string cancelledAffineExpression =
        cvub::MakeAffineBinaryExpression(
            "add",
            cvub::MakeAffineBinaryExpression("add", "c(16)",
                                             cvub::MakeAffineBinaryExpression(
                                                 "add", "v(40)",
                                                 "mul(c(-1),v(41))")),
            cvub::MakeAffineBinaryExpression("add", "v(41)",
                                             "mul(c(-1),v(40))"));
    if (cancelledAffineExpression != "c(16)")
      throw std::runtime_error(
          "affine linear simplification did not cancel opposite SSA terms");

    appendAffineOperation("arith.index_cast", {7}, {21}, {"index"},
                          {"i32"});
    appendAffineOperation("arith.index_cast", {8}, {22}, {"index"},
                          {"i32"});
    appendAffineOperation("arith.index_cast", {9}, {23}, {"index"},
                          {"i32"});
    appendAffineOperation("arith.muli", {10}, {8, 9}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.addi", {11}, {10, 7}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.divsi", {12}, {11, 9}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.subi", {13}, {2, 12}, {"index"},
                          {"index", "index"});
    appendAffineOperation("arith.maxsi", {14}, {13, 1}, {"index"},
                          {"index", "index"});
    affineState = cvub::RunConvertArithToAffine(affineModule);
    if (affineState.replacementOperands.at(14) !=
        std::vector<int>({2, 9, 7, 8}))
      throw std::runtime_error(
          "nested affine.apply composition changed canonical operand order");

    cvub::GenericModule divisibleModule;
    auto appendDivisibleOperation =
        [&](std::string name, std::vector<int> results,
            std::vector<int> operands, std::vector<std::string> resultTypes,
            std::vector<std::string> operandTypes,
            std::string properties = {}) {
          cvub::GenericOperation operation;
          operation.id = static_cast<int>(divisibleModule.operations.size());
          operation.name = std::move(name);
          operation.results = std::move(results);
          operation.operands = std::move(operands);
          operation.resultTypes = std::move(resultTypes);
          operation.operandTypes = std::move(operandTypes);
          operation.properties = std::move(properties);
          divisibleModule.operations.push_back(std::move(operation));
        };
    appendDivisibleOperation("arith.constant", {30}, {}, {"index"}, {},
                             "{value = 64 : index}");
    appendDivisibleOperation("arith.constant", {31}, {}, {"index"}, {},
                             "{value = 16 : index}");
    appendDivisibleOperation("arith.constant", {32}, {}, {"index"}, {},
                             "{value = 0 : index}");
    appendDivisibleOperation("arith.index_cast", {33}, {24}, {"index"},
                             {"i32"});
    appendDivisibleOperation("arith.muli", {34}, {33, 30}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.remsi", {35}, {34, 31}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.subi", {36}, {31, 35}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.maxsi", {37}, {36, 32}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.minsi", {38}, {37, 31}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.minsi", {39}, {38, 32}, {"index"},
                             {"index", "index"});
    appendDivisibleOperation("arith.subi", {40}, {38, 39}, {"index"},
                             {"index", "index"});
    const cvub::ConvertArithToAffineState divisibleState =
        cvub::RunConvertArithToAffine(divisibleModule);
    const std::map<int, int64_t> expectedFolded = {
        {5, 0}, {6, 16}, {7, 16}, {8, 16}, {9, 0}, {10, 16}};
    if (divisibleState.foldedConstants != expectedFolded)
      throw std::runtime_error(
          "affine known-divisor constant folding differs from MLIR");

    const std::string scaledValue = cvub::MakeAffineBinaryExpression(
        "mul", "c(128)", "v(24)");
    const std::string scaledValueWithRemainder =
        cvub::MakeAffineBinaryExpression("add", scaledValue, "c(16)");
    if (cvub::MakeAffineBinaryExpression(
            "floordiv", scaledValueWithRemainder, "c(128)") != "v(24)")
      throw std::runtime_error(
          "affine floor-div did not distribute over a divisible add term");

    const cvub::fs::path canonicalizationPath =
        cvub::fs::temp_directory_path() / "cvub_tensor_empty_cse.mlir";
    std::ofstream canonicalizationOut(canonicalizationPath);
    canonicalizationOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = () -> (), sym_name = "
           "\"empty_cse\"}> ({\n"
           "    %0 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "    %1 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "    \"scf.for\"() ({\n"
           "      %2 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "      \"test.consume\"(%2) : (tensor<4xi32>) -> ()\n"
           "      \"scf.yield\"() : () -> ()\n"
           "    }) : () -> ()\n"
           "    \"test.consume\"(%1) : (tensor<4xi32>) -> ()\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    canonicalizationOut.close();
    cvub::GenericModule canonicalized = cvub::RunCanonicalizationHIVMPipeline(
        cvub::ParseGenericIR(canonicalizationPath, false));
    const size_t survivingEmpties = static_cast<size_t>(std::count_if(
        canonicalized.operations.begin(), canonicalized.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "tensor.empty";
        }));
    if (survivingEmpties != 1)
      throw std::runtime_error(
          "canonicalizationHIVMPipeline did not CSE dominating tensor.empty");

    cvub::GenericModule pureCse = cvub::ParseGenericIR(
        canonicalizationPath, false);
    cvub::GenericOperation &secondEmpty = pureCse.operations.at(3);
    secondEmpty.name = "hivm.hir.get_sub_block_idx";
    secondEmpty.resultTypes = {"i64"};
    cvub::GenericOperation &firstEmpty = pureCse.operations.at(2);
    firstEmpty.name = "hivm.hir.get_sub_block_idx";
    firstEmpty.resultTypes = {"i64"};
    cvub::ApplyOperationSemantics(firstEmpty);
    cvub::ApplyOperationSemantics(secondEmpty);
    pureCse = cvub::RunCanonicalizationHIVMPipeline(std::move(pureCse));
    const size_t survivingSubBlockIndices = static_cast<size_t>(std::count_if(
        pureCse.operations.begin(), pureCse.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "hivm.hir.get_sub_block_idx";
        }));
    if (survivingSubBlockIndices != 1)
      throw std::runtime_error(
          "canonicalizationHIVMPipeline did not CSE a pure operation");

    cvub::GenericModule reorderedCse = cvub::ParseGenericIR(
        canonicalizationPath, false);
    cvub::GenericOperation &firstConstant = reorderedCse.operations.at(2);
    cvub::GenericOperation &secondConstant = reorderedCse.operations.at(3);
    firstConstant.name = "arith.constant";
    firstConstant.resultTypes = {"index"};
    firstConstant.properties = "{value = 0 : index}";
    firstConstant.attributes = "{value = 0 : index}";
    firstConstant.effects = "none";
    secondConstant = firstConstant;
    secondConstant.id = 3;
    secondConstant.results = {1};
    secondConstant.ordinal = 1;
    std::swap(reorderedCse.blocks.at(1).operations.at(0),
              reorderedCse.blocks.at(1).operations.at(1));
    reorderedCse.operations.at(2).ordinal = 1;
    reorderedCse.operations.at(3).ordinal = 0;
    reorderedCse =
        cvub::RunCanonicalizationHIVMPipeline(std::move(reorderedCse));
    const size_t survivingZeroConstants = static_cast<size_t>(std::count_if(
        reorderedCse.operations.begin(), reorderedCse.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "arith.constant" &&
                 operation.properties == "{value = 0 : index}";
        }));
    if (survivingZeroConstants != 1)
      throw std::runtime_error(
          "canonicalization CSE did not follow block order for pure constants");

    const cvub::fs::path affineAliasPath =
        cvub::fs::temp_directory_path() / "cvub_affine_cse_alias.mlir";
    std::ofstream affineAliasOut(affineAliasPath);
    affineAliasOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = (index, index) -> index, "
           "sym_name = \"affine_alias\"}> ({\n"
           "  ^bb0(%arg0: index, %arg1: index):\n"
           "    %0 = \"arith.maxsi\"(%arg0, %arg1) : "
           "(index, index) -> index\n"
           "    %1 = \"arith.maxsi\"(%arg0, %arg1) : "
           "(index, index) -> index\n"
           "    %2 = \"arith.subi\"(%arg0, %1) : "
           "(index, index) -> index\n"
           "    \"func.return\"(%2) : (index) -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    affineAliasOut.close();
    cvub::GenericModule affineAlias = cvub::RunArithToAffineConversion(
        cvub::ParseGenericIR(affineAliasPath, false));
    const auto survivingMax = std::find_if(
        affineAlias.operations.begin(), affineAlias.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "affine.max";
        });
    const auto affineConsumer = std::find_if(
        affineAlias.operations.begin(), affineAlias.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "affine.apply";
        });
    if (survivingMax == affineAlias.operations.end() ||
        affineConsumer == affineAlias.operations.end() ||
        survivingMax->results.size() != 1 ||
        std::find(affineConsumer->operands.begin(),
                  affineConsumer->operands.end(),
                  survivingMax->results.front()) == affineConsumer->operands.end())
      throw std::runtime_error(
          "ArithToAffine materialized a stale CSE result operand");

    const cvub::fs::path cloneEmptyPath =
        cvub::fs::temp_directory_path() / "cvub_clone_tensor_empty.mlir";
    std::ofstream cloneEmptyOut(cloneEmptyPath);
    cloneEmptyOut
        << "\"builtin.module\"() ({\n"
           "  \"func.func\"() <{function_type = () -> (), sym_name = "
           "\"clone_empty\"}> ({\n"
           "    %0 = \"tensor.empty\"() : () -> tensor<4xi32>\n"
           "    %1 = \"hivm.hir.vadd\"(%0, %0, %0) "
           "<{operandSegmentSizes = array<i32: 2, 1, 0>}> : "
           "(tensor<4xi32>, tensor<4xi32>, tensor<4xi32>) -> "
           "tensor<4xi32>\n"
           "    \"func.return\"() : () -> ()\n"
           "  }) : () -> ()\n"
           "}) : () -> ()\n";
    cloneEmptyOut.close();
    cvub::GenericModule clonedEmpties = cvub::RunCloneTensorEmpty(
        cvub::ParseGenericIR(cloneEmptyPath, false));
    const auto clonedVadd = std::find_if(
        clonedEmpties.operations.begin(), clonedEmpties.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "hivm.hir.vadd";
        });
    const size_t clonedEmptyCount = static_cast<size_t>(std::count_if(
        clonedEmpties.operations.begin(), clonedEmpties.operations.end(),
        [](const cvub::GenericOperation &operation) {
          return operation.name == "tensor.empty";
        }));
    if (clonedVadd == clonedEmpties.operations.end() ||
        clonedEmptyCount != 2 || clonedVadd->operands[0] != clonedVadd->operands[1] ||
        clonedVadd->operands[2] == clonedVadd->operands[0])
      throw std::runtime_error(
          "CloneTensorEmpty did not clone the HIVM DPS init exactly once");

    cvub::GenericOperation destinationStyle;
    destinationStyle.name = "tensor.insert";
    destinationStyle.operands = {0, 1};
    destinationStyle.operandTypes = {"i32", "tensor<4xi32>"};
    std::vector<cvub::GenericOperation> reviewedOperations = {
        destinationStyle};
    cvub::ApplyOperationSemanticsToAll(reviewedOperations);
    cvub::ApplyOperationSemanticsToAll(reviewedOperations);
    if (reviewedOperations.front().dpsInputs != std::vector<int>{0} ||
        reviewedOperations.front().dpsInits != std::vector<int>{1})
      throw std::runtime_error("operation semantics initialization is not idempotent");

    cvub::GenericOperation unreviewed;
    unreviewed.name = "test.unmodeled_conv1d";
    std::vector<cvub::GenericOperation> unreviewedOperations = {unreviewed};
    bool rejectedUnreviewedOperation = false;
    try {
      cvub::ApplyOperationSemanticsToAll(unreviewedOperations);
    } catch (const std::runtime_error &error) {
      rejectedUnreviewedOperation =
          std::string(error.what()).find("unreviewed operation") !=
          std::string::npos;
    }
    if (!rejectedUnreviewedOperation)
      throw std::runtime_error("unreviewed operation semantics did not fail fast");

    const std::string modularMultiple = cvub::MakeAffineBinaryExpression(
        "mul", "c(128)", "v(1)");
    const std::string modularSum = cvub::MakeAffineBinaryExpression(
        "add", modularMultiple, "v(2)");
    if (cvub::MakeAffineBinaryExpression("mod", modularSum, "c(128)") !=
        "mod(v(2),c(128))")
      throw std::runtime_error(
          "affine mod did not drop an additive multiple of the modulus");
    if (cvub::MakeAffineBinaryExpression(
            "mod", "mod(v(2),c(256))", "c(128)") !=
        "mod(v(2),c(128))")
      throw std::runtime_error(
          "affine nested mod did not fold the divisible outer modulus");

    std::cout << "SEMANTIC_IR_TESTS=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }

}
