#
#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2016 Intel Corporation.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  Contact Information:
#  Intel Corporation, www.intel.com
#
#  BSD LICENSE
#
#  Copyright(c) 2016 Intel Corporation.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#  Copyright (c) 2003-2016 Intel Corporation. All rights reserved.
#

# set top_srcdir and include this file

ifeq (,$(top_srcdir))
$(error top_srcdir must be set to include makefile fragment)
endif

export os ?= $(shell uname -s | tr '[A-Z]' '[a-z]')
export arch := $(shell uname -m | sed -e 's,\(i[456]86\|athlon$$\),i386,')

ifeq (${CCARCH},$(filter ${CCARCH},gcc gcc4 icc clang))
	export CC := ${CCARCH}
else
	anerr := $(error Unknown C compiler arch: ${CCARCH})
endif

ifeq (${FCARCH},gfortran)
	export FC := gfortran
else
	anerr := $(error Unknown Fortran compiler arch: ${FCARCH})
endif # gfortran

BASECFLAGS := $(BASE_FLAGS) -pthread
LDFLAGS += $(BASE_FLAGS)
ASFLAGS += $(BASE_FLAGS)

ifeq ($(PSM2_MOCK_TESTING),1)
BASECFLAGS += -DPSM2_MOCK_TESTING=1
unexport LINKER_SCRIPT
# We skip the linker script for mock testing version, we want all symbols
# to be reachable from outside the library
else
LINKER_SCRIPT := -Wl,--version-script $(LINKER_SCRIPT_FILE)
endif

WERROR := -Werror
INCLUDES := -I$(top_srcdir)/include -I$(top_srcdir)/mpspawn -I$(top_srcdir)/include/$(os)-$(arch)

#
# use IFS provided hfi1_user.h if installed.
#
IFS_HFI_HEADER_PATH := /usr/include/uapi
INCLUDES += -I${IFS_HFI_HEADER_PATH}

BASECFLAGS +=-Wall $(WERROR)

#
# test if compiler supports 32B(AVX2)/64B(AVX512F) move instruction.
#
ifeq (${CC},icc)
	ifeq ($(PSM_DISABLE_AVX2),)
		MAVX2=-xATOM_SSE4.2 -DPSM_AVX512
	else
		MAVX2=-march=core-avx-i
	endif
else
	ifeq ($(PSM_DISABLE_AVX2),)
		MAVX2=-mavx2
	else
		MAVX2=-mavx
	endif
endif

ifneq (icc,${CC})
	ifeq ($(PSM_DISABLE_AVX2),)
		RET := $(shell echo "int main() {}" | ${CC} ${MAVX2} -E -dM -xc - 2>&1 | grep -q AVX2 ; echo $$?)
	else
		RET := $(shell echo "int main() {}" | ${CC} ${MAVX2} -E -dM -xc - 2>&1 | grep -q AVX ; echo $$?)
                anerr := $(warning ***NOTE TO USER**** Disabling AVX2 will harm performance)
	endif

	ifeq (0,${RET})
		BASECFLAGS += ${MAVX2}
	else
		anerr := $(error Compiler does not support ${MAVX2} )
	endif
else
		BASECFLAGS += ${MAVX2}
endif

# This support is dynamic at runtime, so is OK to enable as long as compiler can generate
# the code.
ifneq (,${PSM_AVX512})
	ifneq (icc,${CC})
		RET := $(shell echo "int main() {}" | ${CC} -mavx512f -E -dM -xc - 2>&1 | grep -q AVX512 ; echo $$?)
		ifeq (0,${RET})
			BASECFLAGS += -mavx512f
		else
			anerr := $(error Compiler does not support AVX512 )
		endif
		BASECFLAGS += -DPSM_AVX512
	endif
endif

#
# feature test macros for drand48_r
#
BASECFLAGS += -D_DEFAULT_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE

ifneq (,${HFI_BRAKE_DEBUG})
	BASECFLAGS += -DHFI_BRAKE_DEBUG
endif
ifneq (,${PSM_FI})
	BASECFLAGS += -DPSM_FI
endif
ifneq (,${PSM_DEBUG})
	BASECFLAGS += -O -g3 -DPSM_DEBUG -D_HFI_DEBUGGING -funit-at-a-time -Wp,-D_FORTIFY_SOURCE=2
else
	BASECFLAGS += -O3 -g3
endif
ifneq (,${PSM_COVERAGE}) # This check must come after PSM_DEBUG to override optimization setting
	BASECFLAGS += -O -fprofile-arcs -ftest-coverage
	LDFLAGS += -fprofile-arcs
endif
ifneq (,${PSM_LOG})
	 BASECFLAGS += -DPSM_LOG
ifneq (,${PSM_LOG_FAST_IO})
	 BASECFLAGS += -DPSM_LOG_FAST_IO
	 PSM2_ADDITIONAL_GLOBALS += psmi_log_fini;psmi_log_message;
endif
endif
ifneq (,${PSM_PERF})
	 BASECFLAGS += -DRDPMC_PERF_FRAMEWORK
endif
ifneq (,${PSM_HEAP_DEBUG})
	 BASECFLAGS += -DPSM_HEAP_DEBUG
	 PSM2_ADDITIONAL_GLOBALS += _psmi_heapdebug_val_heapallocs;
endif
ifneq (,${PSM_PROFILE})
	BASECFLAGS += -DPSM_PROFILE
endif
BASECFLAGS += -DNVIDIA_GPU_DIRECT
ifneq (,${PSM_CUDA})
	BASECFLAGS += -DPSM_CUDA
	CUDA_HOME ?= /usr/local/cuda
	INCLUDES += -I$(CUDA_HOME)/include
endif

BASECFLAGS += -fpic -fPIC -D_GNU_SOURCE

ASFLAGS += -g3 -fpic

BASECFLAGS += ${OPA_CFLAGS}

ifeq (${CCARCH},icc)
	BASECFLAGS += -fpic -fPIC -D_GNU_SOURCE -DPACK_STRUCT_STL=packed,
	LDFLAGS += -static-intel
else
	LDFLAGS += -Wl,--build-id
	ifeq (${CCARCH},$(filter ${CCARCH},gcc clang))
		BASECFLAGS += -funwind-tables -Wno-strict-aliasing -Wformat-security
	else
		ifneq (${CCARCH},gcc4)
			anerr := $(error Unknown compiler arch "${CCARCH}")
		endif # gcc4
	endif # gcc
endif # icc

# We run export here to ensure all the above setup is in the environment
# for sub makes. However, we exclude this during clean and distclean
# to avoid resolution of some variables that don't need to be resolved
# and avoid unnecessary missing file warnings during cleanup.
ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), distclean)
export
endif
endif

