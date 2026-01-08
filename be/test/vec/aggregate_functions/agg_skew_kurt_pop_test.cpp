#include <gtest/gtest.h>

#include "vec/aggregate_functions/aggregate_function.h"
#include "vec/aggregate_functions/aggregate_function_simple_factory.h"
#include "vec/columns/column_vector.h"
#include "vec/data_types/data_type_number.h"
// 【新增】必须引入 Arena 头文件
#include "vec/common/arena.h" 

namespace doris::vectorized {

class AggSkewKurtTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}

    void test_agg_function(const std::string& name, const std::vector<Float64>& data,
                           Float64 expected_result) {
        auto& factory = AggregateFunctionSimpleFactory::instance();
        DataTypes data_types = {std::make_shared<DataTypeFloat64>()};
        auto result_type = std::make_shared<DataTypeFloat64>();
        
        auto agg_function = factory.get(name, data_types, result_type, true, -1);
        ASSERT_TRUE(agg_function != nullptr);

        std::unique_ptr<char[]> place(new char[agg_function->size_of_data()]);
        agg_function->create(place.get());

        auto column = ColumnFloat64::create();
        for (auto val : data) {
            column->insert_value(val);
        }

        const IColumn* columns[] = {column.get()};
        
        // 【修改】定义一个 Arena 对象
        Arena arena; 
        for (size_t i = 0; i < data.size(); ++i) {
            // 【修改】传入 arena 而不是 nullptr
            agg_function->add(place.get(), columns, i, arena);
        }

        auto result_column = ColumnFloat64::create();
        agg_function->insert_result_into(place.get(), *result_column);

        Float64 result = result_column->get_element(0);
        
        if (std::isnan(expected_result)) {
             EXPECT_TRUE(std::isnan(result)) << "Expected NaN for " << name;
        } else {
             EXPECT_NEAR(result, expected_result, 1e-6) << "Failed for " << name;
        }
        
        agg_function->destroy(place.get());
    }

    void test_merge(const std::string& name, const std::vector<Float64>& data1,
                    const std::vector<Float64>& data2, Float64 expected_result) {
        auto& factory = AggregateFunctionSimpleFactory::instance();                        DataTypes data_types = {std::make_shared<DataTypeFloat64>()};
        auto result_type = std::make_shared<DataTypeFloat64>();
        
        auto agg_function = factory.get(name, data_types, result_type, true, -1);
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
        
        // 【修改】定义 Arena
        Arena arena;
        for (size_t i = 0; i < data1.size(); ++i) {
            // 【修改】传入 arena
            agg_function->add(place1.get(), columns1, i, arena);
        }

        auto column2 = ColumnFloat64::create();
        for (auto val : data2) {
            column2->insert_value(val);
        }
        const IColumn* columns2[] = {column2.get()};
        for (size_t i = 0; i < data2.size(); ++i) {
            // 【修改】传入 arena
            agg_function->add(place2.get(), columns2, i, arena);
        }

        // 【修改】传入 arena
        agg_function->merge(place1.get(), place2.get(), arena);

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
    test_agg_function("skew_pop", {1.0, 2.0, 3.0, 4.0, 5.0}, 0.0);
    test_agg_function("skew_pop", {1.0, 1.0, 1.0, 5.0}, 1.1547005);
}

TEST_F(AggSkewKurtTest, KurtPop) {
    test_agg_function("kurt_pop", {1.0, 2.0, 3.0, 4.0, 5.0}, -1.3);
}

TEST_F(AggSkewKurtTest, MergeSkewPop) {
    test_merge("skew_pop", {1.0, 2.0}, {3.0, 4.0, 5.0}, 0.0);
}

TEST_F(AggSkewKurtTest, MergeKurtPop) {
    test_merge("kurt_pop", {1.0, 2.0}, {3.0, 4.0, 5.0}, -1.3);
}

} // namespace doris::vectorized