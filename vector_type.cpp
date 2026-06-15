/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/vector_type.h"
#include "common/value.h"
#include "common/lang/sstream.h"
#include "common/lang/string.h"
#include "common/log/log.h"

int VectorType::compare(const Value &left, const Value &right) const
{
  // Only allow VECTOR to VECTOR comparison
  if (left.attr_type() != AttrType::VECTORS || right.attr_type() != AttrType::VECTORS) {
    return INT32_MAX;
  }

  int left_len  = left.length();
  int right_len = right.length();

  if (left_len != right_len) {
    return INT32_MAX;  // different dimensions: not comparable
  }

  const float *left_data  = reinterpret_cast<const float *>(left.data());
  const float *right_data = reinterpret_cast<const float *>(right.data());
  int          count      = left_len / static_cast<int>(sizeof(float));

  for (int i = 0; i < count; i++) {
    if (left_data[i] != right_data[i]) {
      return INT32_MAX;  // different values: not equal, and no ordering defined
    }
  }

  return 0;  // equal
}

int VectorType::cast_cost(AttrType type)
{
  if (type == AttrType::VECTORS) {
    return 0;
  }
  return INT32_MAX;
}

RC VectorType::to_string(const Value &val, string &result) const
{
  const float *data  = reinterpret_cast<const float *>(val.data());
  int          count = val.length() / static_cast<int>(sizeof(float));

  stringstream ss;
  ss << "[";
  for (int i = 0; i < count; i++) {
    if (i > 0) {
      ss << ",";
    }
    ss << data[i];
  }
  ss << "]";
  result = ss.str();
  return RC::SUCCESS;
}

RC VectorType::set_value_from_str(Value &val, const string &data) const
{
  // Parse a JSON array string like "[1,2,3]"
  const char *p = data.c_str();
  while (*p == ' ')
    p++;

  if (*p != '[') {
    LOG_WARN("vector string must start with '[', got: %s", data.c_str());
    return RC::INVALID_ARGUMENT;
  }
  p++;

  vector<float> vec;
  while (*p) {
    while (*p == ' ')
      p++;
    if (*p == ']')
      break;
    if (*p == '\0')
      break;

    char *end  = nullptr;
    float val = strtof(p, &end);
    if (end == p) {
      LOG_WARN("invalid float in vector string: %s", data.c_str());
      return RC::INVALID_ARGUMENT;
    }
    vec.push_back(val);
    p = end;

    while (*p == ' ')
      p++;
    if (*p == ',')
      p++;
    else if (*p != ']') {
      LOG_WARN("expected ',' or ']' in vector string: %s", data.c_str());
      return RC::INVALID_ARGUMENT;
    }
  }

  if (*p != ']') {
    LOG_WARN("expected closing ']' in vector string: %s", data.c_str());
    return RC::INVALID_ARGUMENT;
  }

  if (vec.empty()) {
    LOG_WARN("vector must have at least one element");
    return RC::INVALID_ARGUMENT;
  }

  val.set_vector(vec.data(), static_cast<int>(vec.size()));
  return RC::SUCCESS;
}
