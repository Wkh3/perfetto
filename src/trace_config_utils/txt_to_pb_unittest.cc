/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "src/trace_config_utils/txt_to_pb.h"

#include <memory>
#include <string>

#include "test/gtest_and_gmock.h"

#include "protos/perfetto/config/data_source_config.gen.h"
#include "protos/perfetto/config/ftrace/ftrace_config.gen.h"
#include "protos/perfetto/config/test_config.gen.h"
#include "protos/perfetto/config/trace_config.gen.h"

namespace perfetto {
namespace {

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::StrictMock;
using TraceConfig = ::perfetto::protos::gen::TraceConfig;

TraceConfig ToProto(const std::string& input) {
  base::StatusOr<std::vector<uint8_t>> output = TraceConfigTxtToPb(input);
  EXPECT_TRUE(output.ok());
  EXPECT_FALSE(output->empty());
  TraceConfig config;
  config.ParseFromArray(output->data(), output->size());
  return config;
}

TEST(TxtToPbTest, OneField) {
  TraceConfig config = ToProto(R"(
    duration_ms: 1234
  )");
  EXPECT_EQ(config.duration_ms(), 1234u);
}

TEST(TxtToPbTest, TwoFields) {
  TraceConfig config = ToProto(R"(
    duration_ms: 1234
    file_write_period_ms: 5678
  )");
  EXPECT_EQ(config.duration_ms(), 1234u);
  EXPECT_EQ(config.file_write_period_ms(), 5678u);
}

TEST(TxtToPbTest, Enum) {
  TraceConfig config = ToProto(R"(
compression_type: COMPRESSION_TYPE_DEFLATE
)");
  EXPECT_EQ(config.compression_type(), 1);
}

TEST(TxtToPbTest, LastCharacters) {
  EXPECT_EQ(ToProto(R"(
duration_ms: 123;)")
                .duration_ms(),
            123u);
  EXPECT_EQ(ToProto(R"(
  duration_ms: 123
)")
                .duration_ms(),
            123u);
  EXPECT_EQ(ToProto(R"(
  duration_ms: 123#)")
                .duration_ms(),
            123u);
  EXPECT_EQ(ToProto(R"(
  duration_ms: 123 )")
                .duration_ms(),
            123u);

  EXPECT_EQ(ToProto(R"(
compression_type: COMPRESSION_TYPE_DEFLATE;)")
                .compression_type(),
            1);
  EXPECT_EQ(ToProto(R"(
compression_type: COMPRESSION_TYPE_DEFLATE
)")
                .compression_type(),
            1);
  EXPECT_EQ(ToProto(R"(
  compression_type: COMPRESSION_TYPE_DEFLATE#)")
                .compression_type(),
            1);
  EXPECT_EQ(ToProto(R"(
  compression_type: COMPRESSION_TYPE_DEFLATE )")
                .compression_type(),
            1);
}

TEST(TxtToPbTest, Semicolons) {
  TraceConfig config = ToProto(R"(
    duration_ms: 1234;
    file_write_period_ms: 5678;
  )");
  EXPECT_EQ(config.duration_ms(), 1234u);
  EXPECT_EQ(config.file_write_period_ms(), 5678u);
}

TEST(TxtToPbTest, NestedMessage) {
  TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 123
    }
  )");
  ASSERT_EQ(config.buffers().size(), 1u);
  EXPECT_EQ(config.buffers()[0].size_kb(), 123u);
}

TEST(TxtToPbTest, SplitNested) {
  TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 1
    }
    duration_ms: 1000;
    buffers: {
      size_kb: 2
    }
  )");
  ASSERT_EQ(config.buffers().size(), 2u);
  EXPECT_EQ(config.buffers()[0].size_kb(), 1u);
  EXPECT_EQ(config.buffers()[1].size_kb(), 2u);
  EXPECT_EQ(config.duration_ms(), 1000u);
}

TEST(TxtToPbTest, MultipleNestedMessage) {
  TraceConfig config = ToProto(R"(
    buffers: {
      size_kb: 1
    }
    buffers: {
      size_kb: 2
    }
  )");
  ASSERT_EQ(config.buffers().size(), 2u);
  EXPECT_EQ(config.buffers()[0].size_kb(), 1u);
  EXPECT_EQ(config.buffers()[1].size_kb(), 2u);
}

TEST(TxtToPbTest, NestedMessageCrossFile) {
  TraceConfig config = ToProto(R"(
data_sources {
  config {
    ftrace_config {
      drain_period_ms: 42
    }
  }
}
  )");
  protos::gen::FtraceConfig ftrace_config;
  ASSERT_TRUE(ftrace_config.ParseFromString(
      config.data_sources()[0].config().ftrace_config_raw()));
  ASSERT_EQ(ftrace_config.drain_period_ms(), 42u);
}

TEST(TxtToPbTest, Booleans) {
  TraceConfig config = ToProto(R"(
    write_into_file: false; deferred_start: true;
  )");
  EXPECT_EQ(config.write_into_file(), false);
  EXPECT_EQ(config.deferred_start(), true);
}

TEST(TxtToPbTest, Comments) {
  TraceConfig config = ToProto(R"(
    write_into_file: false # deferred_start: true;
    buffers# 1
    # 2
    :# 3
    # 4
    {# 5
    # 6
    fill_policy# 7
    # 8
    :# 9
    # 10
    RING_BUFFER# 11
    # 12
    ;# 13
    # 14
    } # 15
    # 16
  )");
  EXPECT_EQ(config.write_into_file(), false);
  EXPECT_EQ(config.deferred_start(), false);
}

TEST(TxtToPbTest, Enums) {
  TraceConfig config = ToProto(R"(
    buffers: {
      fill_policy: RING_BUFFER
    }
  )");
  const auto kRingBuffer = TraceConfig::BufferConfig::RING_BUFFER;
  EXPECT_EQ(config.buffers()[0].fill_policy(), kRingBuffer);
}

TEST(TxtToPbTest, AllFieldTypes) {
  TraceConfig config = ToProto(R"(
data_sources {
  config {
    for_testing {
      dummy_fields {
        field_uint32: 1;
        field_uint64: 2;
        field_int32: 3;
        field_int64: 4;
        field_fixed64: 5;
        field_sfixed64: 6;
        field_fixed32: 7;
        field_sfixed32: 8;
        field_double: 9.9;
        field_float: 10.10;
        field_sint64: 11;
        field_sint32: 12;
        field_string: "13";
        field_bytes: "14";
      }
    }
  }
}
  )");
  const auto& fields =
      config.data_sources()[0].config().for_testing().dummy_fields();
  ASSERT_EQ(fields.field_uint32(), 1u);
  ASSERT_EQ(fields.field_uint64(), 2u);
  ASSERT_EQ(fields.field_int32(), 3);
  ASSERT_EQ(fields.field_int64(), 4);
  ASSERT_EQ(fields.field_fixed64(), 5u);
  ASSERT_EQ(fields.field_sfixed64(), 6);
  ASSERT_EQ(fields.field_fixed32(), 7u);
  ASSERT_EQ(fields.field_sfixed32(), 8);
  ASSERT_DOUBLE_EQ(fields.field_double(), 9.9);
  ASSERT_FLOAT_EQ(fields.field_float(), 10.10f);
  ASSERT_EQ(fields.field_sint64(), 11);
  ASSERT_EQ(fields.field_sint32(), 12);
  ASSERT_EQ(fields.field_string(), "13");
  ASSERT_EQ(fields.field_bytes(), "14");
}

TEST(TxtToPbTest, LeadingDots) {
  TraceConfig config = ToProto(R"(
data_sources {
  config {
    for_testing {
      dummy_fields {
        field_double:  .1;
        field_float:   .2;
      }
    }
  }
}
  )");
  const auto& fields =
      config.data_sources()[0].config().for_testing().dummy_fields();
  ASSERT_DOUBLE_EQ(fields.field_double(), .1);
  ASSERT_FLOAT_EQ(fields.field_float(), .2f);
}

TEST(TxtToPbTest, NegativeNumbers) {
  TraceConfig config = ToProto(R"(
data_sources {
  config {
    for_testing {
      dummy_fields {
        field_int32: -1;
        field_int64: -2;
        field_fixed64: -3;
        field_sfixed64: -4;
        field_fixed32: -5;
        field_sfixed32: -6;
        field_double: -7.7;
        field_float: -8.8;
        field_sint64: -9;
        field_sint32: -10;
      }
    }
  }
}
  )");
  const auto& fields =
      config.data_sources()[0].config().for_testing().dummy_fields();
  ASSERT_EQ(fields.field_int32(), -1);
  ASSERT_EQ(fields.field_int64(), -2);
  ASSERT_EQ(fields.field_fixed64(), static_cast<uint64_t>(-3));
  ASSERT_EQ(fields.field_sfixed64(), -4);
  ASSERT_EQ(fields.field_fixed32(), static_cast<uint32_t>(-5));
  ASSERT_EQ(fields.field_sfixed32(), -6);
  ASSERT_DOUBLE_EQ(fields.field_double(), -7.7);
  ASSERT_FLOAT_EQ(fields.field_float(), -8.8f);
  ASSERT_EQ(fields.field_sint64(), -9);
  ASSERT_EQ(fields.field_sint32(), -10);
}

TEST(TxtToPbTest, EofEndsNumeric) {
  TraceConfig config = ToProto(R"(duration_ms: 1234)");
  EXPECT_EQ(config.duration_ms(), 1234u);
}

TEST(TxtToPbTest, EofEndsIdentifier) {
  TraceConfig config = ToProto(R"(enable_extra_guardrails: true)");
  EXPECT_EQ(config.enable_extra_guardrails(), true);
}

TEST(TxtToPbTest, ExampleConfig) {
  TraceConfig config = ToProto(R"(
buffers {
  size_kb: 100024
  fill_policy: RING_BUFFER
}

data_sources {
  config {
    name: "linux.ftrace"
    target_buffer: 0
    ftrace_config {
      buffer_size_kb: 512 # 4 (page size) * 128
      drain_period_ms: 200
      ftrace_events: "binder_lock"
      ftrace_events: "binder_locked"
      atrace_categories: "gfx"
    }
  }
}

data_sources {
  config {
    name: "linux.process_stats"
    target_buffer: 0
  }
}

data_sources {
  config {
    name: "linux.inode_file_map"
    target_buffer: 0
    inode_file_config {
      scan_delay_ms: 1000
      scan_interval_ms: 1000
      scan_batch_size: 500
      mount_point_mapping: {
        mountpoint: "/data"
        scan_roots: "/data/app"
      }
    }
  }
}

producers {
  producer_name: "perfetto.traced_probes"
  shm_size_kb: 4096
  page_size_kb: 4
}

duration_ms: 10000
)");
  EXPECT_EQ(config.duration_ms(), 10000u);
  EXPECT_EQ(config.buffers()[0].size_kb(), 100024u);
  EXPECT_EQ(config.data_sources()[0].config().name(), "linux.ftrace");
  EXPECT_EQ(config.data_sources()[0].config().target_buffer(), 0u);
  EXPECT_EQ(config.producers()[0].producer_name(), "perfetto.traced_probes");
}

TEST(TxtToPbTest, Strings) {
  TraceConfig config = ToProto(R"(
data_sources {
  config {
    ftrace_config {
      ftrace_events: "binder_lock"
      ftrace_events: "foo/bar"
      ftrace_events: "foo\\bar"
      ftrace_events: "newline\nnewline"
      ftrace_events: "\"quoted\""
      ftrace_events: "\a\b\f\n\r\t\v\\\'\"\?"
      ftrace_events: "\0127_\03422.\177"
    }
  }
}
)");
  protos::gen::FtraceConfig ftrace_config;
  ASSERT_TRUE(ftrace_config.ParseFromString(
      config.data_sources()[0].config().ftrace_config_raw()));
  const auto& events = ftrace_config.ftrace_events();
  EXPECT_THAT(events, Contains("binder_lock"));
  EXPECT_THAT(events, Contains("foo/bar"));
  EXPECT_THAT(events, Contains("foo\\bar"));
  EXPECT_THAT(events, Contains("newline\nnewline"));
  EXPECT_THAT(events, Contains("\"quoted\""));
  EXPECT_THAT(events, Contains("\a\b\f\n\r\t\v\\\'\"\?"));
  EXPECT_THAT(events, Contains("\0127_\03422.\177"));
}

TEST(TxtToPbTest, UnknownField) {
  auto res = TraceConfigTxtToPb(R"(
    not_a_label: false
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(),
              HasSubstr("No field named \"not_a_label\" in proto TraceConfig"));
}

TEST(TxtToPbTest, UnknownNestedField) {
  auto res = TraceConfigTxtToPb(R"(
data_sources {
  config {
    not_a_field_name {
    }
  }
}
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(
      res.status().message(),
      HasSubstr(
          "No field named \"not_a_field_name\" in proto DataSourceConfig"));
}

TEST(TxtToPbTest, BadBoolean) {
  auto res = TraceConfigTxtToPb(R"(
    write_into_file: foo;
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(),
              HasSubstr("Expected 'true' or 'false' for boolean field "
                        "write_into_file in proto TraceConfig instead "
                        "saw 'foo'"));
}

TEST(TxtToPbTest, MissingBoolean) {
  auto res = TraceConfigTxtToPb(R"(
    write_into_file:
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(), HasSubstr("Unexpected end of input"));
}

TEST(TxtToPbTest, RootProtoMustNotEndWithBrace) {
  auto res = TraceConfigTxtToPb("  }");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(), HasSubstr("Unmatched closing brace"));
}

TEST(TxtToPbTest, SawNonRepeatedFieldTwice) {
  auto res = TraceConfigTxtToPb(R"(
    write_into_file: true;
    write_into_file: true;
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(
      res.status().message(),
      HasSubstr("Saw non-repeating field 'write_into_file' more than once"));
}

TEST(TxtToPbTest, WrongTypeBoolean) {
  auto res = TraceConfigTxtToPb(R"(
    duration_ms: true;
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(
      res.status().message(),
      HasSubstr("Expected value of type uint32 for field duration_ms in "
                "proto TraceConfig instead saw 'true'"));
}

TEST(TxtToPbTest, WrongTypeNumber) {
  auto res = TraceConfigTxtToPb(R"(
    buffers: 100;
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(),
              HasSubstr("Expected value of type message for field buffers in "
                        "proto TraceConfig instead saw '100'"));
}

TEST(TxtToPbTest, NestedMessageDidNotTerminate) {
  auto res = TraceConfigTxtToPb(R"(
    buffers: {
  )");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(), HasSubstr("Nested message not closed"));
}

TEST(TxtToPbTest, BadEscape) {
  auto res = TraceConfigTxtToPb(R"(
  data_sources {
    config {
      ftrace_config {
        ftrace_events: "\p"
      }
    }
  })");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(),
              HasSubstr("Unknown string escape in ftrace_events in "
                        "proto FtraceConfig: '\\p'"));
}

TEST(TxtToPbTest, BadEnumValue) {
  auto res = TraceConfigTxtToPb("compression_type: FOO");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(),
              HasSubstr("Unexpected value 'FOO' for enum field "
                        "compression_type in proto TraceConfig"));
}

TEST(TxtToPbTest, UnexpectedBracket) {
  auto res = TraceConfigTxtToPb("{");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(), HasSubstr("Unexpected character '{'"));
}

TEST(TxtToPbTest, UnknownNested) {
  auto res = TraceConfigTxtToPb("foo {}; bar: 42");
  EXPECT_FALSE(res.ok());
  EXPECT_THAT(res.status().message(), HasSubstr("No field named \"foo\" in "
                                                "proto TraceConfig"));
}

// TODO(hjd): Add these tests.
// TEST(TxtToPbTest, WrongTypeString)
// TEST(TxtToPbTest, OverflowOnIntegers)
// TEST(TxtToPbTest, NegativeNumbersForUnsignedInt)
// TEST(TxtToPbTest, UnterminatedString) {
// TEST(TxtToPbTest, NumberIsEof)
// TEST(TxtToPbTest, OneOf)

}  // namespace
}  // namespace perfetto
