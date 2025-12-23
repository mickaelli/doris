#include <gtest/gtest.h>

#include "common/be_mock_util.h"
#include "vec/aggregate_functions/aggregate_function.h"
#include "vec/aggregate_functions/aggregate_function_simple_factory.h"
#include "vec/columns/column_vector.h"
#include "vec/data_types/data_type_number.h"

namespace doris::vectorized {

class AggSkewKurtTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}

    void test_agg_function(const std::string& name, const ColumnVector<Float64>::Container& data,
                           Float64 expected_result) {
        AggregateFunctionSimpleFactory factory;
        DataTypes data_types = {std::make_shared<DataTypeFloat64>()};
        auto agg_function = factory.get(name, data_types, false, -1);
        ASSERT_TRUE(agg_function != nullptr);

        std::unique_ptr<char[]> place(new char[agg_function->size_of_data()]);
        agg_function->create(place.get());

        auto column = ColumnFloat64::create();
        for (auto val : data) {
            column->insert_value(val);
        }

        const IColumn* columns[] = {column.get()};
        for (size_t i = 0; i < data.size(); ++i) {
            agg_function->add(place.get(), columns, i, nullptr);
        }

        auto result_column = ColumnFloat64::create();
        agg_function->insert_result_into(place.get(), *result_column);

        Float64 result = result_column->get_element(0);
        
        // Check for NaN if expected is NaN, otherwise check for near equality
        if (std::isnan(expected_result)) {
             EXPECT_TRUE(std::isnan(result)) << "Expected NaN for " << name;
        } else {
             EXPECT_NEAR(result, expected_result, 1e-6) << "Failed for " << name;
        }
        
        agg_function->destroy(place.get());
    }

    void test_merge(const std::string& name, const ColumnVector<Float64>::Container& data1,
                    const ColumnVector<Float64>::Container& data2, Float64 expected_result) {
        AggregateFunctionSimpleFactory factory;
        DataTypes data_types = {std::make_shared<DataTypeFloat64>()};
        auto agg_function = factory.get(name, data_types, false, -1);
        ASSERT_TRUE(agg_function != nullptr);

        std::unique_ptr<char[]> place1(new char[agg_function->size_of_data()]);
        std::unique_ptr<char[]> place2(new char[agg_function->size_of_data()]);
        agg_function->create(place1.get());
        agg_function->create(place2.get());

        auto column1 = ColumnFloat64::create();
        for (auto val : data1) {
            column1->insert_value(val);
        }
        const IColumn* columns1[] = {column1.get()};
        for (size_t i = 0; i < data1.size(); ++i) {
            agg_function->add(place1.get(), columns1, i, nullptr);
        }

        auto column2 = ColumnFloat64::create();
        for (auto val : data2) {
            column2->insert_value(val);
        }
        const IColumn* columns2[] = {column2.get()};
        for (size_t i = 0; i < data2.size(); ++i) {
            agg_function->add(place2.get(), columns2, i, nullptr);
        }

        agg_function->merge(place1.get(), place2.get(), nullptr);

        auto result_column = ColumnFloat64::create();
        agg_function->insert_result_into(place1.get(), *result_column);

        Float64 result = result_column->get_element(0);
        
        if (std::isnan(expected_result)) {
             EXPECT_TRUE(std::isnan(result)) << "Expected NaN for merge " << name;
        } else {
             EXPECT_NEAR(result, expected_result, 1e-6) << "Failed for merge " << name;
        }

        agg_function->destroy(place1.get());
        agg_function->destroy(place2.get());
    }
};

TEST_F(AggSkewKurtTest, SkewPop) {
    test_agg_function("skew_pop", {1, 2, 3, 4, 5}, 0.0);
    test_agg_function("skew_pop", {1, 1, 1, 5}, 1.1547005);
}

TEST_F(AggSkewKurtTest, KurtPop) {
    test_agg_function("kurt_pop", {1, 2, 3, 4, 5}, -1.3);
}

TEST_F(AggSkewKurtTest, MergeSkewPop) {
    test_merge("skew_pop", {1, 2}, {3, 4, 5}, 0.0);
}

TEST_F(AggSkewKurtTest, MergeKurtPop) {
    test_merge("kurt_pop", {1, 2}, {3, 4, 5}, -1.3);
}

} // namespace doris::vectorized// filepath: be/test/vec/aggregate_functions/agg_skew_kurt_test.cpp
