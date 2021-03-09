# Configuration parameters
ANDROID_API = 18
ANDROID_ABI = armeabi-v7a
ANDROID_HOST = linux-x86_64
ANDROID_TOOLCHAIN = gcc
ANDROID_GCC_VERSION = 4.8
OPENSSL_VERSION = 1.0.2k
OPENSSL_PACKAGE = openssl-$(OPENSSL_VERSION)
OPENSSL_PATCH = $(OPENSSL_PACKAGE).patch
OPENSSL_TARBALL = $(OPENSSL_PACKAGE).tar.gz
OPENSSL_URL = https://www.openssl.org/source/$(OPENSSL_TARBALL)
OPENSSL_OPTIONS = -no-ssl2 -no-ssl3 -no-comp -no-hw -no-engine
OPENSSL_PREFIX = android-$(ANDROID_API)-$(ANDROID_ABI)-$(ANDROID_TOOLCHAIN)
OPENSSL_SOURCE = $(OPENSSL_PACKAGE)-$(ANDROID_ABI)

# Setup target parameters from ABI
ifneq ($(findstring armeabi,$(ANDROID_ABI)),)
ANDROID_ARCH := arm
ANDROID_TRIPLE := arm-linux-androideabi
ANDROID_TOOLARCH := $(ANDROID_TRIPLE)
OPENSSL_MACHINE := armv7
OPENSSL_SYSTEM := android
OPENSSL_TARGET := android-armeabi
endif

ifneq ($(findstring aarch64,$(ANDROID_ABI)),)
ANDROID_ARCH := arm64
ANDROID_TRIPLE := aarch64-linux-android
ANDROID_TOOLARCH := $(ANDROID_TRIPLE)
OPENSSL_MACHINE := aarch64
OPENSSL_SYSTEM := android64
OPENSSL_TARGET := android64-aarch64
endif

ifneq ($(findstring x86,$(ANDROID_ABI)),)
ANDROID_ARCH := x86
ANDROID_TRIPLE := i686-linux-android
ANDROID_TOOLARCH := $(ANDROID_ARCH)
OPENSSL_MACHINE := i686
OPENSSL_SYSTEM := android
OPENSSL_TARGET := android-x86
endif

# Validate Android NDK directory paths and use fallback directories where applicable
define direxists =
$(if $(wildcard $(1)),$(strip $(1)),"")
endef

define findfirstdir =
$(call direxists,$(firstword $(filter-out "",$(foreach val,$(3),$(call direxists,$(subst $(2),$(strip $(val)),$(1)))))))
endef

ifeq ($(call direxists,$(ANDROID_NDK)),"")
$(error invalid Android NDK root directory)
endif

ANDROID_LINK_SYSROOT := $(ANDROID_NDK)/platforms/android-$(ANDROID_API)/arch-$(ANDROID_ARCH)
ifeq ($(call direxists,$(ANDROID_LINK_SYSROOT)),"")
$(error invalid Android ABI or API level, could not locate Android NDK sysroot directory)
endif

ANDROID_SYSROOT := $(ANDROID_NDK)/sysroot
ifeq ($(call direxists,$(ANDROID_SYSROOT)),"")
ANDROID_SYSROOT := $(ANDROID_LINK_SYSROOT)
endif

ANDROID_GCC_VERSIONS := $(ANDROID_GCC_VERSION) 4.9 4.8
ANDROID_GCC_PREBUILT_template := $(ANDROID_NDK)/toolchains/$(ANDROID_TOOLARCH)-<<VERSION>>/prebuilt
ANDROID_GCC_PREBUILT := $(call findfirstdir,$(ANDROID_GCC_PREBUILT_template),<<VERSION>>,$(ANDROID_GCC_VERSIONS))
ifeq ($(ANDROID_GCC_PREBUILT),"")
$(error could not determine Android NDK GCC toolchain prebuilt directory)
endif

ANDROID_HOSTS := $(ANDROID_HOST) linux-x86_64 linux-x86 darwin-x86_64 darwin-x86
ANDROID_GCC_TOOLCHAIN_template := $(ANDROID_GCC_PREBUILT)/<<HOST>>
ANDROID_GCC_TOOLCHAIN := $(call findfirstdir,$(ANDROID_GCC_TOOLCHAIN_template),<<HOST>>,$(ANDROID_HOSTS))
ifeq ($(ANDROID_GCC_TOOLCHAIN),"")
$(error could not determine Android NDK GCC toolchain host directory)
endif

ANDROID_LLVM_VERSIONS := llvm llvm-3.6 llvm-3.5 llvm-3.4
ANDROID_LLVM_TOOLCHAIN_template := $(ANDROID_NDK)/toolchains/<<LLVM>>/prebuilt/$(notdir $(ANDROID_GCC_TOOLCHAIN))
ANDROID_LLVM_TOOLCHAIN := $(call findfirstdir,$(ANDROID_LLVM_TOOLCHAIN_template),<<LLVM>>,$(ANDROID_LLVM_VERSIONS))
ifeq ($(ANDROID_LLVM_TOOLCHAIN),"")
$(error could not determine Android NDK LLVM toolchain directory)
endif

# Configure toolchain
OPENSSL_CROSS_COMPILE :=
OPENSSL_CC :=
OPENSSL_RANLIB := $(ANDROID_GCC_TOOLCHAIN)/bin/$(ANDROID_TRIPLE)-ranlib

ifneq ($(findstring clang,$(ANDROID_TOOLCHAIN)),)
OPENSSL_TARGET := $(OPENSSL_TARGET)-clang
OPENSSL_CC := $(ANDROID_LLVM_TOOLCHAIN)/bin/clang
endif

ifneq ($(findstring gcc,$(ANDROID_TOOLCHAIN)),)
OPENSSL_CROSS_COMPILE := $(ANDROID_TRIPLE)-
OPENSSL_CC := $(ANDROID_GCC_TOOLCHAIN)/bin/$(ANDROID_TRIPLE)-gcc
endif

ifeq ($(OPENSSL_CC),)
$(error invalid toolchain specified for ANDROID_TOOLCHAIN)
endif

all: info $(OPENSSL_PREFIX)/lib/libssl.a

$(OPENSSL_TARBALL):
	@echo "Downloading OpenSSL tarball"
	wget $(OPENSSL_URL)

$(OPENSSL_PREFIX)/lib/libssl.a: $(OPENSSL_TARBALL)
	@echo "Decompressing OpenSSL package" && \
	( \
		set -e; \
		rm -rf $(OPENSSL_SOURCE); \
		rm -rf $(OPENSSL_PACKAGE); \
		tar xzf $(OPENSSL_TARBALL); \
	) && \
	mv $(OPENSSL_PACKAGE) $(OPENSSL_SOURCE)
	@if test -f $(OPENSSL_PATCH); then \
		echo "Patching OpenSSL source tree"; \
		( cd $(OPENSSL_SOURCE) && patch -p1 < ../$(OPENSSL_PATCH) ); \
	fi
	@echo "Building OpenSSL" && \
	export ANDROID_NDK="$(ANDROID_NDK)" && \
	export ANDROID_API="$(ANDROID_API)" && \
	export ANDROID_ARCH="$(ANDROID_ARCH)" && \
	export ANDROID_TRIPLE="$(ANDROID_TRIPLE)" && \
	export ANDROID_SYSROOT="$(ANDROID_SYSROOT)" && \
	export ANDROID_LINK_SYSROOT="$(ANDROID_LINK_SYSROOT)" && \
	export ANDROID_GCC_TOOLCHAIN="$(ANDROID_GCC_TOOLCHAIN)" && \
	export CROSS_SYSROOT="$(ANDROID_SYSROOT)" && \
	export SYSROOT="$(ANDROID_SYSROOT)" && \
	export ARCH="$(ANDROID_ARCH)" && \
	export MACHINE="$(OPENSSL_MACHINE)" && \
	export SYSTEM="$(OPENSSL_SYSTEM)" && \
	export CROSS_COMPILE="$(OPENSSL_CROSS_COMPILE)" && \
	export HOSTCC="$(ANDROID_TOOLCHAIN)" && \
	export PATH="$(ANDROID_GCC_TOOLCHAIN)/bin:$(ANDROID_LLVM_TOOLCHAIN)/bin:$(PATH)" && \
	( \
		cd $(OPENSSL_SOURCE); \
		perl Configure $(OPENSSL_TARGET) shared $(OPENSSL_OPTIONS) --prefix="`pwd`/../$(OPENSSL_PREFIX)" $(OPENSSL_CFLAGS) && \
		make depend && \
		make all && \
		make install CC=$(OPENSSL_CC) RANLIB=$(OPENSSL_RANLIB); \
	)

clean:
	@echo "Cleaning"
	rm -rf $(OPENSSL_SOURCE)
	rm -rf $(OPENSSL_PACKAGE)
	rm -rf $(OPENSSL_PREFIX)

info:
	@echo "OpenSSL build options"
	@echo "ANDROID_NDK = $(ANDROID_NDK)"
	@echo "ANDROID_API = $(ANDROID_API)"
	@echo "ANDROID_ABI = $(ANDROID_ABI)"
	@echo "ANDROID_HOST = $(ANDROID_HOST)"
	@echo "ANDROID_TOOLCHAIN = $(ANDROID_TOOLCHAIN)"
	@echo "ANDROID_TRIPLE = $(ANDROID_TRIPLE)"
	@echo "ANDROID_SYSROOT = $(ANDROID_SYSROOT)"
	@echo "ANDROID_LINK_SYSROOT = $(ANDROID_LINK_SYSROOT)"
	@echo "ANDROID_GCC_TOOLCHAIN = $(ANDROID_GCC_TOOLCHAIN)"
	@echo "ANDROID_LLVM_TOOLCHAIN = $(ANDROID_LLVM_TOOLCHAIN)"
	@echo "OPENSSL_VERSION = $(OPENSSL_VERSION)"
	@echo "OPENSSL_URL = $(OPENSSL_URL)"
	@echo "OPENSSL_OPTIONS = $(OPENSSL_OPTIONS)"
	@echo "OPENSSL_PREFIX = $(OPENSSL_PREFIX)"
	@echo "OPENSSL_CFLAGS = $(OPENSSL_CFLAGS)"
	@echo "OPENSSL_CC = $(OPENSSL_CC)"
	@echo "OPENSSL_RANLIB = $(OPENSSL_RANLIB)"

.PHONY: all clean info
