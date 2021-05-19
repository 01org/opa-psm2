#
#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2017 Intel Corporation.
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
#  Copyright(c) 2017 Intel Corporation.
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


HISTORY = .outdirs
HISTORIC_TARGETS = $(patsubst %, %_clean, $(shell cat $(HISTORY) 2> /dev/null))

RPM_NAME := libpsm2
CONFIG_FILE := .config
TEMP_INST_DIR := $(shell mktemp -d)

ifeq ($(CONFIG_FILE), $(wildcard $(CONFIG_FILE)))
include $(CONFIG_FILE)
endif

PSM_HAL_ENABLE ?= *

PSM_HAL_ENABLE_D = $(wildcard $(addprefix psm_hal_,$(PSM_HAL_ENABLE)))

PSM_HAL_INSTANCE_OBJFILES = $(addsuffix /*.o,$(PSM_HAL_ENABLE_D))

SUBDIRS = ptl_self ptl_ips ptl_am libuuid opa ${wildcard $(PSM_HAL_ENABLE_D)}
top_srcdir := $(shell readlink -m .)

# Default locations
OUTDIR := $(top_srcdir)/build_release
MOCK_OUTDIR := $(top_srcdir)/build_mock
DEBUG_OUTDIR := $(top_srcdir)/build_debug

# We need a temporary test variable, as the OUTDIR macro
# can be overriden by the shell and thus not run.
TESTOUTDIR= $(shell readlink -m $(OUTDIR))
ifeq ($(top_srcdir), $(TESTOUTDIR))
$(error OUTDIR cannot be the same as your source folder ${top_srcdir}))
endif

ifeq (/,$(TESTOUTDIR))
$(error OUTDIR cannot be the / folder ))
endif

# Forces any value to be full path.
# We don't need to override MOCK_OUTDIR or DEBUG_OUTDIR
# as they are recursive make invocations and use OUTDIR
ifneq ($(MAKECMDGOALS), mock)
ifneq ($(MAKECMDGOALS), debug)
override OUTDIR := $(shell readlink -m $(OUTDIR))
endif
endif

PSM2_VERNO_MAJOR := $(shell sed -n 's/^\#define.*PSM2_VERNO_MAJOR.*0x0\?\([1-9a-f]\?[0-9a-f]\+\).*/\1/p' $(top_srcdir)/psm2.h)
PSM2_VERNO_MINOR := $(shell sed -n 's/^\#define.*PSM2_VERNO_MINOR.*0x\([0-9]\?[0-9a-f]\+\).*/\1/p' $(top_srcdir)/psm2.h)
PSM2_LIB_MAJOR   := $(shell printf "%d" ${PSM2_VERNO_MAJOR})
PSM2_LIB_MINOR   := $(shell printf "%d" `sed -n 's/^\#define.*PSM2_VERNO_MINOR.*\(0x[0-9a-f]\+\).*/\1/p' $(top_srcdir)/psm2.h`)
LINKER_SCRIPT_FILE = ${OUTDIR}/psm2_linker_script.map
SOURCES_CHKSUM_FILES = Makefile buildflags.mak $(LINKER_SCRIPT_FILE) \
		`find . -regex '\(.*\.h\|.*\.c\)' -not -path "./test/*" -not -path "./tools/*" -not -path "_revision.c" | sort`
SOURCES_CHKSUM_VALUE = $(shell cat ${SOURCES_CHKSUM_FILES} | sha1sum | cut -d' ' -f 1)
OPA_LIB_MAJOR := 4
OPA_LIB_MINOR := 0

export PSM2_VERNO_MAJOR
export PSM2_LIB_MAJOR
export PSM2_VERNO_MINOR
export PSM2_LIB_MINOR
export OPA_LIB_MAJOR
export OPA_LIB_MINOR
export CCARCH ?= gcc
export FCARCH ?= gfortran
export AR ?= ar

include $(top_srcdir)/buildflags.mak

# We need to unexport these environs as during mock testing and normal calls,
# if they are exported then during each submake they will be evaulated again.
# This is costly and the LINKER_SCRIPT_FILE doesn't exist until after its
# target rule runs.
unexport SOURCES_CHKSUM_FILES
unexport SOURCES_CHKSUM_VALUE
unexport LINKER_SCRIPT_FILE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/ptl_ips -I$(OUTDIR)

ifneq (x86_64,$(arch))
   ifneq (i386,$(arch))
      anerr := $(error Unsupported architecture $(arch))
   endif
endif

ifndef LIBDIR
   ifeq (${arch},x86_64)
      INSTALL_LIB_TARG=/usr/lib64
   else
      INSTALL_LIB_TARG=/usr/lib
   endif
else
   INSTALL_LIB_TARG=${LIBDIR}
endif
export DESTDIR
export INSTALL_LIB_TARG

TARGLIB := libpsm2
COMPATMAJOR := $(shell sed -n 's/^\#define.*PSM2_VERNO_COMPAT_MAJOR.*0x0\?\([1-9a-f]\?[0-9a-f]\+\).*/\1/p' \
			 $(top_srcdir)/psm2.h)
COMPATLIB := libpsm_infinipath

MAJOR := $(PSM2_LIB_MAJOR)
MINOR := $(PSM2_LIB_MINOR)

nthreads := $(shell echo $$(( `nproc` * 2 )) )

# The following line sets the DISTRO variable to:
#  'rhel' if the host is running RHEL.
#  'suse' if the host is running SUSE.
#  'fedora' if the host is running Fedora.
#  'ubuntu' if the host is running Ubuntu.
#
# The DISTRO variable is used subsequently for variable
# behaviors of the 3 distros.

DISTRO := $(shell . /etc/os-release; if [ "$$ID" = "sle_hpc" ]; then ID="sles"; fi; echo $$ID)

# By default the following two variables have the following values:
LIBPSM2_COMPAT_CONF_DIR := /etc
LIBPSM2_COMPAT_SYM_CONF_DIR := /etc
# We can't set SPEC_FILE_RELEASE_DIST to an empty value, a space will result.
# It then messes up sed operations for PSM_CUDA=1.
# So leaving the commented out line here as documentation to NOT set it.
# SPEC_FILE_RELEASE_DIST :=
UDEV_40_PSM_RULES := %{_udevrulesdir}/40-psm.rules

ifeq (fedora,$(DISTRO))
	# On Fedora, we change these two variables to these values:
	LIBPSM2_COMPAT_CONF_DIR := /usr/lib
	LIBPSM2_COMPAT_SYM_CONF_DIR := %{_prefix}/lib
	SPEC_FILE_RELEASE_DIST := %{?dist}
	UDEV_40_PSM_RULES :=#
else ifeq (rhel,${DISTRO})
	# Insert code specific to RHEL here.
else ifeq (sles,${DISTRO})
	# Insert code specific to SLES here.
endif

ifdef PSM_CUDA
#Value needs to be something without spaces or dashes '-'
SPEC_FILE_RELEASE_DIST += cuda
endif

export 	LIBPSM2_COMPAT_CONF_DIR

# The desired version number comes from the most recent tag starting with "v"
ifeq (true, $(shell git rev-parse --is-inside-work-tree 2>/dev/null))
ISGIT := 1 # Cache the result for later
# Note, we don't define ISGIT if we are not in a git folder
VERSION := $(shell git describe --tags --abbrev=0 --match='psm-v*' | sed -e 's/^psm-v//' -e 's/-/_/')
else
ISGIT := 0
VERSION := version
endif

# If we have a file called 'rpm_release_extension' (as on github),
# we take the release extension number from this file
RELEASE_EXT := $(shell if [ -e rpm_release_extension ] ;\
                       then cat rpm_release_extension; fi)
CURRENTSHA := $(shell if [ $(ISGIT) = 1 -a -f rpm_release_extension ] ;\
                      then git log --pretty=format:'%h' -n 1; fi)
RPMEXTHASH := $(shell if [ $(ISGIT) = 1 -a -f rpm_release_extension ] ;\
                      then git log --pretty=format:'%h' -n 1 rpm_release_extension; fi)

# This logic should kick-in only on github
ifdef RELEASE_EXT
ifneq ($(CURRENTSHA), $(RPMEXTHASH))
# On github, the last commit for each release should be the one to bump up
# the release extension number in 'rpm_release_extension'. Further commits
# are counted here and appended to the final rpm name to distinguish commits
# present only on github
NCOMMITS := $(shell if [ $(ISGIT) = 1 -a -f rpm_release_extension ] ;\
                    then git log --children $(RPMEXTHASH)..$(CURRENTSHA) \
                    --pretty=oneline . | wc -l; fi)
RELEASE := $(RELEASE_EXT)_$(NCOMMITS)
endif
endif

# The desired release number comes the git describe following the version which
# is the number of commits since the version tag was planted suffixed by the g<commitid>
ifndef RELEASE
RELTAG := "psm-v$(VERSION)"
RELEASE := $(shell if [ -f rpm_release_extension ]; then cat rpm_release_extension;\
		   elif [ $(ISGIT) = 1 ] ; then git rev-list $(RELTAG)..HEAD -- . | wc -l; \
		   else echo "release" ; fi)
endif

DIST_SHA := ${shell if [ $(ISGIT) = 1 ] ; then git log -n1 --pretty=format:%H .; \
		else echo DIST_SHA ; fi}

# Concatenated version and release
ifndef VERSION_RELEASE_OVERRIDE
VERSION_RELEASE := $(VERSION).$(RELEASE)
else
VERSION_RELEASE := ${VERSION_RELEASE_OVERRIDE}
endif

LDLIBS := -lrt -ldl -lnuma ${EXTRA_LIBS} -pthread

PKG_CONFIG ?= pkg-config

UDEVDIR := $(shell $(PKG_CONFIG) --variable=udevdir udev 2>/dev/null)
ifndef UDEVDIR
	UDEVDIR = /lib/udev
endif

export UDEVDIR

# The DIST variable is a name kernel corresponding to:
# 1. The name of the directory containing the source code distribution
#    (see dist: target below).
# 2. The basename of the filename of the tar file created in the dist:
#    target.
DIST := ${RPM_NAME}-${VERSION_RELEASE}

# If user has empty RPM NAME BASEEXT (defined or not), then attempt to
# see if we are running on SLES 12.3 or newer.
# If we are, then change the base package name, but not the supporting
# packages to libpsm2-2. Do note this requires support both in the Makefile
# specfile target rule as well as changes in the libpsm2.spec.in
# file as well.
ifeq ($(RPM_NAME_BASEEXT),)
# Detect current version of the OS
OS := $(shell grep -m1 NAME /etc/os-release | cut -f 2 -d\")
OSVERSION := $(shell grep VERSION_ID /etc/os-release | cut -f 2 -d\" | cut -f 1 -d.)
OSSUBVERSION := $(shell grep VERSION_ID /etc/os-release | cut -f 2 -d\" | cut -f 2 -d.)

override RPM_NAME_BASEEXT := $(shell \
    if [ "$(OS)" = "SLES" -o "$(OS)" = "SLE_HPC" ]; then \
       if [ $(OSVERSION) -gt 11 ]; then \
          if [ $(OSVERSION) -eq 12 ]; then \
             if [ $(OSSUBVERSION) -gt 2 ]; then \
                echo "-2"; \
             fi \
          else \
             echo "-2"; \
          fi \
       fi \
    fi)
endif

HALDECLFILE=$(OUTDIR)/psm2_hal_inlines_d.h
HALIMPLFILE=$(OUTDIR)/psm2_hal_inlines_i.h

all: symlinks $(HALDECLFILE) $(HALIMPLFILE) | $(OUTDIR)
	@if [ ! -e $(HISTORY) ] || [ -z "`grep -E '^$(OUTDIR)$$' $(HISTORY)`" ]; then \
		echo $(OUTDIR) >> $(HISTORY); \
	fi
	# Our buildflags.mak exports all variables, all are propogated to submakes.
	@for subdir in $(SUBDIRS); do \
		mkdir -p $(OUTDIR)/$$subdir; \
		$(MAKE) -j $(nthreads) -C $$subdir OUTDIR=$(OUTDIR)/$$subdir; \
		if [ $$? -ne 0 ]; then exit 1; fi ;\
	done
	$(MAKE) -j $(nthreads) $(OUTDIR)/${TARGLIB}.so
	$(MAKE) -j $(nthreads) $(OUTDIR)/${TARGLIB}.a
	@mkdir -p $(OUTDIR)/compat
	$(MAKE) -j $(nthreads) -C compat OUTDIR=$(OUTDIR)/compat

$(HALDECLFILE): | $(OUTDIR)
	@test -f $(HALDECLFILE) || ( \
		n_hal_insts=$(words $(wildcard $(PSM_HAL_ENABLE_D)));\
		echo "#define PSMI_HAL_INST_CNT $$n_hal_insts"                > $(HALDECLFILE);\
		if [ $$n_hal_insts -eq 1 ]; then					       \
		    echo "#define PSMI_HAL_INLINE inline"                    >> $(HALDECLFILE);\
		    hal_inst_dir=$(PSM_HAL_ENABLE_D);					       \
		    echo "#define PSMI_HAL_CAT_INL_SYM(KERNEL) hfp_$(subst psm_hal_,,$(PSM_HAL_ENABLE_D))"	       \
				"## _ ## KERNEL"			     >> $(HALDECLFILE);\
		    echo "#include \"psm2_hal_inline_t.h\""		     >> $(HALDECLFILE);\
		else									       \
		    echo "#define PSMI_HAL_INLINE /* nothing */"             >> $(HALDECLFILE);\
		fi )

$(HALIMPLFILE): | $(OUTDIR)
	@test -f $(HALIMPLFILE) || ( \
		n_hal_insts=$(words $(wildcard $(PSM_HAL_ENABLE_D)));\
		if [ $$n_hal_insts -eq 1 ]; then\
		    hal_inst=$(PSM_HAL_ENABLE_D);\
		    echo "#include \"$$hal_inst/psm_hal_inline_i.h\""        >> $(HALIMPLFILE);\
		else\
		    echo "/* no inlining since more than 1 hal instance"     >> $(HALIMPLFILE);\
		    echo "   is included in the libpsm2 linkage.        */"  >> $(HALIMPLFILE);\
		fi )

%_clean:
	make OUTDIR=$* clean

clean: cleanlinks
	rm -rf ${OUTDIR}
	@if [ -e $(HISTORY) ]; then \
		grep -v -E "^$(OUTDIR)$$" $(HISTORY) > $(HISTORY)_tmp; \
		mv $(HISTORY)_tmp $(HISTORY); \
		if [ "`wc -c $(HISTORY) | cut -d ' ' -f 1`" -eq 0 ]; then \
			rm -f $(HISTORY); \
		fi; \
	fi
	rm -fr $(TEMP_INST_DIR)

# Easily add more items to config target if more options need
# to be cached.
config: $(CONFIG_FILE)

$(CONFIG_FILE):
	@echo PSM_HAL_ENABLE=$(PSM_HAL_ENABLE) > $(CONFIG_FILE)
	@echo CCARCH=$(CCARCH) >> $(CONFIG_FILE)
	@echo HFI_BRAKE_DEBUG=$(HFI_BRAKE_DEBUG) >> $(CONFIG_FILE)
	@echo PSM_DEBUG=$(PSM_DEBUG) >> $(CONFIG_FILE)
	@echo PSM_AVX512=$(PSM_AVX512) >> $(CONFIG_FILE)
	@echo PSM_LOG=$(PSM_LOG) >> $(CONFIG_FILE)
	@echo PSM_LOG_FAST_IO=$(PSM_LOG_FAST_IO) >> $(CONFIG_FILE)
	@echo PSM_PERF=$(PSM_PERF) >> $(CONFIG_FILE)
	@echo PSM_HEAP_DEBUG=$(PSM_HEAP_DEBUG) >> $(CONFIG_FILE)
	@echo PSM_PROFILE=$(PSM_PROFILE) >> $(CONFIG_FILE)
	@echo PSM_CUDA=$(PSM_CUDA) >> $(CONFIG_FILE)
	@echo Wrote $(CONFIG_FILE)

mock: OUTDIR := $(MOCK_OUTDIR)
mock:
	$(MAKE) OUTDIR=$(OUTDIR) PSM2_MOCK_TESTING=1

debug: OUTDIR := $(DEBUG_OUTDIR)
debug:
	$(MAKE) OUTDIR=$(OUTDIR) PSM_DEBUG=1

test_clean:
	if [ -d ./test ]; then \
		$(MAKE) -C test clean; \
	fi

specfile_clean:
	rm -f ${OUTDIR}/${RPM_NAME}.spec

distclean: specfile_clean cleanlinks $(HISTORIC_TARGETS) test_clean
	rm -f $(CONFIG_FILE)
	rm -rf ${OUTDIR}/${DIST}
	rm -f ${OUTDIR}/${DIST}.tar.gz
	rm -fr temp.* *.rej.patch

$(OUTDIR):
	mkdir -p ${OUTDIR}

symlinks:
	@test -L $(top_srcdir)/include/linux-x86_64 || \
		ln -sf linux-i386 $(top_srcdir)/include/linux-x86_64

cleanlinks:
	rm -rf $(top_srcdir)/include/linux-x86_64

install: all
	for subdir in $(SUBDIRS) ; do \
		mkdir -p $(OUTDIR)/$$subdir ; \
		$(MAKE) -j $(nthreads) -C $$subdir OUTDIR=$(OUTDIR)/$$subdir install ; \
	done
	$(MAKE) -j $(nthreads) $(OUTDIR)/${TARGLIB}.so OUTDIR=$(OUTDIR)
	$(MAKE) -j $(nthreads) -C compat OUTDIR=$(OUTDIR)/compat install
	install -D $(OUTDIR)/${TARGLIB}.so.${MAJOR}.${MINOR} \
		${DESTDIR}${INSTALL_LIB_TARG}/${TARGLIB}.so.${MAJOR}.${MINOR}
	(cd ${DESTDIR}${INSTALL_LIB_TARG} ; \
		ln -sf ${TARGLIB}.so.${MAJOR}.${MINOR} ${TARGLIB}.so.${MAJOR} ; \
		ln -sf ${TARGLIB}.so.${MAJOR} ${TARGLIB}.so)
	install -D $(OUTDIR)/${TARGLIB}.a \
		${DESTDIR}${INSTALL_LIB_TARG}/${TARGLIB}.a
	install -m 0644 -D psm2.h ${DESTDIR}/usr/include/psm2.h
	install -m 0644 -D psm2_mq.h ${DESTDIR}/usr/include/psm2_mq.h
	install -m 0644 -D psm2_am.h ${DESTDIR}/usr/include/psm2_am.h
ifneq (fedora,${DISTRO})
	install -m 0644 -D 40-psm.rules ${DESTDIR}$(UDEVDIR)/rules.d/40-psm.rules
endif
	# The following files and dirs were part of the noship rpm:
	mkdir -p ${DESTDIR}/usr/include/hfi1diag
	mkdir -p ${DESTDIR}/usr/include/hfi1diag/linux-x86_64
	install -m 0644 -D include/linux-x86_64/bit_ops.h ${DESTDIR}/usr/include/hfi1diag/linux-x86_64/bit_ops.h
	install -m 0644 -D include/linux-x86_64/sysdep.h ${DESTDIR}/usr/include/hfi1diag/linux-x86_64/sysdep.h
	install -m 0644 -D include/opa_udebug.h ${DESTDIR}/usr/include/hfi1diag/opa_udebug.h
	install -m 0644 -D include/opa_debug.h ${DESTDIR}/usr/include/hfi1diag/opa_debug.h
	install -m 0644 -D include/opa_intf.h ${DESTDIR}/usr/include/hfi1diag/opa_intf.h
	for h in opa_user_gen1.h opa_service_gen1.h opa_common_gen1.h ; do \
		sed -e 's/#include "opa_user_gen1.h"/#include "opa_user.h"/' \
			-e 's/#include "opa_common_gen1.h"/#include "opa_common.h"/' \
			-e 's/#include "hfi1_deprecated_gen1.h"/#include "hfi1_deprecated.h"/' \
			-e 's/#include "opa_service_gen1.h"/#include "opa_service.h"/' psm_hal_gen1/$$h \
				> $(TEMP_INST_DIR)/$$h ; \
	done
	cat include/opa_user.h    $(TEMP_INST_DIR)/opa_user_gen1.h  > $(TEMP_INST_DIR)/opa_user.h
	cat include/opa_service.h $(TEMP_INST_DIR)/opa_service_gen1.h > $(TEMP_INST_DIR)/opa_service.h
	install -m 0644 -D $(TEMP_INST_DIR)/opa_user.h    ${DESTDIR}/usr/include/hfi1diag/opa_user.h
	install -m 0644 -D $(TEMP_INST_DIR)/opa_service.h ${DESTDIR}/usr/include/hfi1diag/opa_service.h
	install -m 0644 -D $(TEMP_INST_DIR)/opa_common_gen1.h ${DESTDIR}/usr/include/hfi1diag/opa_common.h
	install -m 0644 -D include/opa_byteorder.h ${DESTDIR}/usr/include/hfi1diag/opa_byteorder.h
	install -m 0644 -D include/psm2_mock_testing.h ${DESTDIR}/usr/include/hfi1diag/psm2_mock_testing.h
	install -m 0644 -D include/opa_revision.h ${DESTDIR}/usr/include/hfi1diag/opa_revision.h
	install -m 0644 -D psmi_wrappers.h ${DESTDIR}/usr/include/hfi1diag/psmi_wrappers.h
	install -m 0644 -D psm_hal_gen1/hfi1_deprecated_gen1.h ${DESTDIR}/usr/include/hfi1diag/hfi1_deprecated.h
	rm -fr $(TEMP_INST_DIR)

specfile: specfile_clean | $(OUTDIR)
	sed -e 's/@VERSION@/'${VERSION_RELEASE}'/g' libpsm2.spec.in | \
		sed -e 's/@TARGLIB@/'${TARGLIB}'/g' \
			-e 's/@RPM_NAME@/'${RPM_NAME}'/g' \
			-e 's/@RPM_NAME_BASEEXT@/'${RPM_NAME_BASEEXT}'/g' \
			-e 's/@COMPATLIB@/'${COMPATLIB}'/g' \
			-e 's/@COMPATMAJOR@/'${COMPATMAJOR}'/g' \
			-e 's;@UDEVDIR@;'${UDEVDIR}';g' \
			-e 's/@MAJOR@/'${MAJOR}'/g' \
			-e 's/@MINOR@/'${MINOR}'/g' \
			-e 's:@LIBPSM2_COMPAT_CONF_DIR@:'${LIBPSM2_COMPAT_CONF_DIR}':g' \
			-e 's:@LIBPSM2_COMPAT_SYM_CONF_DIR@:'${LIBPSM2_COMPAT_SYM_CONF_DIR}':g' \
			-e 's;@SPEC_FILE_RELEASE_DIST@;'${SPEC_FILE_RELEASE_DIST}';g'  \
			-e 's/@DIST_SHA@/'${DIST_SHA}'/g' > \
		${OUTDIR}/${RPM_NAME}.spec
	if [ -f /etc/redhat-release ] && [ `grep -o "[0-9.]*" /etc/redhat-release | cut -d"." -f1` -lt 7 ]; then \
		sed -i 's;@40_PSM_RULES@;'${UDEVDIR}'/rules.d/40-psm.rules;g' ${OUTDIR}/${RPM_NAME}.spec; \
	else \
		sed -i 's;@40_PSM_RULES@;'${UDEV_40_PSM_RULES}';g' ${OUTDIR}/${RPM_NAME}.spec; \
	fi

# We can't totally prevent two make dist calls in a row from packaging
# the previous make dist, unless we switch to using a dedicated ./src folder
# That will come in the next major revision of the Makefile for now we can
# prevent the easy and default cases
#
# Notes on PRUNE_LIST:
# To make the dist, we always eliminate the psm_hal_MOCK dir.
# we also eliminate the psm hal instances that are not enabled via the PSM_HAL_ENABLE variable.
# To implement this, we build the prune list in two passes:
# 1.  The first pass includes all of the common items we want to exclude.
# 2.  The second pass we include the differnce of
#     (all of the PSM HAL instances) minus (the PSM hal instances that are enabled)
# The final prune list is supplied to find, and the dist is created.
dist: distclean
	mkdir -p ${OUTDIR}/${DIST}
	PRUNE_LIST="";										\
	for pd in ".git" "cscope*" "$(shell realpath --relative-to=${top_srcdir} ${OUTDIR})"	\
		"*.orig" "*~" "#*" ".gitignore" "doc" "libcm" "psm.supp" "test" "psm_hal_MOCK"	\
		 "psm_test" "tools" "artifacts" "*.rej.patch"; do			\
		PRUNE_LIST="$$PRUNE_LIST -name $$pd -prune -o";					\
	done;											\
	for hid in psm_hal_* ; do								\
		found=0;									\
		for ehid in $(PSM_HAL_ENABLE_D) ; do						\
			if [ "$$hid" = "$$ehid" ]; then						\
				found=1;							\
				break;								\
			fi;									\
		done;										\
		if [ $$found -eq 0 ]; then							\
			PRUNE_LIST="$$PRUNE_LIST -name $$hid -prune -o";			\
		fi;										\
	done;											\
	for x in $$(/usr/bin/find .								\
				$$PRUNE_LIST							\
			-print); do								\
		dir=$$(dirname $$x);								\
		mkdir -p ${OUTDIR}/${DIST}/$$dir;						\
		[ ! -d $$x ] && cp $$x ${OUTDIR}/${DIST}/$$dir;					\
	done
	if [ $(ISGIT) = 1 ] ; then git log -n1 --pretty=format:%H . > ${OUTDIR}/${DIST}/COMMIT ; fi
	echo ${RELEASE} > ${OUTDIR}/${DIST}/rpm_release_extension
	cd ${OUTDIR}; tar czvf ${DIST}.tar.gz ${DIST}
	@echo "${DIST}.tar.gz is located in ${OUTDIR}/${DIST}.tar.gz"

ofeddist:
	$(MAKE) -j $(nthreads) dist

# rebuild the cscope database, skipping sccs files, done once for
# top level
cscope:
	find * -type f ! -name '[ps].*' \( -iname '*.[cfhs]' -o \
	  -iname \\*.cc -o -name \\*.cpp -o -name \\*.f90 \) -print | cscope -bqu -i -

sources-checksum:
	@echo ${SOURCES_CHKSUM_VALUE}

${TARGLIB}-objs := ptl_am/am_reqrep_shmem.o	\
		   ptl_am/am_reqrep.o		\
		   ptl_am/ptl.o			\
		   ptl_am/cmarwu.o		\
		   ptl_am/am_cuda_memhandle_cache.o  \
		   psm_context.o		\
		   psm_ep.o			\
		   psm_ep_connect.o		\
		   psm_error.o			\
		   psm_utils.o			\
		   psm_sysbuf.o			\
		   psm_timer.o			\
		   psm_am.o			\
		   psm_mq.o			\
		   psm_mq_utils.o		\
		   psm_mq_recv.o		\
		   psm_mpool.o			\
		   psm_stats.o			\
		   psm_memcpy.o			\
		   psm_mock.o			\
		   psm.o			\
		   psm_perf.o			\
		   libuuid/psm_uuid.o		\
		   libuuid/parse.o		\
		   libuuid/pack.o		\
		   libuuid/unpack.o		\
		   libuuid/unparse.o		\
		   ptl_ips/ptl.o		\
		   ptl_ips/ptl_rcvthread.o	\
		   ptl_ips/ips_scb.o		\
		   ptl_ips/ips_epstate.o	\
		   ptl_ips/ips_recvq.o		\
		   ptl_ips/ips_recvhdrq.o	\
		   ptl_ips/ips_proto.o		\
		   ptl_ips/ips_proto_recv.o	\
		   ptl_ips/ips_proto_connect.o  \
		   ptl_ips/ips_proto_expected.o \
		   ptl_ips/ips_tid.o		\
		   ptl_ips/ips_tidcache.o       \
		   ptl_ips/ips_tidflow.o        \
		   ptl_ips/ips_crc32.o 		\
		   ptl_ips/ips_proto_dump.o	\
		   ptl_ips/ips_proto_mq.o       \
		   ptl_ips/ips_proto_am.o       \
		   ptl_ips/ips_path_rec.o       \
		   ptl_ips/ips_opp_path_rec.o   \
		   ptl_ips/ips_writehdrq.o	\
		   ptl_self/ptl.o		\
		   opa/*.o			\
		   psm_diags.o                  \
		   psm2_hal.o			\
		   $(PSM_HAL_INSTANCE_OBJFILES)	\
		   psmi_wrappers.o

${TARGLIB}-objs := $(patsubst %.o, ${OUTDIR}/%.o, ${${TARGLIB}-objs})

DEPS:= $(${TARGLIB}-objs:.o=.d)
-include $(DEPS)

${OUTDIR}/${TARGLIB}.so: ${OUTDIR}/${TARGLIB}.so.${MAJOR}
	ln -fs ${TARGLIB}.so.${MAJOR}.${MINOR} $@

${OUTDIR}/${TARGLIB}.so.${MAJOR}: ${OUTDIR}/${TARGLIB}.so.${MAJOR}.${MINOR}
	ln -fs ${TARGLIB}.so.${MAJOR}.${MINOR} $@

# when we build the shared library, generate a revision and date
# string in it, for easier id'ing when people may have copied the
# file around.  Generate it such that the ident command can find it
# and strings -a | grep OPA does a reasonable job as well.
$(OUTDIR)/${TARGLIB}.so.${MAJOR}.${MINOR}: ${${TARGLIB}-objs} $(LINKER_SCRIPT_FILE)
	echo "char psmi_hfi_IFS_version[]=\"`printenv RELEASE_TAG`\";" > ${OUTDIR}/_revision.c
	date -u -d@$${SOURCE_DATE_EPOCH:-$$(date +%s)} +'char psmi_hfi_build_timestamp[] ="%F %T%:z";' >> ${OUTDIR}/_revision.c
	echo "char psmi_hfi_sources_checksum[] =\"${SOURCES_CHKSUM_VALUE}\";" >> ${OUTDIR}/_revision.c
	echo "char psmi_hfi_git_checksum[] =\"`git rev-parse HEAD`\";" >> ${OUTDIR}/_revision.c
	$(CC) -c $(CFLAGS) $(BASECFLAGS) $(INCLUDES) ${OUTDIR}/_revision.c -o $(OUTDIR)/_revision.o
	$(CC) $(LINKER_SCRIPT) $(LDFLAGS) -o $@ -Wl,-soname=${TARGLIB}.so.${MAJOR} -shared \
		${${TARGLIB}-objs} $(OUTDIR)/_revision.o $(LDLIBS)

$(OUTDIR)/${TARGLIB}.a: $(OUTDIR)/${TARGLIB}.so.${MAJOR}.${MINOR}
	$(AR) rcs $(OUTDIR)/${TARGLIB}.a ${${TARGLIB}-objs} $(OUTDIR)/_revision.o

${OUTDIR}/%.o: ${top_srcdir}/%.c
	$(CC) $(CFLAGS) $(BASECFLAGS) $(INCLUDES) -MMD -c $< -o $@

$(LINKER_SCRIPT_FILE): psm2_linker_script_map.in
	sed "s/_psm2_additional_globals_;/$(PSM2_ADDITIONAL_GLOBALS)/" \
	     psm2_linker_script_map.in > ${OUTDIR}/psm2_linker_script.map

.PHONY: all %_clean clean config mock debug distclean symlinks cleanlinks install specfile dist ofeddist cscope sources-checksum
