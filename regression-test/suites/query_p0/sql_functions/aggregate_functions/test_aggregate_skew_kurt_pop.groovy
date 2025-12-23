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

// The cases is copied from https://github.com/trinodb/trino/tree/master
// /testing/trino-product-tests/src/main/resources/sql-tests/testcases/aggregate
// and modified by Doris.

suite("test_skew_kurt") {
    sql "DROP TABLE IF EXISTS test_skew_kurt_tbl"
    sql """
        CREATE TABLE test_skew_kurt_tbl (
            id INT,
            val_double DOUBLE
        ) DISTRIBUTED BY HASH(id) BUCKETS 1 PROPERTIES("replication_num" = "1");
    """

    sql "INSERT INTO test_skew_kurt_tbl VALUES (1, 1), (2, 2), (3, 3), (4, 4), (5, 5)"

    qt_skew "SELECT skew(val_double) FROM test_skew_kurt_tbl"
    qt_kurt "SELECT kurt(val_double) FROM test_skew_kurt_tbl"
    
    sql "INSERT INTO test_skew_kurt_tbl VALUES (6, NULL)"
    qt_skew_null "SELECT skew(val_double) FROM test_skew_kurt_tbl"
}