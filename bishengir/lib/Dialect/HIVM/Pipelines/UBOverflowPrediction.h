#ifndef BISHENGIR_LIB_DIALECT_HIVM_PIPELINES_UBOVERFLOWPREDICTION_H
#define BISHENGIR_LIB_DIALECT_HIVM_PIPELINES_UBOVERFLOWPREDICTION_H

#include "ub_overflow_model/api.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace mlir {
class Pass;

namespace hivm {

struct UBOverflowPredictionConfig {
  std::string target;
  cvub::UBRelevantCompileOptions modelOptions;
  bool pruneOnOverflow = false;
  uint64_t traceAttempt = 0;
};

std::unique_ptr<Pass>
createUBOverflowPredictionPass(UBOverflowPredictionConfig config);

} // namespace hivm
} // namespace mlir

#endif
