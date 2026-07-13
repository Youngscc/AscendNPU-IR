#ifndef CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_ORACLE_HPP
#define CVPIPELINE_UB_MODEL_CPP_MARK_MULTI_BUFFER_ORACLE_HPP

#include "bufferized_semantic_ir_oracle.hpp"

namespace cvub {

inline unsigned MarkMultiBufferOracleInteger(const std::string &value) {
  const size_t colon = value.find(':');
  return static_cast<unsigned>(std::stoul(trim(value.substr(0, colon))));
}

inline std::string CollectMarkMultiBufferOracle(
    const GenericModule &module) {
  AfterBufferRootResolver roots(module);
  std::ostringstream output;
  output << "MARK_MULTI_BUFFER_ORACLE\t1\n";
  for (const GenericOperation &operation : module.operations) {
    if (operation.name != "annotation.mark" || operation.operands.empty())
      continue;
    std::string count =
        FindDictionaryValue(operation.attributes, "hivm.multi_buffer");
    if (count.empty())
      count = FindDictionaryValue(operation.properties, "hivm.multi_buffer");
    if (count.empty())
      continue;
    std::string preload = FindDictionaryValue(operation.attributes,
                                              "hivm.preload_local_buffer");
    if (preload.empty())
      preload = FindDictionaryValue(operation.properties,
                                    "hivm.preload_local_buffer");
    const GenericOperation *function = EnclosingFunction(module, operation);
    if (!function)
      throw std::runtime_error("MarkMultiBuffer oracle: mark outside function");
    output << "MARK\t" << HexEncode(FunctionSymName(*function)) << '\t'
           << HexEncode(roots.root(operation.operands.front())) << '\t'
           << MarkMultiBufferOracleInteger(count) << '\t'
           << (!preload.empty() && MarkMultiBufferOracleInteger(preload) != 0 ? 1 : 0)
           << '\n';
  }
  return output.str();
}

} // namespace cvub

#endif
