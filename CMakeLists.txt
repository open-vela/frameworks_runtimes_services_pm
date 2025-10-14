#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_SYSTEM_PACKAGE_SERVICE)

  set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
  set(CUR_TARGET package_manager)

  nuttx_add_prebuilt(BIN_TARGETS package_manager APP_TARGETS pm pmTest)

  nuttx_library_ifndef(SKIP_PREBUILT ${CUR_TARGET})
  nuttx_add_dependencies(TARGET ${CUR_TARGET} DEPENDS android_binder)

  set(CFLAGS -Wno-class-memaccess)
  set(INCDIR ${CURRENT_DIR}/include ${CURRENT_DIR}/../include
             ${CMAKE_CURRENT_BINARY_DIR}/aidl)
  file(GLOB_RECURSE CXXRCS ${CURRENT_DIR}/src/*.cpp)
  list(REMOVE_ITEM CXXRCS ${CURRENT_DIR}/src/InstallMain.cpp)

  nuttx_compile_options(${CFLAGS})
  nuttx_sources(${CXXRCS})
  nuttx_include_directories(${INCDIR})

  # aidl header need set_property
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_CXX_INCLUDE_DIRECTORIES ${CURRENT_DIR}/include
             ${CMAKE_CURRENT_BINARY_DIR}/aidl)

  # aidl sources
  file(GLOB_RECURSE AIDLS aidl/*.aidl)
  nuttx_add_aidl(
    TARGET
    ${CUR_TARGET}
    AIDL_BASE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_INCLUDE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_HEADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_FLAGS
    --lang=cpp
    AIDLS
    ${AIDLS})

  nuttx_add_application(
    NAME
    pmsInstaller
    STACKSIZE
    ${CONFIG_SYSTEM_SERVER_STACKSIZE}
    PRIORITY
    SCHED_PRIORITY_DEFAULT
    SRCS
    src/InstallMain.cpp
    INCLUDE_DIRECTORIES
    ${INCDIR}
    DEPENDS
    ${CUR_TARGET})

  # pm command tool
  if(CONFIG_SYSTEM_PACKAGE_SERVICE_COMMAND)
    nuttx_add_application(
      NAME
      pm
      STACKSIZE
      ${CONFIG_DEFAULT_TASK_STACKSIZE}
      PRIORITY
      SCHED_PRIORITY_DEFAULT
      SRCS
      cmd/PmCommand.cpp
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET})
  endif()

  # testcase
  if(CONFIG_SYSTEM_PACKAGE_SERVICE_TEST)
    nuttx_add_application(
      NAME
      pmTest
      STACKSIZE
      ${CONFIG_DEFAULT_TASK_STACKSIZE}
      PRIORITY
      SCHED_PRIORITY_DEFAULT
      SRCS
      test/PackageManagerTest.cpp
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET}
      googletest)
  endif()

endif()
