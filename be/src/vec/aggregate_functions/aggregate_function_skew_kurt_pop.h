// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>

#include "vec/aggregate_functions/aggregate_function.h"
#include "vec/columns/column.h"
#include "vec/columns/column_nullable.h"
#include "vec/common/assert_cast.h"
#include "vec/core/types.h"
#include "vec/data_types/data_type_decimal.h"
#include "vec/data_types/data_type_number.h"

namespace doris::vectorized {
#include "common/compile_check_begin.h"
class Arena;
class BufferReadable;
class BufferWritable;
template <PrimitiveType T>
class ColumnDecimal;
template <PrimitiveType T>
class ColumnVector;

template <PrimitiveType T>
struct BaseData {
    BaseData() = default;
    virtual ~BaseData() = default;

    void write(BufferWritable& buf) const {
        buf.write_binary(mean);
        buf.write_binary(m2);
        buf.write_binary(m3);
        buf.write_binary(m4);
        buf.write_binary(count);
    }

    void read(BufferReadable& buf) {
        buf.read_binary(mean);
        buf.read_binary(m2);
        buf.read_binary(m3);
        buf.read_binary(m4);
        buf.read_binary(count);
    }

    void reset() {
        mean = 0.0;
        m2 = 0.0;
        m3 = 0.0;
        m4 = 0.0;
        count = 0;
    }

    static double inf_to_nan(double val) {
        if (std::isinf(val)) {
            return std::nan("");
        }
        return val;
    }

    double get_skew_pop_result() const {
        if (count == 0 || m2 <= 0) return 0.0;
        double res = (m3 * std::sqrt(double(count))) / std::pow(m2, 1.5);
        return inf_to_nan(res);
    }

    double get_kurtosis_pop_result() const {
        if (count == 0 || m2 <= 0) return 0.0;
        double res = (double)count * m4 / (m2 * m2) - 3.0;
        return inf_to_nan(res);
    }

    void merge(const BaseData& rhs) {
        if (rhs.count == 0) return;
        double n1 = (double)this->count;
        double n2 = (double)rhs.count;
        double n = n1 + n2;
        double inv_n = 1.0 / n;
        double inv_n2 = inv_n * inv_n;
        double term1 = n1 * n2 * inv_n;

        double delta = rhs.mean - this->mean;
        double delta2 = delta * delta;
        double delta3 = delta * delta2;
        double delta4 = delta2 * delta2;

        m4 = this->m4 + rhs.m4 + delta4 * term1 * (n1 * n1 - n1 * n2 + n2 * n2) * inv_n2;
        m4 += 6.0 * delta2 * (n1 * n1 * rhs.m2 + n2 * n2 * this->m2) * inv_n2 +
              4.0 * delta * (n1 * rhs.m3 - n2 * this->m3) * inv_n;

        m3 = this->m3 + rhs.m3 + delta3 * term1 * (n1 - n2) * inv_n;
        m3 += 3.0 * delta * (n1 * rhs.m2 - n2 * this->m2) * inv_n;

        m2 = rhs.m2 + m2 + (delta * delta) * term1;
        this->mean += delta * n2 * inv_n;
        count = int64_t(n);
    }

    void add(const IColumn* column, size_t row_num) {
        const auto& sources = assert_cast<const typename PrimitiveTypeTraits<T>::ColumnType&,
                                          TypeCheckOnRelease::DISABLE>(*column);
        double val = (double)sources.get_data()[row_num];

        long long n1 = count;
        count++;
        double delta = val - mean;
        double delta_n = delta / double(count);
        double delta_n2 = delta_n * delta_n;
        double term1 = delta * delta_n * double(n1);

        mean += delta_n;
        m4 += term1 * delta_n2 * (double(count) * double(count) - 3 * double(count) + 3) + 6 * delta_n2 * m2 - 4 * delta_n * m3;
        m3 += term1 * delta_n * (double(count) - 2) - 3 * delta_n * m2;
        m2 += term1;
    }

    double mean {};
    double m2 {};
    double m3 {};
    double m4 {};
    int64_t count {};
};

struct SkewPopName {
    static const char* name() { return "skew_pop"; }
};

struct KurtosisPopName {
    static const char* name() { return "kurtosis_pop"; }
};

template <PrimitiveType T, typename Name>
struct SkewPopData : BaseData<T>, Name {
    using ColVecResult = std::conditional_t<is_decimal(T), ColumnDecimal128V2, ColumnFloat64>;
    void insert_result_into(IColumn& to) const {
        auto& col = assert_cast<ColVecResult&>(to);
        if constexpr (is_decimal(T)) {
            col.get_data().push_back(this->get_skew_pop_result().value());
        } else {
            col.get_data().push_back(this->get_skew_pop_result());
        }
    }
    static DataTypePtr get_return_type() { return std::make_shared<DataTypeFloat64>(); }
};

template <PrimitiveType T, typename Name>
struct KurtPopData : BaseData<T>, Name {
    using ColVecResult = std::conditional_t<is_decimal(T), ColumnDecimal128V2, ColumnFloat64>;
    void insert_result_into(IColumn& to) const {
        auto& col = assert_cast<ColVecResult&>(to);
        if constexpr (is_decimal(T)) {
            col.get_data().push_back(this->get_kurtosis_pop_result().value());
        } else {
            col.get_data().push_back(this->get_kurtosis_pop_result());
        }
    }
    static DataTypePtr get_return_type() { return std::make_shared<DataTypeFloat64>(); }
};

template <typename Data>
class AggregateFunctionSkewKurt
        : public IAggregateFunctionDataHelper<Data, AggregateFunctionSkewKurt<Data>>,
          public UnaryExpression,
          public NullableAggregateFunction {
public:
    AggregateFunctionSkewKurt(const DataTypes& argument_types_)
            : IAggregateFunctionDataHelper<Data, AggregateFunctionSkewKurt<Data>>(
                      argument_types_) {}

    String get_name() const override { return Data::name(); }

    DataTypePtr get_return_type() const override { return Data::get_return_type(); }

    void add(AggregateDataPtr __restrict place, const IColumn** columns, ssize_t row_num,
             Arena&) const override {
        this->data(place).add(columns[0], row_num);
    }

    void reset(AggregateDataPtr __restrict place) const override { this->data(place).reset(); }

    void merge(AggregateDataPtr __restrict place, ConstAggregateDataPtr rhs,
               Arena&) const override {
        this->data(place).merge(this->data(rhs));
    }

    void serialize(ConstAggregateDataPtr __restrict place, BufferWritable& buf) const override {
        this->data(place).write(buf);
    }

    void deserialize(AggregateDataPtr __restrict place, BufferReadable& buf,
                     Arena&) const override {
        this->data(place).read(buf);
    }

    void insert_result_into(ConstAggregateDataPtr __restrict place, IColumn& to) const override {
        this->data(place).insert_result_into(to);
    }
};

} // namespace doris::vectorized

#include "common/compile_check_end.h"