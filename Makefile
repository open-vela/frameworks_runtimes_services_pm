############################################################################
#
# Copyright (C) 2023 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

include $(APPDIR)/Make.defs

CXXEXT = .cpp

ifneq ($(CONFIG_SYSTEM_PACKAGE_SERVICE),)
CXXFLAGS += -Wno-class-memaccess
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/rapidjson/rapidjson/include  ${INCDIR_PREFIX}../include
AIDLSRCS += $(shell find aidl -name *.aidl)
AIDLFLAGS = --lang=cpp --include=aidl/ -I. -haidl/ -oaidl/
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXSRCS += $(wildcard src/*.cpp)
endif

ifneq ($(CONFIG_SYSTEM_PACKAGE_SERVICE_COMMAND),)
PROGNAME += pm
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MAINSRC += cmd/PmCommand.cpp
endif

ifneq ($(CONFIG_SYSTEM_PACKAGE_SERVICE_TEST),)
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/googletest/googletest/googletest/include
PROGNAME += pmTest
MAINSRC += test/PackageManagerTest.cpp
endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libxms.a
endif

EXPORT_FILES :=

include $(APPDIR)/Application.mk
