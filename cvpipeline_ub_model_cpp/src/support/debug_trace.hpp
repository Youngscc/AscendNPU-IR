#ifndef CVPIPELINE_UB_MODEL_CPP_DEBUG_TRACE_HPP
#define CVPIPELINE_UB_MODEL_CPP_DEBUG_TRACE_HPP

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace cvub {

class DebugTrace {
public:
  DebugTrace(std::ostream &output, std::filesystem::path artifactDirectory = {})
      : output_(output), artifactDirectory_(std::move(artifactDirectory)) {
    if (!artifactDirectory_.empty())
      std::filesystem::create_directories(artifactDirectory_);
  }

  void Pass(
      const std::string &passName,
      std::initializer_list<std::pair<std::string, uint64_t>> fields = {}) {
    output_ << "CVUB_DEBUG\tPASS\t" << passName;
    for (const auto &[name, value] : fields)
      output_ << '\t' << name << '=' << value;
    output_ << '\n';
  }

  template <typename Serializer>
  void Artifact(const std::string &passName, Serializer &&serialize) {
    if (artifactDirectory_.empty())
      return;
    WriteArtifact(passName, serialize());
  }

private:
  void WriteArtifact(const std::string &passName,
                     const std::string &contents) {
    const std::filesystem::path path =
        artifactDirectory_ /
        (PaddedSequence() + "_after_" + Sanitize(passName) + ".tsv");
    std::ofstream output(path, std::ios::binary);
    if (!output)
      throw std::runtime_error("debug artifact cannot be opened: " +
                               path.string());
    output << contents;
    if (!output)
      throw std::runtime_error("debug artifact cannot be written: " +
                               path.string());
    output_ << "CVUB_DEBUG\tARTIFACT\t" << passName << '\t' << path.string()
            << '\n';
    ++sequence_;
  }

  std::string PaddedSequence() const {
    const std::string sequence = std::to_string(sequence_);
    return std::string(sequence.size() < 3 ? 3 - sequence.size() : 0, '0') +
           sequence;
  }

  static std::string Sanitize(const std::string &name) {
    std::string result;
    result.reserve(name.size());
    for (char raw : name) {
      const unsigned char c = static_cast<unsigned char>(raw);
      result.push_back(std::isalnum(c) ? static_cast<char>(c) : '_');
    }
    return result;
  }

  std::ostream &output_;
  std::filesystem::path artifactDirectory_;
  size_t sequence_ = 0;
};

} // namespace cvub

#endif
