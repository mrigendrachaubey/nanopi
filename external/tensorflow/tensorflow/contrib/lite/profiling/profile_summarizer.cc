/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/contrib/lite/profiling/profile_summarizer.h"

#include <sstream>

#include "tensorflow/contrib/lite/schema/schema_generated.h"

namespace tflite {
namespace profiling {
namespace {

using Detail = tensorflow::StatsCalculator::Detail;

struct OperatorDetails {
  string name;
  std::vector<string> inputs;
  std::vector<string> outputs;
};

string GetTensorName(const tflite::Interpreter& interpreter, int tensor_index) {
  const auto tensor = interpreter.tensor(tensor_index);
  if (tensor == nullptr || tensor->name == nullptr) {
    return "Unknown";
  }
  return tensor->name;
}
std::vector<string> GetTensorNames(const tflite::Interpreter& interpreter,
                                   const TfLiteIntArray* tensor_indices) {
  std::vector<string> tensors;
  tensors.reserve(tensor_indices->size);
  for (int i = 0; i < tensor_indices->size; i++) {
    tensors.push_back(GetTensorName(interpreter, tensor_indices->data[i]));
  }
  return tensors;
}

string ToString(const std::vector<string>& str_vector) {
  std::stringstream stream;
  stream << "[";
  bool first = true;
  for (const auto& s : str_vector) {
    if (!first) {
      stream << ", ";
    } else {
      first = false;
    }
    stream << s;
  }
  stream << "]";
  return stream.str();
}

OperatorDetails GetOperatorDetails(const tflite::Interpreter& interpreter,
                                   int node_index) {
  auto node_reg = interpreter.node_and_registration(node_index);
  auto inputs = node_reg->first.inputs;
  auto outputs = node_reg->first.outputs;
  int code = node_reg->second.builtin_code;
  const char* op_name = nullptr;
  if (code == tflite::BuiltinOperator_CUSTOM) {
    const char* custom_name = node_reg->second.custom_name;
    op_name = custom_name ? custom_name : "UnknownCustomOp";
  } else {
    op_name = tflite::EnumNamesBuiltinOperator()[code];
  }
  OperatorDetails details;
  details.name = op_name;
  details.inputs = GetTensorNames(interpreter, inputs);
  details.outputs = GetTensorNames(interpreter, outputs);
  return details;
}

}  // namespace

ProfileSummarizer::ProfileSummarizer()
    : stats_calculator_(new ::tensorflow::StatsCalculator(
          tensorflow::StatSummarizerOptions())) {}

void ProfileSummarizer::ProcessProfiles(
    const std::vector<const ProfileEvent*>& profile_stats,
    const tflite::Interpreter& interpreter) {
  std::vector<const ProfileEvent*> events;
  std::copy_if(profile_stats.begin(), profile_stats.end(),
               std::back_inserter(events), [](const ProfileEvent* e) {
                 return e->event_type ==
                            ProfileEvent::EventType::OPERATOR_INVOKE_EVENT &&
                        e->end_timestamp_us >= e->begin_timestamp_us;
               });
  // Sort with begin_time.
  std::sort(events.begin(), events.end(),
            [](const ProfileEvent* const& a, const ProfileEvent* const& b) {
              return a->begin_timestamp_us < b->begin_timestamp_us;
            });
  if (events.empty()) {
    return;
  }

  int64_t base_start_us = events[0]->begin_timestamp_us;
  int node_num = 0;
  int64_t curr_total_us = 0;
  std::map<std::string, Detail> details;
  for (auto event : events) {
    auto op_details = GetOperatorDetails(interpreter, event->event_metadata);
    auto node_name = ToString(op_details.outputs);
    auto result = details.emplace(node_name, Detail());
    Detail* detail = &(result.first->second);
    detail->start_us.UpdateStat(event->begin_timestamp_us - base_start_us);
    int64_t node_exec_time =
        event->end_timestamp_us - event->begin_timestamp_us;
    detail->rel_end_us.UpdateStat(node_exec_time);
    curr_total_us += node_exec_time;
    ++node_num;

    if (result.second) {
      detail->name = node_name;
      detail->type = op_details.name;
      detail->run_order = node_num;
      detail->times_called = 0;
    }
    ++detail->times_called;
  }
  stats_calculator_->UpdateDetails(details);
  stats_calculator_->UpdateRunTotalUs(curr_total_us);
}
}  // namespace profiling
}  // namespace tflite
