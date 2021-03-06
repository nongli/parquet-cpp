# Copyright 2012 Cloudera Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


add_library(Example STATIC
  example_util.cc
)

SET(LINK_LIBS
  ${PARQUET_LIBS}
  Example
  ${PARQUET_EXTERNAL_LIBS})

FUNCTION(ADD_EXAMPLE EXAMPLE_NAME)
  ADD_EXECUTABLE(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cc)
  TARGET_LINK_LIBRARIES(${EXAMPLE_NAME} ${LINK_LIBS})
ENDFUNCTION()

ADD_EXAMPLE(compute-stats)
ADD_EXAMPLE(decode-benchmark)
ADD_EXAMPLE(parquet-reader)
ADD_EXAMPLE(generic-record-test)
ADD_EXAMPLE(parquet-record-reader)
ADD_EXAMPLE(dump-metadata)
