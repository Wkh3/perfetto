/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tools/trace_to_text/symbolize_profile.h"

#include <vector>

#include "perfetto/base/logging.h"
#include "perfetto/profiling/symbolizer.h"

#if PERFETTO_BUILDFLAG(PERFETTO_LOCAL_SYMBOLIZER)
#include "tools/trace_to_text/local_symbolizer.h"
#endif

#include "protos/perfetto/trace/trace.pbzero.h"
#include "protos/perfetto/trace/trace_packet.pbzero.h"
#include "tools/trace_to_text/utils.h"

namespace perfetto {
namespace trace_to_text {
namespace {

using ::protozero::proto_utils::kMessageLengthFieldSize;
using ::protozero::proto_utils::MakeTagLengthDelimited;
using ::protozero::proto_utils::WriteVarInt;

void WriteTracePacket(const std::string& str, std::ostream* output) {
  constexpr char kPreamble =
      MakeTagLengthDelimited(protos::pbzero::Trace::kPacketFieldNumber);
  uint8_t length_field[10];
  uint8_t* end = WriteVarInt(str.size(), length_field);
  *output << kPreamble;
  *output << std::string(length_field, end);
  *output << str;
}
}

// Ingest profile, and emit a symbolization table for each sequence. This can
// be prepended to the profile to attach the symbol information.
int SymbolizeProfile(std::istream* input, std::ostream* output) {
  std::unique_ptr<Symbolizer> symbolizer;
  auto binary_path = GetPerfettoBinaryPath();
  if (!binary_path.empty()) {
#if PERFETTO_BUILDFLAG(PERFETTO_LOCAL_SYMBOLIZER)
    symbolizer.reset(new LocalSymbolizer(GetPerfettoBinaryPath()));
#else
    PERFETTO_FATAL("This build does not support local symbolization.");
#endif
  }

  if (!symbolizer)
    PERFETTO_FATAL("No symbolizer selected");
  trace_processor::Config config;
  std::unique_ptr<trace_processor::TraceProcessor> tp =
      trace_processor::TraceProcessor::CreateInstance(config);

  if (!ReadTrace(tp.get(), input))
    PERFETTO_FATAL("Failed to read trace.");

  tp->NotifyEndOfFile();

  SymbolizeDatabase(tp.get(), symbolizer.get(),
                    [output](const perfetto::protos::TracePacket& packet) {
                      WriteTracePacket(packet.SerializeAsString(), output);
                    });
  return 0;
}

}  // namespace trace_to_text
}  // namespace perfetto
