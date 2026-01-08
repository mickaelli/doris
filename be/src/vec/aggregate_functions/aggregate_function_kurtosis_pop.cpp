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

#include "vec/aggregate_functions/aggregate_function_skew_kurt_pop.h"
#include "vec/aggregate_functions/aggregate_function_simple_factory.h"
#include "vec/aggregate_functions/factory_helpers.h"
#include "vec/aggregate_functions/helpers.h"

namespace doris::vectorized {
#include "common/compile_check_begin.h"

template <PrimitiveType T>
using KurtPopDataPT = KurtPopData<T, KurtosisPopName>;

AggregateFunctionPtr create_aggregate_function_kurt_pop(const std::string& name,
                                                    const DataTypes& argument_types,
                                                    const DataTypePtr& result_type,
                                                    const bool result_is_nullable,
                                                    const AggregateFunctionAttr& attr) {
    assert_arity_range(name, argument_types, 1, 1);
    if (!result_is_nullable) {
        throw doris::Exception(ErrorCode::INTERNAL_ERROR,
                               "Aggregate function {} requires result_is_nullable", name);
    }

    return creator_with_type_list<TYPE_TINYINT, TYPE_SMALLINT, TYPE_INT, TYPE_BIGINT, TYPE_LARGEINT,
                                  TYPE_FLOAT, TYPE_DOUBLE>::template create<
            AggregateFunctionSkewKurt, KurtPopDataPT>(argument_types, result_is_nullable, attr);
}

void register_aggregate_function_kurtosis_pop(AggregateFunctionSimpleFactory& factory) {
    factory.register_function_both("kurt_pop", create_aggregate_function_kurt_pop);
    factory.register_alias("kurt_pop", "kurtosis_pop");
}

#include "common/compile_check_end.h"
} // namespace doris::vectorized