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

#ifndef SRC_TRACE_PROCESSOR_WINDOW_OPERATOR_TABLE_H_
#define SRC_TRACE_PROCESSOR_WINDOW_OPERATOR_TABLE_H_

#include <limits>
#include <memory>

#include "src/trace_processor/sqlite/sqlite_table.h"

namespace perfetto {
namespace trace_processor {

class TraceStorage;

class WindowOperatorTable : public SqliteTable {
 public:
  enum Column {
    kRowId = 0,
    kQuantum = 1,
    kWindowStart = 2,
    kWindowDur = 3,
    kTs = 4,
    kDuration = 5,
    kQuantumTs = 6
  };
  class Cursor : public SqliteTable::Cursor {
   public:
    Cursor(WindowOperatorTable*);

    // Implementation of SqliteTable::Cursor.
    int Filter(const QueryConstraints& qc,
               sqlite3_value**,
               FilterHistory) override;
    int Next() override;
    int Eof() override;
    int Column(sqlite3_context*, int N) override;

   private:
    // Defines the data to be generated by the table.
    enum FilterType {
      // Returns all the spans.
      kReturnAll = 0,
      // Only returns the first span of the table. Useful for UPDATE operations.
      kReturnFirst = 1,
    };

    int64_t window_start_ = 0;
    int64_t window_end_ = 0;
    int64_t step_size_ = 0;

    int64_t current_ts_ = 0;
    int64_t quantum_ts_ = 0;
    int64_t row_id_ = 0;

    FilterType filter_type_ = FilterType::kReturnAll;

    WindowOperatorTable* table_ = nullptr;
  };

  static void RegisterTable(sqlite3* db, const TraceStorage* storage);

  WindowOperatorTable(sqlite3*, const TraceStorage*);

  // Table implementation.
  util::Status Init(int, const char* const*, Schema* schema) override;
  std::unique_ptr<SqliteTable::Cursor> CreateCursor() override;
  int BestIndex(const QueryConstraints&, BestIndexInfo*) override;
  int ModifyConstraints(QueryConstraints* qc) override;
  int Update(int, sqlite3_value**, sqlite3_int64*) override;

 private:
  int64_t quantum_ = 0;
  int64_t window_start_ = 0;

  // max of int64_t because SQLite technically only supports int64s and not
  // uint64s.
  int64_t window_dur_ = std::numeric_limits<int64_t>::max();
};
}  // namespace trace_processor
}  // namespace perfetto

#endif  // SRC_TRACE_PROCESSOR_WINDOW_OPERATOR_TABLE_H_
