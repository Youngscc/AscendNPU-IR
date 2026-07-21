#ifndef CVPIPELINE_UB_MODEL_CPP_DEBUG_TRACE_HPP
#define CVPIPELINE_UB_MODEL_CPP_DEBUG_TRACE_HPP

#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace cvub {

class DebugTrace {
public:
  DebugTrace(std::ostream &output,
             std::filesystem::path artifactDirectory = {},
             bool emitDebug = true, bool emitRuntimeTiming = false)
      : output_(output), artifactDirectory_(std::move(artifactDirectory)),
        emitDebug_(emitDebug), emitRuntimeTiming_(emitRuntimeTiming) {
    if (emitDebug_ && !artifactDirectory_.empty())
      std::filesystem::create_directories(artifactDirectory_);
  }

  void Pass(
      const std::string &passName,
      std::initializer_list<std::pair<std::string, uint64_t>> fields = {}) {
    if (!emitDebug_)
      return;
    output_ << "CVUB_DEBUG\tPASS\t" << passName;
    for (const auto &[name, value] : fields)
      output_ << '\t' << name << '=' << value;
    output_ << '\n';
  }

  template <typename Serializer>
  void Artifact(const std::string &passName, Serializer &&serialize) {
    if (!emitDebug_ || artifactDirectory_.empty())
      return;
    WriteArtifact(passName, serialize());
  }

  template <typename Callable>
  std::invoke_result_t<Callable &&>
  Measure(const std::string &stageName, Callable &&callable) {
    if (!emitRuntimeTiming_)
      return std::forward<Callable>(callable)();
    const auto started = Clock::now();
    using Result = std::invoke_result_t<Callable &&>;
    try {
      if constexpr (std::is_void_v<Result>) {
        std::forward<Callable>(callable)();
        RecordTiming(stageName, Clock::now() - started);
        return;
      } else {
        Result result = std::forward<Callable>(callable)();
        RecordTiming(stageName, Clock::now() - started);
        return result;
      }
    } catch (...) {
      RecordTiming(stageName, Clock::now() - started);
      throw;
    }
  }

  bool RuntimeTimingEnabled() const { return emitRuntimeTiming_; }

  template <typename Duration>
  void PrintRuntimeTiming(Duration total) {
    if (!emitRuntimeTiming_)
      return;
    output_ << "CVPIPELINE_TIMING\t1\tmodel\tTOTAL\t-\t0\t"
            << ToNanoseconds(total) << '\n';
    for (const RuntimeTimingRecord &record : runtimeTimings_)
      output_ << "CVPIPELINE_TIMING\t1\tmodel\tSTAGE\t" << record.name
              << '\t' << record.occurrence << '\t' << record.nanoseconds
              << '\n';
  }

private:
  using Clock = std::chrono::steady_clock;

  struct RuntimeTimingRecord {
    std::string name;
    size_t occurrence = 0;
    uint64_t nanoseconds = 0;
  };

  template <typename Duration> static uint64_t ToNanoseconds(Duration value) {
    const auto nanoseconds =
        std::chrono::duration_cast<std::chrono::nanoseconds>(value).count();
    return nanoseconds <= 0 ? 0 : static_cast<uint64_t>(nanoseconds);
  }

  template <typename Duration>
  void RecordTiming(const std::string &stageName, Duration duration) {
    runtimeTimings_.push_back(
        {stageName, ++timingOccurrences_[stageName], ToNanoseconds(duration)});
  }

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
  bool emitDebug_ = true;
  bool emitRuntimeTiming_ = false;
  std::vector<RuntimeTimingRecord> runtimeTimings_;
  std::map<std::string, size_t> timingOccurrences_;
  size_t sequence_ = 0;
};

template <typename Callable>
std::invoke_result_t<Callable &&>
MeasureStage(DebugTrace *trace, const std::string &stageName,
             Callable &&callable) {
  if (trace)
    return trace->Measure(stageName, std::forward<Callable>(callable));
  return std::forward<Callable>(callable)();
}

} // namespace cvub

#endif
