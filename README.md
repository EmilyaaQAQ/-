# -taskpoint1
 1. src/observer/sql/parser/parse_defs.h

  修改内容：添加 VECTOR 类型常量定义

  /// VECTOR type constants
  constexpr int VECTOR_DEFAULT_DIM = 2048;
  constexpr int VECTOR_MAX_DIM     = 16383;

  ---
  2. src/observer/sql/parser/lex_sql.l

  修改内容：添加 STRING_TO_VECTOR 关键字 token

  在第 114 行后添加：
  STRING_TO_VECTOR                        RETURN_TOKEN(STRING_TO_VECTOR_T);

  ---
  3. src/observer/sql/parser/yacc_sql.y

  修改内容：

  - 新增 token 声明：STRING_TO_VECTOR_T
  - 修改 attr_def 规则：
    - ID VECTOR(N) → 校验 N ∈ [1, 16383]，length = N × sizeof(float)
    - ID VECTOR → length = 2048 × sizeof(float)（默认维度）
    - VECTOR() → YYERROR
  - 修改 value 规则：新增 STRING_TO_VECTOR_T LBRACE SSS RBRACE 分支，解析 JSON 数组字符串 '[1,2,3]' 并创建 VECTOR 类型的
  Value

  ---
  4. src/observer/common/type/vector_type.h

  修改内容：完善 VectorType 类声明，新增方法：
  - compare() — VECTOR 等值比较
  - cast_cost() — 类型转换代价
  - to_string() — 向量转字符串
  - set_value_from_str() — 从字符串解析向量

  ---
  5. src/observer/common/type/vector_type.cpp（新建）

  修改内容：实现 VectorType 全部方法：
  - compare：仅允许 VECTOR 与 VECTOR 比较。相同维度且所有元素相等返回 0，否则返回 INT32_MAX（不可比较）
  - cast_cost：仅 VECTORS→VECTORS 返回 0，其余返回 INT32_MAX
  - to_string：将向量格式化为 [1,2,3] 格式
  - set_value_from_str：解析 [1,2,3] JSON 数组字符串

  ---
  6. src/observer/common/value.h

  修改内容：新增方法声明：
  void set_vector(const float *data, int dim);

  ---
  7. src/observer/common/value.cpp

  修改内容：为 AttrType::VECTORS 添加完整的生命周期处理（类似 CHARS，使用 pointer_value_ + own_data_）：

  - set_data()：新增 VECTORS 分支，调用 set_vector()
  - set_value()：新增 VECTORS 分支
  - reset()：VECTORS 与 CHARS 共用 delete[] pointer_value_ 分支
  - 拷贝构造 / 拷贝赋值：新增 VECTORS 分支（深拷贝）
  - data()：VECTORS 返回 pointer_value_
  - set_vector()：新方法实现，分配内存并拷贝 float 数组

  ---
  8. src/observer/storage/table/table.cpp

  修改内容：在 set_value_to_record() 中添加 VECTOR 维度校验：
  - 插入向量维度必须与字段定义维度完全一致，否则返回 RC::INVALID_ARGUMENT

  ---
  9. src/observer/storage/common/condition_filter.cpp

  修改内容：在 DefaultConditionFilter::init() 中添加 VECTOR 比较限制：
  - VECTOR 类型仅支持 EQUAL_TO 和 NOT_EQUAL 比较
  - 使用 > / < / >= / <= 比较 VECTOR 会返回 RC::SCHEMA_FIELD_TYPE_MISMATCH
