#include "../src/semantic_ir/semantic_ir.hpp"
#include "../src/semantic_ir/generic_ir.hpp"
#include "../src/suffix/bufferized_semantic_ir.hpp"

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
        {"memref<4xi32>", "64 : i64", 0, "result:0:0", {}});
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
    std::cout << "SEMANTIC_IR_TESTS=PASS\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
    return 1;
  }
}
