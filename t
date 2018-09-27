diff --git a/CONTRIBUTING.md b/CONTRIBUTING.md
index 10e62d1..1934b4b 100644
--- a/CONTRIBUTING.md
+++ b/CONTRIBUTING.md
@@ -81,8 +81,12 @@ Examples:
     Qt: Add feed bump button
     Trivial: Fix typo in init.cpp
 
-If a pull request is specifically not to be considered for merging (yet) please
-prefix the title with [WIP] or use [Tasks Lists](https://help.github.com/articles/basic-writing-and-formatting-syntax/#task-lists)
+Note that translations should not be submitted as pull requests, please see
+[Translation Process](http://github.com/blockchaingate/fabcoin/blob/master/doc/translation_process.md) 
+for more information on helping with translations.
+
+If a pull request is not to be considered for merging (yet), please
+prefix the title with [WIP] or use [Tasks Lists](http://help.github.com/articles/basic-writing-and-formatting-syntax/#task-lists)
 in the body of the pull request to indicate tasks are pending.
 
 The body of the pull request should contain enough description about what the
@@ -153,6 +157,14 @@ behaviour of code within the pull request (bugs must be preserved as is).
 Project maintainers aim for a quick turnaround on refactoring pull requests, so
 where possible keep them short, uncomplex and easy to verify.
 
+Pull requests that refactor the code should not be made by new contributors. It
+requires a certain level of experience to know where the code belongs to and to
+understand the full ramification (including rebase effort of open pull requests).
+
+Trivial pull requests or pull requests that refactor the code with no clear
+benefits may be immediately closed by the maintainers to reduce unnecessary
+workload on reviewing.
+
 
 "Decision Making" Process
 -------------------------
@@ -174,7 +186,7 @@ In general, all pull requests must:
     the project (for example refactoring for modularisation);
   - Be well peer reviewed;
   - Have unit tests and functional tests where appropriate;
-  - Follow code style guidelines;
+  - Follow code style guidelines ([C++](doc/developer-notes.md), [functional tests](test/functional/README.md));
   - Not break the existing test suite;
   - Where bugs are fixed, where possible, there should be unit tests
     demonstrating the bug and also proving the fix. This helps prevent regression.
diff --git a/Makefile.am b/Makefile.am
index 94a3b74..b2903e8 100644
--- a/Makefile.am
+++ b/Makefile.am
@@ -43,6 +43,7 @@ DIST_CONTRIB = $(top_srcdir)/contrib/fabcoin-cli.bash-completion \
 	       $(top_srcdir)/contrib/fabcoin-tx.bash-completion \
 	       $(top_srcdir)/contrib/fabcoind.bash-completion \
 	       $(top_srcdir)/contrib/init \
+	       $(top_srcdir)/contrib/install_db4.sh \
 	       $(top_srcdir)/contrib/rpm
 DIST_SHARE = \
   $(top_srcdir)/share/genbuild.sh \
@@ -281,4 +282,4 @@ DISTCHECK_CONFIGURE_FLAGS = --enable-man
 
 clean-local:
 	rm -rf coverage_percent.txt test_fabcoin.coverage/ total.coverage/ test/tmp/ cache/ $(OSX_APP)
-	rm -rf test/functional/__pycache__
+	rm -rf test/functional/__pycache__ test/functional/test_framework/__pycache__ test/cache
diff --git a/configure.ac b/configure.ac
index 51d9bd5..aa05d85 100644
--- a/configure.ac
+++ b/configure.ac
@@ -1,7 +1,7 @@
 dnl require autoconf 2.60 (AS_ECHO/AS_ECHO_N)
 AC_PREREQ([2.60])
 define(_CLIENT_VERSION_MAJOR, 0)
-define(_CLIENT_VERSION_MINOR, 15)
+define(_CLIENT_VERSION_MINOR, 16)
 define(_CLIENT_VERSION_REVISION, 2)
 define(_CLIENT_VERSION_BUILD, 3)
 define(_CLIENT_VERSION_IS_RELEASE, true)
@@ -179,9 +179,9 @@ AC_ARG_ENABLE([ccache],
 AC_ARG_ENABLE([lcov],
   [AS_HELP_STRING([--enable-lcov],
   [enable lcov testing (default is no)])],
-  [use_lcov=yes],
+  [use_lcov=$enableval],
   [use_lcov=no])
-  
+
 AC_ARG_ENABLE([lcov-branch-coverage],
   [AS_HELP_STRING([--enable-lcov-branch-coverage],
   [enable lcov testing branch coverage (default is no)])],
@@ -194,14 +194,14 @@ AC_ARG_ENABLE([glibc-back-compat],
   [use_glibc_compat=$enableval],
   [use_glibc_compat=no])
 
-AC_ARG_ENABLE([experimental-asm],
-  [AS_HELP_STRING([--enable-experimental-asm],
-  [Enable experimental assembly routines (default is no)])],
-  [experimental_asm=$enableval],
-  [experimental_asm=no])
+AC_ARG_ENABLE([asm],
+  [AS_HELP_STRING([--enable-asm],
+  [Enable assembly routines (default is yes)])],
+  [use_asm=$enableval],
+  [use_asm=yes])
 
-if test "x$experimental_asm" = xyes; then
-  AC_DEFINE(EXPERIMENTAL_ASM, 1, [Define this symbol to build in experimental assembly routines])
+if test "x$use_asm" = xyes; then
+  AC_DEFINE(USE_ASM, 1, [Define this symbol to build in assembly routines])
 fi
 
 AC_ARG_WITH([system-univalue],
@@ -253,12 +253,14 @@ if test "x$enable_debug" = xyes; then
 fi
 
 CPPFLAGS="$CPPFLAGS -DFASC_BUILD"
+
 ERROR_CXXFLAGS=
 if test "x$enable_werror" = "xyes"; then
   if test "x$CXXFLAG_WERROR" = "x"; then
     AC_MSG_ERROR("enable-werror set but -Werror is not usable")
   fi
   AX_CHECK_COMPILE_FLAG([-Werror=vla],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=vla"],,[[$CXXFLAG_WERROR]])
+  AX_CHECK_COMPILE_FLAG([-Werror=thread-safety-analysis],[ERROR_CXXFLAGS="$ERROR_CXXFLAGS -Werror=thread-safety-analysis"],,[[$CXXFLAG_WERROR]])
 fi
 
 if test "x$CXXFLAGS_overridden" = "xno"; then
@@ -267,6 +269,7 @@ if test "x$CXXFLAGS_overridden" = "xno"; then
   AX_CHECK_COMPILE_FLAG([-Wformat],[CXXFLAGS="$CXXFLAGS -Wformat"],,[[$CXXFLAG_WERROR]])
   AX_CHECK_COMPILE_FLAG([-Wvla],[CXXFLAGS="$CXXFLAGS -Wvla"],,[[$CXXFLAG_WERROR]])
   AX_CHECK_COMPILE_FLAG([-Wformat-security],[CXXFLAGS="$CXXFLAGS -Wformat-security"],,[[$CXXFLAG_WERROR]])
+  AX_CHECK_COMPILE_FLAG([-Wthread-safety-analysis],[CXXFLAGS="$CXXFLAGS -Wthread-safety-analysis"],,[[$CXXFLAG_WERROR]])
 
   ## Some compilers (gcc) ignore unknown -Wno-* options, but warn about all
   ## unknown options if any other warning is produced. Test the -Wfoo case, and
@@ -278,6 +281,7 @@ if test "x$CXXFLAGS_overridden" = "xno"; then
   AX_CHECK_COMPILE_FLAG([-Wimplicit-fallthrough],[CXXFLAGS="$CXXFLAGS -Wno-implicit-fallthrough"],,[[$CXXFLAG_WERROR]])
   AX_CHECK_COMPILE_FLAG([-Wno-unknown-pragmas],[CXXFLAGS="$CXXFLAGS -Wno-unknown-pragmas"],,[[$CXXFLAG_WERROR]])
 fi
+
 # Check for optional instruction set support. Enabling these does _not_ imply that all code will
 # be compiled with them, rather that specific objects/libs may use them after checking for runtime
 # compatibility.
@@ -465,6 +469,9 @@ case $host in
    *openbsd*)
      LEVELDB_TARGET_FLAGS="-DOS_OPENBSD"
      ;;
+   *netbsd*)
+     LEVELDB_TARGET_FLAGS="-DOS_NETBSD"
+     ;;
    *)
      OTHER_OS=`echo ${host_os} | awk '{print toupper($0)}'`
      AC_MSG_WARN([Guessing LevelDB OS as OS_${OTHER_OS}, please check whether this is correct, if not add an entry to configure.ac.])
@@ -677,6 +684,28 @@ AC_LINK_IFELSE([AC_LANG_SOURCE([
   ]
 )
 
+TEMP_LDFLAGS="$LDFLAGS"
+LDFLAGS="$TEMP_LDFLAGS $PTHREAD_CFLAGS"
+AC_MSG_CHECKING([for thread_local support])
+AC_LINK_IFELSE([AC_LANG_SOURCE([
+  #include <thread>
+  static thread_local int foo = 0;
+  static void run_thread() { foo++;}
+  int main(){
+  for(int i = 0; i < 10; i++) { std::thread(run_thread).detach();}
+  return foo;
+  }
+  ])],
+  [
+    AC_DEFINE(HAVE_THREAD_LOCAL,1,[Define if thread_local is supported.])
+    AC_MSG_RESULT(yes)
+  ],
+  [
+    AC_MSG_RESULT(no)
+  ]
+)
+LDFLAGS="$TEMP_LDFLAGS"
+
 # Check for different ways of gathering OS randomness
 AC_MSG_CHECKING(for Linux getrandom syscall)
 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
@@ -725,14 +754,12 @@ AC_SUBST(LEVELDB_CPPFLAGS)
 AC_SUBST(LIBLEVELDB)
 AC_SUBST(LIBMEMENV)
 
-
 CRYPTOPP_CPPFLAGS=
 LIBCRYPTOPP=
 AM_CONDITIONAL([EMBEDDED_CRYPTOPP],[true])
 AC_SUBST(CRYPTOPP_CPPFLAGS)
 AC_SUBST(LIBCRYPTOPP)
 
-
 if test x$enable_wallet != xno; then
     dnl Check for libdb_cxx only if wallet enabled
     FABCOIN_FIND_BDB48
@@ -770,6 +797,9 @@ define(MINIMUM_REQUIRED_BOOST, 1.58.0)
 
 dnl Check for boost libs
 AX_BOOST_BASE([MINIMUM_REQUIRED_BOOST])
+if test x$want_boost = xno; then
+    AC_MSG_ERROR([[only libfabcoinconsensus can be built without boost]])
+fi
 AX_BOOST_SYSTEM
 AX_BOOST_FILESYSTEM
 AX_BOOST_PROGRAM_OPTIONS
@@ -861,14 +891,14 @@ TEMP_CPPFLAGS="$CPPFLAGS"
 CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
 AC_MSG_CHECKING([for mismatched boost c++11 scoped enums])
 AC_LINK_IFELSE([AC_LANG_PROGRAM([[
-  #include "boost/config.hpp"
-  #include "boost/version.hpp"
+  #include <boost/config.hpp>
+  #include <boost/version.hpp>
   #if !defined(BOOST_NO_SCOPED_ENUMS) && !defined(BOOST_NO_CXX11_SCOPED_ENUMS) && BOOST_VERSION < 105700
   #define BOOST_NO_SCOPED_ENUMS
   #define BOOST_NO_CXX11_SCOPED_ENUMS
   #define CHECK
   #endif
-  #include "boost/filesystem.hpp"
+  #include <boost/filesystem.hpp>
   ]],[[
   #if defined(CHECK)
     boost::filesystem::copy_file("foo", "bar");
@@ -1277,7 +1307,7 @@ AM_CONDITIONAL([USE_LCOV],[test x$use_lcov = xyes])
 AM_CONDITIONAL([GLIBC_BACK_COMPAT],[test x$use_glibc_compat = xyes])
 AM_CONDITIONAL([HARDEN],[test x$use_hardening = xyes])
 AM_CONDITIONAL([ENABLE_HWCRC32],[test x$enable_hwcrc32 = xyes])
-AM_CONDITIONAL([EXPERIMENTAL_ASM],[test x$experimental_asm = xyes])
+AM_CONDITIONAL([USE_ASM],[test x$use_asm = xyes])
 
 AC_DEFINE(CLIENT_VERSION_MAJOR, _CLIENT_VERSION_MAJOR, [Major version])
 AC_DEFINE(CLIENT_VERSION_MINOR, _CLIENT_VERSION_MINOR, [Minor version])
@@ -1393,7 +1423,7 @@ case ${OS} in
    ;;
 esac
 
-echo 
+echo
 echo "Options used to compile and link:"
 echo "  with wallet   = $enable_wallet"
 echo "  with gpuminer   = $enable_gpu"
@@ -1407,9 +1437,10 @@ echo "  with zmq      = $use_zmq"
 echo "  with test     = $use_tests"
 echo "  with bench    = $use_bench"
 echo "  with upnp     = $use_upnp"
+echo "  use asm       = $use_asm"
 echo "  debug enabled = $enable_debug"
 echo "  werror        = $enable_werror"
-echo 
+echo
 echo "  target os     = $TARGET_OS"
 echo "  build os      = $BUILD_OS"
 echo
@@ -1420,4 +1451,4 @@ echo "  CXX           = $CXX"
 echo "  CXXFLAGS      = $CXXFLAGS"
 echo "  LDFLAGS       = $LDFLAGS"
 echo "  ARFLAGS       = $ARFLAGS"
-echo 
+echo
diff --git a/depends/Makefile b/depends/Makefile
index 0ddd348..14e94ba 100644
--- a/depends/Makefile
+++ b/depends/Makefile
@@ -21,7 +21,6 @@ BUILD_ID_SALT ?= salt
 host:=$(BUILD)
 ifneq ($(HOST),)
 host:=$(HOST)
-host_toolchain:=$(HOST)-
 endif
 
 ifneq ($(DEBUG),)
diff --git a/depends/README.md b/depends/README.md
index 23eec8e..2abee17 100644
--- a/depends/README.md
+++ b/depends/README.md
@@ -28,6 +28,22 @@ Common `host-platform-triplets` for cross compilation are:
 
 No other options are needed, the paths are automatically configured.
 
+Install the required dependencies: Ubuntu & Debian
+--------------------------------------------------
+
+For macOS cross compilation:
+
+    sudo apt-get install curl librsvg2-bin libtiff-tools bsdmainutils cmake imagemagick libcap-dev libz-dev libbz2-dev python-setuptools
+
+For Win32/Win64 cross compilation:
+
+- see [build-windows.md](../doc/build-windows.md#cross-compilation-for-ubuntu-and-windows-subsystem-for-linux)
+
+For linux (including i386, ARM) cross compilation:
+
+    sudo apt-get install curl g++-aarch64-linux-gnu g++-4.8-aarch64-linux-gnu gcc-4.8-aarch64-linux-gnu binutils-aarch64-linux-gnu g++-arm-linux-gnueabihf g++-4.8-arm-linux-gnueabihf gcc-4.8-arm-linux-gnueabihf binutils-arm-linux-gnueabihf g++-4.8-multilib gcc-4.8-multilib binutils-gold bsdmainutils
+
+
 Dependency Options:
 The following can be set when running make: make FOO=bar
 
diff --git a/depends/hosts/default.mk b/depends/hosts/default.mk
index 6f60d6b..144e5f8 100644
--- a/depends/hosts/default.mk
+++ b/depends/hosts/default.mk
@@ -1,3 +1,7 @@
+ifneq ($(host),$(build))
+host_toolchain:=$(host)-
+endif
+
 default_host_CC = $(host_toolchain)gcc
 default_host_CXX = $(host_toolchain)g++
 default_host_AR = $(host_toolchain)ar
diff --git a/depends/packages/expat.mk b/depends/packages/expat.mk
index 7f48472..61ff0f0 100644
--- a/depends/packages/expat.mk
+++ b/depends/packages/expat.mk
@@ -1,6 +1,6 @@
 package=expat
 $(package)_version=2.2.1
-$(package)_download_path=https://downloads.sourceforge.net/project/expat/expat/$($(package)_version)
+$(package)_download_path=http://github.com/libexpat/libexpat/releases/download/R_2_2_1/
 $(package)_file_name=$(package)-$($(package)_version).tar.bz2
 $(package)_sha256_hash=1868cadae4c82a018e361e2b2091de103cd820aaacb0d6cfa49bd2cd83978885
 
diff --git a/depends/packages/libevent.mk b/depends/packages/libevent.mk
index 00231d7..5f622f8 100644
--- a/depends/packages/libevent.mk
+++ b/depends/packages/libevent.mk
@@ -9,7 +9,7 @@ define $(package)_preprocess_cmds
 endef
 
 define $(package)_set_vars
-  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress
+  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
   $(package)_config_opts_release=--disable-debug-mode
   $(package)_config_opts_linux=--with-pic
 endef
diff --git a/depends/packages/native_ds_store.mk b/depends/packages/native_ds_store.mk
index 49f5829..d699e6e 100644
--- a/depends/packages/native_ds_store.mk
+++ b/depends/packages/native_ds_store.mk
@@ -1,9 +1,8 @@
 package=native_ds_store
-$(package)_version=1.1.0
-$(package)_download_path=https://bitbucket.org/al45tair/ds_store/get
-$(package)_download_file=v$($(package)_version).tar.bz2
-$(package)_file_name=$(package)-$($(package)_version).tar.bz2
-$(package)_sha256_hash=921596764d71d1bbd3297a90ef6d286f718794d667e4f81d91d14053525d64c1
+$(package)_version=1.1.2
+$(package)_download_path=http://github.com/al45tair/ds_store/archive/
+$(package)_file_name=v$($(package)_version).tar.gz
+$(package)_sha256_hash=3b3ecb7bf0a5157f5b6010bc3af7c141fb0ad3527084e63336220d22744bc20c
 $(package)_install_libdir=$(build_prefix)/lib/python/dist-packages
 $(package)_dependencies=native_biplist
 
diff --git a/depends/packages/native_mac_alias.mk b/depends/packages/native_mac_alias.mk
index 85a8a40..59bb298 100644
--- a/depends/packages/native_mac_alias.mk
+++ b/depends/packages/native_mac_alias.mk
@@ -1,14 +1,13 @@
 package=native_mac_alias
-$(package)_version=1.1.0
-$(package)_download_path=https://bitbucket.org/al45tair/mac_alias/get
-$(package)_download_file=v$($(package)_version).tar.bz2
-$(package)_file_name=$(package)-$($(package)_version).tar.bz2
-$(package)_sha256_hash=87ad827e66790028361e43fc754f68ed041a9bdb214cca03c853f079b04fb120
+$(package)_version=2.0.6
+$(package)_download_path=http://github.com/al45tair/mac_alias/archive/
+$(package)_file_name=v$($(package)_version).tar.gz
+$(package)_sha256_hash=78a3332d9a597eebf09ae652d38ad1e263b28db5c2e6dd725fad357b03b77371
 $(package)_install_libdir=$(build_prefix)/lib/python/dist-packages
 $(package)_patches=python3.patch
 
 define $(package)_preprocess_cmds
-  patch -p1 < $($(package)_patch_dir)/python3.patch
+    patch -p1 < $($(package)_patch_dir)/python3.patch
 endef
 
 define $(package)_build_cmds
diff --git a/depends/packages/openssl.mk b/depends/packages/openssl.mk
index 5ee9f17..37f0c28 100644
--- a/depends/packages/openssl.mk
+++ b/depends/packages/openssl.mk
@@ -47,6 +47,7 @@ $(package)_config_opts_linux=-fPIC -Wa,--noexecstack
 $(package)_config_opts_x86_64_linux=linux-x86_64
 $(package)_config_opts_i686_linux=linux-generic32
 $(package)_config_opts_arm_linux=linux-generic32
+$(package)_config_opts_armv7l_linux=linux-generic32
 $(package)_config_opts_aarch64_linux=linux-generic64
 $(package)_config_opts_mipsel_linux=linux-generic32
 $(package)_config_opts_mips_linux=linux-generic32
diff --git a/depends/packages/qt.mk b/depends/packages/qt.mk
index bbfdb76..8491927 100644
--- a/depends/packages/qt.mk
+++ b/depends/packages/qt.mk
@@ -8,7 +8,7 @@ $(package)_dependencies=openssl zlib
 $(package)_linux_dependencies=freetype fontconfig libxcb libX11 xproto libXext
 $(package)_build_subdir=qtbase
 $(package)_qt_libs=corelib network widgets gui plugins testlib
-$(package)_patches=mac-qmake.conf mingw-uuidof.patch pidlist_absolute.patch fix-xcb-include-order.patch fix_qt_pkgconfig.patch
+$(package)_patches=mac-qmake.conf mingw-uuidof.patch pidlist_absolute.patch fix-xcb-include-order.patch fix_qt_pkgconfig.patch fix-cocoahelpers-macos.patch
 
 $(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
 $(package)_qttranslations_sha256_hash=3a15aebd523c6d89fb97b2d3df866c94149653a26d27a00aac9b6d3020bc5a1d
@@ -140,6 +140,7 @@ define $(package)_preprocess_cmds
   patch -p1 < $($(package)_patch_dir)/pidlist_absolute.patch && \
   patch -p1 < $($(package)_patch_dir)/fix-xcb-include-order.patch && \
   patch -p1 < $($(package)_patch_dir)/fix_qt_pkgconfig.patch && \
+  patch -p1 < $($(package)_patch_dir)/fix-cocoahelpers-macos.patch && \
   echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
   echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
   echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
diff --git a/depends/packages/zeromq.mk b/depends/packages/zeromq.mk
index 01146c2..11033c2 100644
--- a/depends/packages/zeromq.mk
+++ b/depends/packages/zeromq.mk
@@ -1,19 +1,18 @@
 package=zeromq
-$(package)_version=4.1.5
-$(package)_download_path=https://github.com/zeromq/zeromq4-1/releases/download/v$($(package)_version)/
+$(package)_version=4.2.2
+$(package)_download_path=http://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
 $(package)_file_name=$(package)-$($(package)_version).tar.gz
-$(package)_sha256_hash=04aac57f081ffa3a2ee5ed04887be9e205df3a7ddade0027460b8042432bdbcf
-$(package)_patches=9114d3957725acd34aa8b8d011585812f3369411.patch 9e6745c12e0b100cd38acecc16ce7db02905e27c.patch
+$(package)_sha256_hash=5b23f4ca9ef545d5bd3af55d305765e3ee06b986263b31967435d285a3e6df6b
+$(package)_patches=0001-fix-build-with-older-mingw64.patch
 
 define $(package)_set_vars
-  $(package)_config_opts=--without-documentation --disable-shared --without-libsodium --disable-curve
+  $(package)_config_opts=--without-docs --disable-shared --without-libsodium --disable-curve --disable-curve-keygen --disable-perf
   $(package)_config_opts_linux=--with-pic
   $(package)_cxxflags=-std=c++11
 endef
 
 define $(package)_preprocess_cmds
-  patch -p1 < $($(package)_patch_dir)/9114d3957725acd34aa8b8d011585812f3369411.patch && \
-  patch -p1 < $($(package)_patch_dir)/9e6745c12e0b100cd38acecc16ce7db02905e27c.patch && \
+   patch -p1 < $($(package)_patch_dir)/0001-fix-build-with-older-mingw64.patch && \
   ./autogen.sh
 endef
 
@@ -22,7 +21,7 @@ define $(package)_config_cmds
 endef
 
 define $(package)_build_cmds
-  $(MAKE) libzmq.la
+  $(MAKE) src/libzmq.la
 endef
 
 define $(package)_stage_cmds
@@ -30,5 +29,6 @@ define $(package)_stage_cmds
 endef
 
 define $(package)_postprocess_cmds
+  sed -i.old "s/ -lstdc++//" lib/pkgconfig/libzmq.pc && \
   rm -rf bin share
 endef
diff --git a/depends/patches/native_mac_alias/python3.patch b/depends/patches/native_mac_alias/python3.patch
index 1a32340..6f2f553 100644
--- a/depends/patches/native_mac_alias/python3.patch
+++ b/depends/patches/native_mac_alias/python3.patch
@@ -1,7 +1,7 @@
 diff -dur a/mac_alias/alias.py b/mac_alias/alias.py
---- a/mac_alias/alias.py	2015-10-19 12:12:48.000000000 +0200
-+++ b/mac_alias/alias.py	2016-04-03 12:13:12.037159417 +0200
-@@ -243,10 +243,10 @@
+--- a/mac_alias/alias.py
++++ b/mac_alias/alias.py
+@@ -258,10 +258,10 @@
          alias = Alias()
          alias.appinfo = appinfo
              
@@ -14,7 +14,7 @@ diff -dur a/mac_alias/alias.py b/mac_alias/alias.py
                                     folder_cnid, cnid,
                                     crdate, creator_code, type_code)
          alias.target.levels_from = levels_from
-@@ -261,9 +261,9 @@
+@@ -276,9 +276,9 @@
                  b.read(1)
  
              if tag == TAG_CARBON_FOLDER_NAME:
@@ -26,7 +26,7 @@ diff -dur a/mac_alias/alias.py b/mac_alias/alias.py
                                                             value)
              elif tag == TAG_CARBON_PATH:
                  alias.target.carbon_path = value
-@@ -298,9 +298,9 @@
+@@ -313,9 +313,9 @@
                  alias.target.creation_date \
                      = mac_epoch + datetime.timedelta(seconds=seconds)
              elif tag == TAG_POSIX_PATH:
@@ -38,23 +38,7 @@ diff -dur a/mac_alias/alias.py b/mac_alias/alias.py
              elif tag == TAG_RECURSIVE_ALIAS_OF_DISK_IMAGE:
                  alias.volume.disk_image_alias = Alias.from_bytes(value)
              elif tag == TAG_USER_HOME_LENGTH_PREFIX:
-@@ -422,13 +422,13 @@
-         #       (so doing so is ridiculous, and nothing could rely on it).
-         b.write(struct.pack(b'>h28pI2shI64pII4s4shhI2s10s',
-                             self.target.kind,
--                            carbon_volname, voldate,
-+                            carbon_volname, int(voldate),
-                             self.volume.fs_type,
-                             self.volume.disk_type,
-                             self.target.folder_cnid,
-                             carbon_filename,
-                             self.target.cnid,
--                            crdate,
-+                            int(crdate),
-                             self.target.creator_code,
-                             self.target.type_code,
-                             self.target.levels_from,
-@@ -449,12 +449,12 @@
+@@ -467,12 +467,12 @@
  
          b.write(struct.pack(b'>hhQhhQ',
                  TAG_HIGH_RES_VOLUME_CREATION_DATE,
diff --git a/src/bench/Examples.cpp b/src/bench/Examples.cpp
index 314947d..b68c9cd 100644
--- a/src/bench/Examples.cpp
+++ b/src/bench/Examples.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2015-2016 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "validation.h"
-#include "utiltime.h"
+#include <bench/bench.h>
+#include <validation.h>
+#include <utiltime.h>
 
 // Sanity test: this should loop ten times, and
 // min/max/average should be close to 100ms.
@@ -15,7 +15,7 @@ static void Sleep100ms(benchmark::State& state)
     }
 }
 
-BENCHMARK(Sleep100ms);
+BENCHMARK(Sleep100ms, 10);
 
 // Extremely fast-running benchmark:
 #include <math.h>
@@ -31,4 +31,4 @@ static void Trig(benchmark::State& state)
     }
 }
 
-BENCHMARK(Trig);
+BENCHMARK(Trig, 12 * 1000 * 1000);
diff --git a/src/bench/base58.cpp b/src/bench/base58.cpp
index 65e27a6..70bfd7d 100644
--- a/src/bench/base58.cpp
+++ b/src/bench/base58.cpp
@@ -1,11 +1,11 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
+#include <bench/bench.h>
 
-#include "validation.h"
-#include "base58.h"
+#include <validation.h>
+#include <base58.h>
 
 #include <array>
 #include <vector>
@@ -22,7 +22,7 @@ static void Base58Encode(benchmark::State& state)
         }
     };
     while (state.KeepRunning()) {
-        EncodeBase58(buff.begin(), buff.end());
+        EncodeBase58(buff.data(), buff.data() + buff.size());
     }
 }
 
@@ -54,6 +54,6 @@ static void Base58Decode(benchmark::State& state)
 }
 
 
-BENCHMARK(Base58Encode);
-BENCHMARK(Base58CheckEncode);
-BENCHMARK(Base58Decode);
+BENCHMARK(Base58Encode, 470 * 1000);
+BENCHMARK(Base58CheckEncode, 320 * 1000);
+BENCHMARK(Base58Decode, 800 * 1000);
diff --git a/src/bench/bench.cpp b/src/bench/bench.cpp
index 849d924..21329a5 100644
--- a/src/bench/bench.cpp
+++ b/src/bench/bench.cpp
@@ -1,106 +1,146 @@
-// Copyright (c) 2015-2016 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "perf.h"
+#include <bench/bench.h>
+#include <bench/perf.h>
 
 #include <assert.h>
 #include <iostream>
 #include <iomanip>
-#include <sys/time.h>
+#include <algorithm>
+#include <regex>
+#include <numeric>
 
-benchmark::BenchRunner::BenchmarkMap &benchmark::BenchRunner::benchmarks() {
-    static std::map<std::string, benchmark::BenchFunction> benchmarks_map;
-    return benchmarks_map;
+void benchmark::ConsolePrinter::header()
+{
+    std::cout << "# Benchmark, evals, iterations, total, min, max, median" << std::endl;
 }
 
-static double gettimedouble(void) {
-    struct timeval tv;
-    gettimeofday(&tv, nullptr);
-    return tv.tv_usec * 0.000001 + tv.tv_sec;
+void benchmark::ConsolePrinter::result(const State& state)
+{
+    auto results = state.m_elapsed_results;
+    std::sort(results.begin(), results.end());
+
+    double total = state.m_num_iters * std::accumulate(results.begin(), results.end(), 0.0);
+
+    double front = 0;
+    double back = 0;
+    double median = 0;
+
+    if (!results.empty()) {
+        front = results.front();
+        back = results.back();
+
+        size_t mid = results.size() / 2;
+        median = results[mid];
+        if (0 == results.size() % 2) {
+            median = (results[mid] + results[mid + 1]) / 2;
+        }
+    }
+
+    std::cout << std::setprecision(6);
+    std::cout << state.m_name << ", " << state.m_num_evals << ", " << state.m_num_iters << ", " << total << ", " << front << ", " << back << ", " << median << std::endl;
 }
 
-benchmark::BenchRunner::BenchRunner(std::string name, benchmark::BenchFunction func)
+void benchmark::ConsolePrinter::footer() {}
+benchmark::PlotlyPrinter::PlotlyPrinter(std::string plotly_url, int64_t width, int64_t height)
+    : m_plotly_url(plotly_url), m_width(width), m_height(height)
 {
-    benchmarks().insert(std::make_pair(name, func));
 }
 
-void
-benchmark::BenchRunner::RunAll(double elapsedTimeForOne)
+void benchmark::PlotlyPrinter::header()
 {
-    perf_init();
-    std::cout << "#Benchmark" << "," << "count" << "," << "min" << "," << "max" << "," << "average" << ","
-              << "min_cycles" << "," << "max_cycles" << "," << "average_cycles" << "\n";
+    std::cout << "<html><head>"
+              << "<script src=\"" << m_plotly_url << "\"></script>"
+              << "</head><body><div id=\"myDiv\" style=\"width:" << m_width << "px; height:" << m_height << "px\"></div>"
+              << "<script> var data = ["
+              << std::endl;
+}
 
-    for (const auto &p: benchmarks()) {
-        State state(p.first, elapsedTimeForOne);
-        p.second(state);
+void benchmark::PlotlyPrinter::result(const State& state)
+{
+    std::cout << "{ " << std::endl
+              << "  name: '" << state.m_name << "', " << std::endl
+              << "  y: [";
+
+    const char* prefix = "";
+    for (const auto& e : state.m_elapsed_results) {
+        std::cout << prefix << std::setprecision(6) << e;
+        prefix = ", ";
     }
-    perf_fini();
+    std::cout << "]," << std::endl
+              << "  boxpoints: 'all', jitter: 0.3, pointpos: 0, type: 'box',"
+              << std::endl
+              << "}," << std::endl;
 }
 
-bool benchmark::State::KeepRunning()
+void benchmark::PlotlyPrinter::footer()
 {
-    if (count & countMask) {
-      ++count;
-      return true;
-    }
-    double now;
-    uint64_t nowCycles;
-    if (count == 0) {
-        lastTime = beginTime = now = gettimedouble();
-        lastCycles = beginCycles = nowCycles = perf_cpucycles();
+    std::cout << "]; var layout = { showlegend: false, yaxis: { rangemode: 'tozero', autorange: true } };"
+              << "Plotly.newPlot('myDiv', data, layout);"
+              << "</script></body></html>";
+}
+
+
+benchmark::BenchRunner::BenchmarkMap& benchmark::BenchRunner::benchmarks()
+{
+    static std::map<std::string, Bench> benchmarks_map;
+    return benchmarks_map;
+}
+
+benchmark::BenchRunner::BenchRunner(std::string name, benchmark::BenchFunction func, uint64_t num_iters_for_one_second)
+{
+    benchmarks().insert(std::make_pair(name, Bench{func, num_iters_for_one_second}));
+}
+
+void benchmark::BenchRunner::RunAll(Printer& printer, uint64_t num_evals, double scaling, const std::string& filter, bool is_list_only)
+{
+    perf_init();
+    if (!std::ratio_less_equal<benchmark::clock::period, std::micro>::value) {
+        std::cerr << "WARNING: Clock precision is worse than microsecond - benchmarks may be less accurate!\n";
     }
-    else {
-        now = gettimedouble();
-        double elapsed = now - lastTime;
-        double elapsedOne = elapsed * countMaskInv;
-        if (elapsedOne < minTime) minTime = elapsedOne;
-        if (elapsedOne > maxTime) maxTime = elapsedOne;
-
-        // We only use relative values, so don't have to handle 64-bit wrap-around specially
-        nowCycles = perf_cpucycles();
-        uint64_t elapsedOneCycles = (nowCycles - lastCycles) * countMaskInv;
-        if (elapsedOneCycles < minCycles) minCycles = elapsedOneCycles;
-        if (elapsedOneCycles > maxCycles) maxCycles = elapsedOneCycles;
-
-        if (elapsed*128 < maxElapsed) {
-          // If the execution was much too fast (1/128th of maxElapsed), increase the count mask by 8x and restart timing.
-          // The restart avoids including the overhead of this code in the measurement.
-          countMask = ((countMask<<3)|7) & ((1LL<<60)-1);
-          countMaskInv = 1./(countMask+1);
-          count = 0;
-          minTime = std::numeric_limits<double>::max();
-          maxTime = std::numeric_limits<double>::min();
-          minCycles = std::numeric_limits<uint64_t>::max();
-          maxCycles = std::numeric_limits<uint64_t>::min();
-          return true;
+#ifdef DEBUG
+    std::cerr << "WARNING: This is a debug build - may result in slower benchmarks.\n";
+#endif
+
+    std::regex reFilter(filter);
+    std::smatch baseMatch;
+
+    printer.header();
+
+    for (const auto& p : benchmarks()) {
+        if (!std::regex_match(p.first, baseMatch, reFilter)) {
+            continue;
         }
-        if (elapsed*16 < maxElapsed) {
-          uint64_t newCountMask = ((countMask<<1)|1) & ((1LL<<60)-1);
-          if ((count & newCountMask)==0) {
-              countMask = newCountMask;
-              countMaskInv = 1./(countMask+1);
-          }
+
+        uint64_t num_iters = static_cast<uint64_t>(p.second.num_iters_for_one_second * scaling);
+        if (0 == num_iters) {
+            num_iters = 1;
         }
+        State state(p.first, num_evals, num_iters, printer);
+        if (!is_list_only) {
+            p.second.func(state);
+        }
+        printer.result(state);
     }
-    lastTime = now;
-    lastCycles = nowCycles;
-    ++count;
 
-    if (now - beginTime < maxElapsed) return true; // Keep going
+    printer.footer();
 
-    --count;
+    perf_fini();
+}
 
-    assert(count != 0 && "count == 0 => (now == 0 && beginTime == 0) => return above");
+bool benchmark::State::UpdateTimer(const benchmark::time_point current_time)
+{
+    if (m_start_time != time_point()) {
+        std::chrono::duration<double> diff = current_time - m_start_time;
+        m_elapsed_results.push_back(diff.count() / m_num_iters);
 
-    // Output results
-    double average = (now-beginTime)/count;
-    int64_t averageCycles = (nowCycles-beginCycles)/count;
-    std::cout << std::fixed << std::setprecision(15) << name << "," << count << "," << minTime << "," << maxTime << "," << average << ","
-              << minCycles << "," << maxCycles << "," << averageCycles << "\n";
-    std::cout.copyfmt(std::ios(nullptr));
+        if (m_elapsed_results.size() == m_num_evals) {
+            return false;
+        }
+    }
 
-    return false;
+    m_num_iters_left = m_num_iters - 1;
+    return true;
 }
diff --git a/src/bench/bench.h b/src/bench/bench.h
index 01d4966..4364c7e 100644
--- a/src/bench/bench.h
+++ b/src/bench/bench.h
@@ -1,4 +1,4 @@
-// Copyright (c) 2015-2016 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
@@ -9,12 +9,14 @@
 #include <limits>
 #include <map>
 #include <string>
+#include <vector>
+#include <chrono>
 
 #include <boost/preprocessor/cat.hpp>
 #include <boost/preprocessor/stringize.hpp>
 
 // Simple micro-benchmarking framework; API mostly matches a subset of the Google Benchmark
-// framework (see https://github.com/google/benchmark)
+// framework (see http://github.com/google/benchmark)
 // Why not use the Google Benchmark framework? Because adding Yet Another Dependency
 // (that uses cmake as its build system and has lots of features we don't need) isn't
 // worth it.
@@ -31,51 +33,110 @@ static void CODE_TO_TIME(benchmark::State& state)
     ... do any cleanup needed...
 }
 
-BENCHMARK(CODE_TO_TIME);
+// default to running benchmark for 5000 iterations
+BENCHMARK(CODE_TO_TIME, 5000);
 
  */
- 
+
 namespace benchmark {
+// In case high_resolution_clock is steady, prefer that, otherwise use steady_clock.
+struct best_clock {
+    using hi_res_clock = std::chrono::high_resolution_clock;
+    using steady_clock = std::chrono::steady_clock;
+    using type = std::conditional<hi_res_clock::is_steady, hi_res_clock, steady_clock>::type;
+};
+using clock = best_clock::type;
+using time_point = clock::time_point;
+using duration = clock::duration;
 
-    class State {
-        std::string name;
-        double maxElapsed;
-        double beginTime;
-        double lastTime, minTime, maxTime, countMaskInv;
-        uint64_t count;
-        uint64_t countMask;
-        uint64_t beginCycles;
-        uint64_t lastCycles;
-        uint64_t minCycles;
-        uint64_t maxCycles;
-    public:
-        State(std::string _name, double _maxElapsed) : name(_name), maxElapsed(_maxElapsed), count(0) {
-            minTime = std::numeric_limits<double>::max();
-            maxTime = std::numeric_limits<double>::min();
-            minCycles = std::numeric_limits<uint64_t>::max();
-            maxCycles = std::numeric_limits<uint64_t>::min();
-            countMask = 1;
-            countMaskInv = 1./(countMask + 1);
-        }
-        bool KeepRunning();
-    };
+class Printer;
 
-    typedef std::function<void(State&)> BenchFunction;
+class State
+{
+public:
+    std::string m_name;
+    uint64_t m_num_iters_left;
+    const uint64_t m_num_iters;
+    const uint64_t m_num_evals;
+    std::vector<double> m_elapsed_results;
+    time_point m_start_time;
+
+    bool UpdateTimer(time_point finish_time);
 
-    class BenchRunner
+    State(std::string name, uint64_t num_evals, double num_iters, Printer& printer) : m_name(name), m_num_iters_left(0), m_num_iters(num_iters), m_num_evals(num_evals)
     {
-        typedef std::map<std::string, BenchFunction> BenchmarkMap;
-        static BenchmarkMap &benchmarks();
+    }
+
+    inline bool KeepRunning()
+    {
+        if (m_num_iters_left--) {
+            return true;
+        }
+
+        bool result = UpdateTimer(clock::now());
+        // measure again so runtime of UpdateTimer is not included
+        m_start_time = clock::now();
+        return result;
+    }
+};
 
-    public:
-        BenchRunner(std::string name, BenchFunction func);
+typedef std::function<void(State&)> BenchFunction;
 
-        static void RunAll(double elapsedTimeForOne=1.0);
+class BenchRunner
+{
+    struct Bench {
+        BenchFunction func;
+        uint64_t num_iters_for_one_second;
     };
+    typedef std::map<std::string, Bench> BenchmarkMap;
+    static BenchmarkMap& benchmarks();
+
+public:
+    BenchRunner(std::string name, BenchFunction func, uint64_t num_iters_for_one_second);
+
+    static void RunAll(Printer& printer, uint64_t num_evals, double scaling, const std::string& filter, bool is_list_only);
+};
+
+// interface to output benchmark results.
+class Printer
+{
+public:
+    virtual ~Printer() {}
+    virtual void header() = 0;
+    virtual void result(const State& state) = 0;
+    virtual void footer() = 0;
+};
+
+// default printer to console, shows min, max, median.
+class ConsolePrinter : public Printer
+{
+public:
+    void header();
+    void result(const State& state);
+    void footer();
+};
+
+// creates box plot with plotly.js
+class PlotlyPrinter : public Printer
+{
+public:
+    PlotlyPrinter(std::string plotly_url, int64_t width, int64_t height);
+    void header();
+    void result(const State& state);
+    void footer();
+
+private:
+    std::string m_plotly_url;
+    int64_t m_width;
+    int64_t m_height;
+};
 }
 
-// BENCHMARK(foo) expands to:  benchmark::BenchRunner bench_11foo("foo", foo);
-#define BENCHMARK(n) \
-    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n);
+
+// BENCHMARK(foo, num_iters_for_one_second) expands to:  benchmark::BenchRunner bench_11foo("foo", num_iterations);
+// Choose a num_iters_for_one_second that takes roughly 1 second. The goal is that all benchmarks should take approximately
+// the same time, and scaling factor can be used that the total time is appropriate for your system.
+#define BENCHMARK(n, num_iters_for_one_second) \
+    benchmark::BenchRunner BOOST_PP_CAT(bench_, BOOST_PP_CAT(__LINE__, n))(BOOST_PP_STRINGIZE(n), n, (num_iters_for_one_second));
 
 #endif // FABCOIN_BENCH_BENCH_H
diff --git a/src/bench/bench_fabcoin.cpp b/src/bench/bench_fabcoin.cpp
index 126adbc..0a0bb79 100644
--- a/src/bench/bench_fabcoin.cpp
+++ b/src/bench/bench_fabcoin.cpp
@@ -1,27 +1,71 @@
-// Copyright (c) 2015-2016 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
+#include <bench/bench.h>
 
-#include "crypto/sha256.h"
-#include "key.h"
-#include "validation.h"
-#include "util.h"
-#include "random.h"
-#include "chainparams.h"
+#include <crypto/sha256.h>
+#include <key.h>
+#include <validation.h>
+#include <util.h>
+#include <random.h>
+
+#include <boost/lexical_cast.hpp>
+
+#include <memory>
+
+static const int64_t DEFAULT_BENCH_EVALUATIONS = 5;
+static const char* DEFAULT_BENCH_FILTER = ".*";
+static const char* DEFAULT_BENCH_SCALING = "1.0";
+static const char* DEFAULT_BENCH_PRINTER = "console";
+static const char* DEFAULT_PLOT_PLOTLYURL = "http://cdn.plot.ly/plotly-latest.min.js";
+static const int64_t DEFAULT_PLOT_WIDTH = 1024;
+static const int64_t DEFAULT_PLOT_HEIGHT = 768;
 
 int
 main(int argc, char** argv)
 {
+    gArgs.ParseParameters(argc, argv);
+
+    if (gArgs.IsArgSet("-?") || gArgs.IsArgSet("-h") || gArgs.IsArgSet("-help")) {
+        std::cout << HelpMessageGroup(_("Options:"))
+                  << HelpMessageOpt("-?", _("Print this help message and exit"))
+                  << HelpMessageOpt("-list", _("List benchmarks without executing them. Can be combined with -scaling and -filter"))
+                  << HelpMessageOpt("-evals=<n>", strprintf(_("Number of measurement evaluations to perform. (default: %u)"), DEFAULT_BENCH_EVALUATIONS))
+                  << HelpMessageOpt("-filter=<regex>", strprintf(_("Regular expression filter to select benchmark by name (default: %s)"), DEFAULT_BENCH_FILTER))
+                  << HelpMessageOpt("-scaling=<n>", strprintf(_("Scaling factor for benchmark's runtime (default: %u)"), DEFAULT_BENCH_SCALING))
+                  << HelpMessageOpt("-printer=(console|plot)", strprintf(_("Choose printer format. console: print data to console. plot: Print results as HTML graph (default: %s)"), DEFAULT_BENCH_PRINTER))
+                  << HelpMessageOpt("-plot-plotlyurl=<uri>", strprintf(_("URL to use for plotly.js (default: %s)"), DEFAULT_PLOT_PLOTLYURL))
+                  << HelpMessageOpt("-plot-width=<x>", strprintf(_("Plot width in pixel (default: %u)"), DEFAULT_PLOT_WIDTH))
+                  << HelpMessageOpt("-plot-height=<x>", strprintf(_("Plot height in pixel (default: %u)"), DEFAULT_PLOT_HEIGHT));
+
+        return 0;
+    }
+
     SHA256AutoDetect();
     RandomInit();
     ECC_Start();
     SetupEnvironment();
     fPrintToDebugLog = false; // don't want to write to debug.log file
 
-    SelectParams("unittest");
-    benchmark::BenchRunner::RunAll();
+    int64_t evaluations = gArgs.GetArg("-evals", DEFAULT_BENCH_EVALUATIONS);
+    std::string regex_filter = gArgs.GetArg("-filter", DEFAULT_BENCH_FILTER);
+    std::string scaling_str = gArgs.GetArg("-scaling", DEFAULT_BENCH_SCALING);
+    bool is_list_only = gArgs.GetBoolArg("-list", false);
+
+    double scaling_factor = boost::lexical_cast<double>(scaling_str);
+
+
+    std::unique_ptr<benchmark::Printer> printer(new benchmark::ConsolePrinter());
+    std::string printer_arg = gArgs.GetArg("-printer", DEFAULT_BENCH_PRINTER);
+    if ("plot" == printer_arg) {
+        printer.reset(new benchmark::PlotlyPrinter(
+            gArgs.GetArg("-plot-plotlyurl", DEFAULT_PLOT_PLOTLYURL),
+            gArgs.GetArg("-plot-width", DEFAULT_PLOT_WIDTH),
+            gArgs.GetArg("-plot-height", DEFAULT_PLOT_HEIGHT)));
+    }
+
+    benchmark::BenchRunner::RunAll(*printer, evaluations, scaling_factor, regex_filter, is_list_only);
 
     ECC_Stop();
 }
diff --git a/src/bench/ccoins_caching.cpp b/src/bench/ccoins_caching.cpp
index 1bf42b7..7aea901 100644
--- a/src/bench/ccoins_caching.cpp
+++ b/src/bench/ccoins_caching.cpp
@@ -1,11 +1,11 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "coins.h"
-#include "policy/policy.h"
-#include "wallet/crypter.h"
+#include <bench/bench.h>
+#include <coins.h>
+#include <policy/policy.h>
+#include <wallet/crypter.h>
 
 #include <vector>
 
@@ -52,7 +52,7 @@ SetupDummyInputs(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsRet)
 // many times micro-benchmarks of the database showed completely different
 // characteristics than e.g. reindex timings. But that's not a requirement of
 // every benchmark."
-// (https://github.com/blockchaingate/fabcoin/issues/7883#issuecomment-224807484)
+// (http://github.com/blockchaingate/fabcoin/issues/7883#issuecomment-224807484)
 static void CCoinsCaching(benchmark::State& state)
 {
     CBasicKeyStore keystore;
@@ -84,4 +84,4 @@ static void CCoinsCaching(benchmark::State& state)
     }
 }
 
-BENCHMARK(CCoinsCaching);
+BENCHMARK(CCoinsCaching, 170 * 1000);
diff --git a/src/bench/checkqueue.cpp b/src/bench/checkqueue.cpp
index 88a2a57..6e816f1 100644
--- a/src/bench/checkqueue.cpp
+++ b/src/bench/checkqueue.cpp
@@ -1,62 +1,22 @@
-// Copyright (c) 2015 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "util.h"
-#include "validation.h"
-#include "checkqueue.h"
-#include "prevector.h"
+#include <bench/bench.h>
+#include <util.h>
+#include <validation.h>
+#include <checkqueue.h>
+#include <prevector.h>
 #include <vector>
 #include <boost/thread/thread.hpp>
-#include "random.h"
+#include <random.h>
 
 
-// This Benchmark tests the CheckQueue with the lightest
-// weight Checks, so it should make any lock contention
-// particularly visible
 static const int MIN_CORES = 2;
 static const size_t BATCHES = 101;
 static const size_t BATCH_SIZE = 30;
 static const int PREVECTOR_SIZE = 28;
-static const int QUEUE_BATCH_SIZE = 128;
-static void CCheckQueueSpeed(benchmark::State& state)
-{
-    struct FakeJobNoWork {
-        bool operator()()
-        {
-            return true;
-        }
-        void swap(FakeJobNoWork& x){};
-    };
-    CCheckQueue<FakeJobNoWork> queue {QUEUE_BATCH_SIZE};
-    boost::thread_group tg;
-    for (auto x = 0; x < std::max(MIN_CORES, GetNumCores()); ++x) {
-       tg.create_thread([&]{queue.Thread();});
-    }
-    while (state.KeepRunning()) {
-        CCheckQueueControl<FakeJobNoWork> control(&queue);
-
-        // We call Add a number of times to simulate the behavior of adding
-        // a block of transactions at once.
-
-        std::vector<std::vector<FakeJobNoWork>> vBatches(BATCHES);
-        for (auto& vChecks : vBatches) {
-            vChecks.resize(BATCH_SIZE);
-        }
-        for (auto& vChecks : vBatches) {
-            // We can't make vChecks in the inner loop because we want to measure
-            // the cost of getting the memory to each thread and we might get the same
-            // memory
-            control.Add(vChecks);
-        }
-        // control waits for completion by RAII, but
-        // it is done explicitly here for clarity
-        control.Wait();
-    }
-    tg.interrupt_all();
-    tg.join_all();
-}
+static const unsigned int QUEUE_BATCH_SIZE = 128;
 
 // This Benchmark tests the CheckQueue with a slightly realistic workload,
 // where checks all contain a prevector that is indirect 50% of the time
@@ -67,7 +27,7 @@ static void CCheckQueueSpeedPrevectorJob(benchmark::State& state)
         prevector<PREVECTOR_SIZE, uint8_t> p;
         PrevectorJob(){
         }
-        PrevectorJob(FastRandomContext& insecure_rand){
+        explicit PrevectorJob(FastRandomContext& insecure_rand){
             p.resize(insecure_rand.randrange(PREVECTOR_SIZE*2));
         }
         bool operator()()
@@ -99,5 +59,4 @@ static void CCheckQueueSpeedPrevectorJob(benchmark::State& state)
     tg.interrupt_all();
     tg.join_all();
 }
-BENCHMARK(CCheckQueueSpeed);
-BENCHMARK(CCheckQueueSpeedPrevectorJob);
+BENCHMARK(CCheckQueueSpeedPrevectorJob, 1400);
diff --git a/src/bench/coin_selection.cpp b/src/bench/coin_selection.cpp
index ea5ac52..3ea9bc8 100644
--- a/src/bench/coin_selection.cpp
+++ b/src/bench/coin_selection.cpp
@@ -1,9 +1,9 @@
-// Copyright (c) 2012-2016 The Bitcoin Core developers
+// Copyright (c) 2012-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "wallet/wallet.h"
+#include <bench/bench.h>
+#include <wallet/wallet.h>
 
 #include <set>
 
@@ -29,7 +29,7 @@ static void addCoin(const CAmount& nValue, const CWallet& wallet, std::vector<CO
 // the hardest, as you need a wider selection of scenarios, just testing the
 // same one over and over isn't too useful. Generating random isn't useful
 // either for measurements."
-// (https://github.com/blockchaingate/fabcoin/issues/7883#issuecomment-224807484)
+// (http://github.com/blockchaingate/fabcoin/issues/7883#issuecomment-224807484)
 static void CoinSelection(benchmark::State& state)
 {
     const CWallet wallet;
@@ -56,4 +56,4 @@ static void CoinSelection(benchmark::State& state)
     }
 }
 
-BENCHMARK(CoinSelection);
+BENCHMARK(CoinSelection, 650);
diff --git a/src/bench/crypto_hash.cpp b/src/bench/crypto_hash.cpp
index 2914a36..adb69bc 100644
--- a/src/bench/crypto_hash.cpp
+++ b/src/bench/crypto_hash.cpp
@@ -1,19 +1,19 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
 #include <iostream>
 
-#include "bench.h"
-#include "bloom.h"
-#include "hash.h"
-#include "random.h"
-#include "uint256.h"
-#include "utiltime.h"
-#include "crypto/ripemd160.h"
-#include "crypto/sha1.h"
-#include "crypto/sha256.h"
-#include "crypto/sha512.h"
+#include <bench/bench.h>
+#include <bloom.h>
+#include <hash.h>
+#include <random.h>
+#include <uint256.h>
+#include <utiltime.h>
+#include <crypto/ripemd160.h>
+#include <crypto/sha1.h>
+#include <crypto/sha256.h>
+#include <crypto/sha512.h>
 
 /* Number of bytes to hash per iteration */
 static const uint64_t BUFFER_SIZE = 1000*1000;
@@ -46,9 +46,9 @@ static void SHA256_32b(benchmark::State& state)
 {
     std::vector<uint8_t> in(32,0);
     while (state.KeepRunning()) {
-        for (int i = 0; i < 1000000; i++) {
-            CSHA256().Write(in.data(), in.size()).Finalize(&in[0]);
-        }
+        CSHA256()
+            .Write(in.data(), in.size())
+            .Finalize(in.data());
     }
 }
 
@@ -63,10 +63,9 @@ static void SHA512(benchmark::State& state)
 static void SipHash_32b(benchmark::State& state)
 {
     uint256 x;
+    uint64_t k1 = 0;
     while (state.KeepRunning()) {
-        for (int i = 0; i < 1000000; i++) {
-            *((uint64_t*)x.begin()) = SipHashUint256(0, i, x);
-        }
+        *((uint64_t*)x.begin()) = SipHashUint256(0, ++k1, x);
     }
 }
 
@@ -75,9 +74,7 @@ static void FastRandom_32bit(benchmark::State& state)
     FastRandomContext rng(true);
     uint32_t x = 0;
     while (state.KeepRunning()) {
-        for (int i = 0; i < 1000000; i++) {
-            x += rng.rand32();
-        }
+        x += rng.rand32();
     }
 }
 
@@ -86,18 +83,16 @@ static void FastRandom_1bit(benchmark::State& state)
     FastRandomContext rng(true);
     uint32_t x = 0;
     while (state.KeepRunning()) {
-        for (int i = 0; i < 1000000; i++) {
-            x += rng.randbool();
-        }
+        x += rng.randbool();
     }
 }
 
-BENCHMARK(RIPEMD160);
-BENCHMARK(SHA1);
-BENCHMARK(SHA256);
-BENCHMARK(SHA512);
+BENCHMARK(RIPEMD160, 440);
+BENCHMARK(SHA1, 570);
+BENCHMARK(SHA256, 340);
+BENCHMARK(SHA512, 330);
 
-BENCHMARK(SHA256_32b);
-BENCHMARK(SipHash_32b);
-BENCHMARK(FastRandom_32bit);
-BENCHMARK(FastRandom_1bit);
+BENCHMARK(SHA256_32b, 4700 * 1000);
+BENCHMARK(SipHash_32b, 40 * 1000 * 1000);
+BENCHMARK(FastRandom_32bit, 110 * 1000 * 1000);
+BENCHMARK(FastRandom_1bit, 440 * 1000 * 1000);
diff --git a/src/bench/lockedpool.cpp b/src/bench/lockedpool.cpp
index 43a1422..ca30d81 100644
--- a/src/bench/lockedpool.cpp
+++ b/src/bench/lockedpool.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
+#include <bench/bench.h>
 
-#include "support/lockedpool.h"
+#include <support/lockedpool.h>
 
 #include <iostream>
 #include <vector>
@@ -21,14 +21,14 @@ static void BenchLockedPool(benchmark::State& state)
 
     std::vector<void*> addr;
     for (int x=0; x<ASIZE; ++x)
-        addr.push_back(0);
+        addr.push_back(nullptr);
     uint32_t s = 0x12345678;
     while (state.KeepRunning()) {
         for (int x=0; x<BITER; ++x) {
             int idx = s & (addr.size()-1);
             if (s & 0x80000000) {
                 b.free(addr[idx]);
-                addr[idx] = 0;
+                addr[idx] = nullptr;
             } else if(!addr[idx]) {
                 addr[idx] = b.alloc((s >> 16) & (MSIZE-1));
             }
@@ -43,5 +43,4 @@ static void BenchLockedPool(benchmark::State& state)
     addr.clear();
 }
 
-BENCHMARK(BenchLockedPool);
-
+BENCHMARK(BenchLockedPool, 530);
diff --git a/src/bench/mempool_eviction.cpp b/src/bench/mempool_eviction.cpp
index ffeb136..cdda0bd 100644
--- a/src/bench/mempool_eviction.cpp
+++ b/src/bench/mempool_eviction.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2011-2016 The Bitcoin Core developers
+// Copyright (c) 2011-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "policy/policy.h"
-#include "txmempool.h"
+#include <bench/bench.h>
+#include <policy/policy.h>
+#include <txmempool.h>
 
 #include <list>
 #include <vector>
@@ -16,9 +16,6 @@ static void AddTx(const CTransaction& tx, const CAmount& nFee, CTxMemPool& pool)
     bool spendsCoinbase = false;
     unsigned int sigOpCost = 4;
     LockPoints lp;
-    CAmount inChainInputValue = 0.0;
-    double dPriority = .0;
-    dev::u256 txMinGasPrice = 0;
     pool.addUnchecked(tx.GetHash(), CTxMemPoolEntry(
                                         MakeTransactionRef(tx), nFee, nTime, nHeight,
                                         spendsCoinbase, sigOpCost, lp));
@@ -114,4 +111,4 @@ static void MempoolEviction(benchmark::State& state)
     }
 }
 
-BENCHMARK(MempoolEviction);
+BENCHMARK(MempoolEviction, 41000);
diff --git a/src/bench/perf.cpp b/src/bench/perf.cpp
index a549ec2..f92d08c 100644
--- a/src/bench/perf.cpp
+++ b/src/bench/perf.cpp
@@ -1,8 +1,8 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "perf.h"
+#include <bench/perf.h>
 
 #if defined(__i386__) || defined(__x86_64__)
 
diff --git a/src/bench/prevector_destructor.cpp b/src/bench/prevector_destructor.cpp
index 55af3de..39d0ee5 100644
--- a/src/bench/prevector_destructor.cpp
+++ b/src/bench/prevector_destructor.cpp
@@ -2,8 +2,8 @@
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "prevector.h"
+#include <bench/bench.h>
+#include <prevector.h>
 
 static void PrevectorDestructor(benchmark::State& state)
 {
@@ -32,5 +32,5 @@ static void PrevectorClear(benchmark::State& state)
     }
 }
 
-BENCHMARK(PrevectorDestructor);
-BENCHMARK(PrevectorClear);
+BENCHMARK(PrevectorDestructor, 5700);
+BENCHMARK(PrevectorClear, 5600);
diff --git a/src/bench/rollingbloom.cpp b/src/bench/rollingbloom.cpp
index 73c02cf..f7f7260 100644
--- a/src/bench/rollingbloom.cpp
+++ b/src/bench/rollingbloom.cpp
@@ -1,20 +1,17 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
 #include <iostream>
 
-#include "bench.h"
-#include "bloom.h"
-#include "utiltime.h"
+#include <bench/bench.h>
+#include <bloom.h>
 
 static void RollingBloom(benchmark::State& state)
 {
     CRollingBloomFilter filter(120000, 0.000001);
     std::vector<unsigned char> data(32);
     uint32_t count = 0;
-    uint32_t nEntriesPerGeneration = (120000 + 1) / 2;
-    uint32_t countnow = 0;
     uint64_t match = 0;
     while (state.KeepRunning()) {
         count++;
@@ -22,16 +19,8 @@ static void RollingBloom(benchmark::State& state)
         data[1] = count >> 8;
         data[2] = count >> 16;
         data[3] = count >> 24;
-        if (countnow == nEntriesPerGeneration) {
-            int64_t b = GetTimeMicros();
-            filter.insert(data);
-            int64_t e = GetTimeMicros();
-            std::cout << "RollingBloom-refresh,1," << (e-b)*0.000001 << "," << (e-b)*0.000001 << "," << (e-b)*0.000001 << "\n";
-            countnow = 0;
-        } else {
-            filter.insert(data);
-        }
-        countnow++;
+        filter.insert(data);
+
         data[0] = count >> 24;
         data[1] = count >> 16;
         data[2] = count >> 8;
@@ -40,4 +29,4 @@ static void RollingBloom(benchmark::State& state)
     }
 }
 
-BENCHMARK(RollingBloom);
+BENCHMARK(RollingBloom, 1500 * 1000);
diff --git a/src/bench/verify_script.cpp b/src/bench/verify_script.cpp
index 5ef0f28..6921f1e 100644
--- a/src/bench/verify_script.cpp
+++ b/src/bench/verify_script.cpp
@@ -1,15 +1,15 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "bench.h"
-#include "key.h"
+#include <bench/bench.h>
+#include <key.h>
 #if defined(HAVE_CONSENSUS_LIB)
-#include "script/fabcoinconsensus.h"
+#include <script/fabcoinconsensus.h>
 #endif
-#include "script/script.h"
-#include "script/sign.h"
-#include "streams.h"
+#include <script/script.h>
+#include <script/sign.h>
+#include <streams.h>
 
 #include <array>
 
@@ -105,4 +105,4 @@ static void VerifyScriptBench(benchmark::State& state)
     }
 }
 
-BENCHMARK(VerifyScriptBench);
+BENCHMARK(VerifyScriptBench, 6300);
diff --git a/src/crypto/aes.cpp b/src/crypto/aes.cpp
index 5e70d25..bf7a252 100644
--- a/src/crypto/aes.cpp
+++ b/src/crypto/aes.cpp
@@ -1,15 +1,15 @@
-// Copyright (c) 2016 The Bitcoin Core developers
+// Copyright (c) 2016-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "aes.h"
-#include "crypto/common.h"
+#include <crypto/aes.h>
+#include <crypto/common.h>
 
 #include <assert.h>
 #include <string.h>
 
 extern "C" {
-#include "crypto/ctaes/ctaes.c"
+#include <crypto/ctaes/ctaes.c>
 }
 
 AES128Encrypt::AES128Encrypt(const unsigned char key[16])
diff --git a/src/crypto/aes.h b/src/crypto/aes.h
index dd12312..2164dfa 100644
--- a/src/crypto/aes.h
+++ b/src/crypto/aes.h
@@ -1,4 +1,4 @@
-// Copyright (c) 2015-2016 The Bitcoin Core developers
+// Copyright (c) 2015-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 //
@@ -8,7 +8,7 @@
 #define FABCOIN_CRYPTO_AES_H
 
 extern "C" {
-#include "crypto/ctaes/ctaes.h"
+#include <crypto/ctaes/ctaes.h>
 }
 
 static const int AES_BLOCKSIZE = 16;
@@ -22,7 +22,7 @@ private:
     AES128_ctx ctx;
 
 public:
-    AES128Encrypt(const unsigned char key[16]);
+    explicit AES128Encrypt(const unsigned char key[16]);
     ~AES128Encrypt();
     void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const;
 };
@@ -34,7 +34,7 @@ private:
     AES128_ctx ctx;
 
 public:
-    AES128Decrypt(const unsigned char key[16]);
+    explicit AES128Decrypt(const unsigned char key[16]);
     ~AES128Decrypt();
     void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const;
 };
@@ -46,7 +46,7 @@ private:
     AES256_ctx ctx;
 
 public:
-    AES256Encrypt(const unsigned char key[32]);
+    explicit AES256Encrypt(const unsigned char key[32]);
     ~AES256Encrypt();
     void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const;
 };
@@ -58,7 +58,7 @@ private:
     AES256_ctx ctx;
 
 public:
-    AES256Decrypt(const unsigned char key[32]);
+    explicit AES256Decrypt(const unsigned char key[32]);
     ~AES256Decrypt();
     void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const;
 };
diff --git a/src/crypto/chacha20.cpp b/src/crypto/chacha20.cpp
index 4038ae9..ac4470f 100644
--- a/src/crypto/chacha20.cpp
+++ b/src/crypto/chacha20.cpp
@@ -5,8 +5,8 @@
 // Based on the public domain implementation 'merged' by D. J. Bernstein
 // See https://cr.yp.to/chacha.html.
 
-#include "crypto/common.h"
-#include "crypto/chacha20.h"
+#include <crypto/common.h>
+#include <crypto/chacha20.h>
 
 #include <string.h>
 
diff --git a/src/crypto/common.h b/src/crypto/common.h
index 209b619..986899f 100644
--- a/src/crypto/common.h
+++ b/src/crypto/common.h
@@ -1,4 +1,4 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
@@ -6,13 +6,13 @@
 #define FABCOIN_CRYPTO_COMMON_H
 
 #if defined(HAVE_CONFIG_H)
-#include "fabcoin-config.h"
+#include <config/fabcoin-config.h>
 #endif
 
 #include <stdint.h>
 #include <string.h>
 
-#include "compat/endian.h"
+#include <compat/endian.h>
 
 uint16_t static inline ReadLE16(const unsigned char* ptr)
 {
diff --git a/src/crypto/hmac_sha256.cpp b/src/crypto/hmac_sha256.cpp
index 3c79162..d4afe14 100644
--- a/src/crypto/hmac_sha256.cpp
+++ b/src/crypto/hmac_sha256.cpp
@@ -1,8 +1,8 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/hmac_sha256.h"
+#include <crypto/hmac_sha256.h>
 
 #include <string.h>
 
diff --git a/src/crypto/hmac_sha256.h b/src/crypto/hmac_sha256.h
index 53dbba4..5fa2bc3 100644
--- a/src/crypto/hmac_sha256.h
+++ b/src/crypto/hmac_sha256.h
@@ -1,16 +1,16 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
 #ifndef FABCOIN_CRYPTO_HMAC_SHA256_H
 #define FABCOIN_CRYPTO_HMAC_SHA256_H
 
-#include "crypto/sha256.h"
+#include <crypto/sha256.h>
 
 #include <stdint.h>
 #include <stdlib.h>
 
-/** A hasher class for HMAC-SHA-512. */
+/** A hasher class for HMAC-SHA-256. */
 class CHMAC_SHA256
 {
 private:
diff --git a/src/crypto/hmac_sha512.cpp b/src/crypto/hmac_sha512.cpp
index 5939c6e..d9c4d04 100644
--- a/src/crypto/hmac_sha512.cpp
+++ b/src/crypto/hmac_sha512.cpp
@@ -1,8 +1,8 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/hmac_sha512.h"
+#include <crypto/hmac_sha512.h>
 
 #include <string.h>
 
diff --git a/src/crypto/hmac_sha512.h b/src/crypto/hmac_sha512.h
index 16d206a..121d6c8 100644
--- a/src/crypto/hmac_sha512.h
+++ b/src/crypto/hmac_sha512.h
@@ -1,11 +1,11 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
 #ifndef FABCOIN_CRYPTO_HMAC_SHA512_H
 #define FABCOIN_CRYPTO_HMAC_SHA512_H
 
-#include "crypto/sha512.h"
+#include <crypto/sha512.h>
 
 #include <stdint.h>
 #include <stdlib.h>
diff --git a/src/crypto/ripemd160.cpp b/src/crypto/ripemd160.cpp
index 77c9acf..51468ec 100644
--- a/src/crypto/ripemd160.cpp
+++ b/src/crypto/ripemd160.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/ripemd160.h"
+#include <crypto/ripemd160.h>
 
-#include "crypto/common.h"
+#include <crypto/common.h>
 
 #include <string.h>
 
diff --git a/src/crypto/sha1.cpp b/src/crypto/sha1.cpp
index 0b895b3..dc96ac5 100644
--- a/src/crypto/sha1.cpp
+++ b/src/crypto/sha1.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/sha1.h"
+#include <crypto/sha1.h>
 
-#include "crypto/common.h"
+#include <crypto/common.h>
 
 #include <string.h>
 
diff --git a/src/crypto/sha256.cpp b/src/crypto/sha256.cpp
index 15d6db9..f3245b8 100644
--- a/src/crypto/sha256.cpp
+++ b/src/crypto/sha256.cpp
@@ -1,16 +1,16 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/sha256.h"
-#include "crypto/common.h"
+#include <crypto/sha256.h>
+#include <crypto/common.h>
 
 #include <assert.h>
 #include <string.h>
 #include <atomic>
 
 #if defined(__x86_64__) || defined(__amd64__)
-#if defined(EXPERIMENTAL_ASM)
+#if defined(USE_ASM)
 #include <cpuid.h>
 namespace sha256_sse4
 {
@@ -178,7 +178,7 @@ TransformType Transform = sha256::Transform;
 
 std::string SHA256AutoDetect()
 {
-#if defined(EXPERIMENTAL_ASM) && (defined(__x86_64__) || defined(__amd64__))
+#if defined(USE_ASM) && (defined(__x86_64__) || defined(__amd64__))
     uint32_t eax, ebx, ecx, edx;
     if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) && (ecx >> 19) & 1) {
         Transform = sha256_sse4::Transform;
diff --git a/src/crypto/sha256.h b/src/crypto/sha256.h
index cddb8a1..31eefe8 100644
--- a/src/crypto/sha256.h
+++ b/src/crypto/sha256.h
@@ -1,4 +1,4 @@
-// Copyright (c) 2014-2016 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
diff --git a/src/crypto/sha512.cpp b/src/crypto/sha512.cpp
index 564127c..dff4d8d 100644
--- a/src/crypto/sha512.cpp
+++ b/src/crypto/sha512.cpp
@@ -1,10 +1,10 @@
-// Copyright (c) 2014 The Bitcoin Core developers
+// Copyright (c) 2014-2017 The Bitcoin Core developers
 // Distributed under the MIT software license, see the accompanying
 // file COPYING or http://www.opensource.org/licenses/mit-license.php.
 
-#include "crypto/sha512.h"
+#include <crypto/sha512.h>
 
-#include "crypto/common.h"
+#include <crypto/common.h>
 
 #include <string.h>
 
diff --git a/src/leveldb/db/db_impl.cc b/src/leveldb/db/db_impl.cc
index f43ad76..3bb58e5 100644
--- a/src/leveldb/db/db_impl.cc
+++ b/src/leveldb/db/db_impl.cc
@@ -414,7 +414,7 @@ Status DBImpl::RecoverLogFile(uint64_t log_number, bool last_log,
          status.ok()) {
     if (record.size() < 12) {
       reporter.Corruption(
-          record.size(), Status::Corruption("log record too small"));
+          record.size(), Status::Corruption("log record too small", fname));
       continue;
     }
     WriteBatchInternal::SetContents(&batch, record);
diff --git a/src/leveldb/db/leveldbutil.cc b/src/leveldb/db/leveldbutil.cc
index 9f4b7dd..d06d64d 100644
--- a/src/leveldb/db/leveldbutil.cc
+++ b/src/leveldb/db/leveldbutil.cc
@@ -19,6 +19,7 @@ class StdoutPrinter : public WritableFile {
   virtual Status Close() { return Status::OK(); }
   virtual Status Flush() { return Status::OK(); }
   virtual Status Sync() { return Status::OK(); }
+  virtual std::string GetName() const { return "[stdout]"; }
 };
 
 bool HandleDumpCommand(Env* env, char** files, int num) {
diff --git a/src/leveldb/db/log_reader.cc b/src/leveldb/db/log_reader.cc
index a6d3045..8b6ad13 100644
--- a/src/leveldb/db/log_reader.cc
+++ b/src/leveldb/db/log_reader.cc
@@ -186,7 +186,7 @@ uint64_t Reader::LastRecordOffset() {
 }
 
 void Reader::ReportCorruption(uint64_t bytes, const char* reason) {
-  ReportDrop(bytes, Status::Corruption(reason));
+  ReportDrop(bytes, Status::Corruption(reason, file_->GetName()));
 }
 
 void Reader::ReportDrop(uint64_t bytes, const Status& reason) {
diff --git a/src/leveldb/db/repair.cc b/src/leveldb/db/repair.cc
index 4cd4bb0..7281e3d 100644
--- a/src/leveldb/db/repair.cc
+++ b/src/leveldb/db/repair.cc
@@ -203,7 +203,7 @@ class Repairer {
     while (reader.ReadRecord(&record, &scratch)) {
       if (record.size() < 12) {
         reporter.Corruption(
-            record.size(), Status::Corruption("log record too small"));
+            record.size(), Status::Corruption("log record too small", logname));
         continue;
       }
       WriteBatchInternal::SetContents(&batch, record);
diff --git a/src/leveldb/helpers/memenv/memenv.cc b/src/leveldb/helpers/memenv/memenv.cc
index 9a98884..68c0614 100644
--- a/src/leveldb/helpers/memenv/memenv.cc
+++ b/src/leveldb/helpers/memenv/memenv.cc
@@ -176,6 +176,7 @@ class SequentialFileImpl : public SequentialFile {
     return Status::OK();
   }
 
+  virtual std::string GetName() const { return "[memenv]"; }
  private:
   FileState* file_;
   uint64_t pos_;
@@ -196,6 +197,7 @@ class RandomAccessFileImpl : public RandomAccessFile {
     return file_->Read(offset, n, result, scratch);
   }
 
+  virtual std::string GetName() const { return "[memenv]"; }
  private:
   FileState* file_;
 };
@@ -218,6 +220,7 @@ class WritableFileImpl : public WritableFile {
   virtual Status Flush() { return Status::OK(); }
   virtual Status Sync() { return Status::OK(); }
 
+  virtual std::string GetName() const { return "[memenv]"; }
  private:
   FileState* file_;
 };
diff --git a/src/leveldb/include/leveldb/env.h b/src/leveldb/include/leveldb/env.h
index 99b6c21..275d441 100644
--- a/src/leveldb/include/leveldb/env.h
+++ b/src/leveldb/include/leveldb/env.h
@@ -191,6 +191,9 @@ class SequentialFile {
   // REQUIRES: External synchronization
   virtual Status Skip(uint64_t n) = 0;
 
+  // Get a name for the file, only for error reporting
+  virtual std::string GetName() const = 0;
+
  private:
   // No copying allowed
   SequentialFile(const SequentialFile&);
@@ -215,6 +218,9 @@ class RandomAccessFile {
   virtual Status Read(uint64_t offset, size_t n, Slice* result,
                       char* scratch) const = 0;
 
+  // Get a name for the file, only for error reporting
+  virtual std::string GetName() const = 0;
+
  private:
   // No copying allowed
   RandomAccessFile(const RandomAccessFile&);
@@ -234,6 +240,9 @@ class WritableFile {
   virtual Status Flush() = 0;
   virtual Status Sync() = 0;
 
+  // Get a name for the file, only for error reporting
+  virtual std::string GetName() const = 0;
+
  private:
   // No copying allowed
   WritableFile(const WritableFile&);
diff --git a/src/leveldb/table/format.cc b/src/leveldb/table/format.cc
index 24e4e02..285e1c0 100644
--- a/src/leveldb/table/format.cc
+++ b/src/leveldb/table/format.cc
@@ -82,7 +82,7 @@ Status ReadBlock(RandomAccessFile* file,
   }
   if (contents.size() != n + kBlockTrailerSize) {
     delete[] buf;
-    return Status::Corruption("truncated block read");
+    return Status::Corruption("truncated block read", file->GetName());
   }
 
   // Check the crc of the type and the block contents
@@ -92,7 +92,7 @@ Status ReadBlock(RandomAccessFile* file,
     const uint32_t actual = crc32c::Value(data, n + 1);
     if (actual != crc) {
       delete[] buf;
-      s = Status::Corruption("block checksum mismatch");
+      s = Status::Corruption("block checksum mismatch", file->GetName());
       return s;
     }
   }
@@ -119,13 +119,13 @@ Status ReadBlock(RandomAccessFile* file,
       size_t ulength = 0;
       if (!port::Snappy_GetUncompressedLength(data, n, &ulength)) {
         delete[] buf;
-        return Status::Corruption("corrupted compressed block contents");
+        return Status::Corruption("corrupted compressed block contents", file->GetName());
       }
       char* ubuf = new char[ulength];
       if (!port::Snappy_Uncompress(data, n, ubuf)) {
         delete[] buf;
         delete[] ubuf;
-        return Status::Corruption("corrupted compressed block contents");
+        return Status::Corruption("corrupted compressed block contents", file->GetName());
       }
       delete[] buf;
       result->data = Slice(ubuf, ulength);
@@ -135,7 +135,7 @@ Status ReadBlock(RandomAccessFile* file,
     }
     default:
       delete[] buf;
-      return Status::Corruption("bad block type");
+      return Status::Corruption("bad block type", file->GetName());
   }
 
   return Status::OK();
diff --git a/src/leveldb/util/env_posix.cc b/src/leveldb/util/env_posix.cc
index dd852af..4676bc2 100644
--- a/src/leveldb/util/env_posix.cc
+++ b/src/leveldb/util/env_posix.cc
@@ -121,6 +121,8 @@ class PosixSequentialFile: public SequentialFile {
     }
     return Status::OK();
   }
+
+  virtual std::string GetName() const { return filename_; }
 };
 
 // pread() based random-access
@@ -172,6 +174,8 @@ class PosixRandomAccessFile: public RandomAccessFile {
     }
     return s;
   }
+
+  virtual std::string GetName() const { return filename_; }
 };
 
 // mmap() based random-access
@@ -206,6 +210,8 @@ class PosixMmapReadableFile: public RandomAccessFile {
     }
     return s;
   }
+
+  virtual std::string GetName() const { return filename_; }
 };
 
 class PosixWritableFile : public WritableFile {
@@ -287,6 +293,8 @@ class PosixWritableFile : public WritableFile {
     }
     return s;
   }
+
+  virtual std::string GetName() const { return filename_; }
 };
 
 static int LockOrUnlock(int fd, bool lock) {
diff --git a/src/leveldb/util/env_win.cc b/src/leveldb/util/env_win.cc
index d32c4e6..8138021 100644
--- a/src/leveldb/util/env_win.cc
+++ b/src/leveldb/util/env_win.cc
@@ -78,6 +78,7 @@ public:
     virtual Status Read(size_t n, Slice* result, char* scratch);
     virtual Status Skip(uint64_t n);
     BOOL isEnable();
+    virtual std::string GetName() const { return _filename; }
 private:
     BOOL _Init();
     void _CleanUp();
@@ -94,6 +95,7 @@ public:
     virtual ~Win32RandomAccessFile();
     virtual Status Read(uint64_t offset, size_t n, Slice* result,char* scratch) const;
     BOOL isEnable();
+    virtual std::string GetName() const { return _filename; }
 private:
     BOOL _Init(LPCWSTR path);
     void _CleanUp();
@@ -114,6 +116,7 @@ public:
     virtual Status Flush();
     virtual Status Sync();
     BOOL isEnable();
+    virtual std::string GetName() const { return filename_; }
 private:
     std::string filename_;
     ::HANDLE _hFile;
diff --git a/src/secp256k1/.travis.yml b/src/secp256k1/.travis.yml
index 2439529..3cec094 100644
--- a/src/secp256k1/.travis.yml
+++ b/src/secp256k1/.travis.yml
@@ -12,7 +12,7 @@ cache:
 env:
   global:
     - FIELD=auto  BIGNUM=auto  SCALAR=auto  ENDOMORPHISM=no  STATICPRECOMPUTATION=yes  ASM=no  BUILD=check  EXTRAFLAGS=  HOST=  ECDH=no  RECOVERY=no  EXPERIMENTAL=no
-    - GUAVA_URL=https://search.maven.org/remotecontent?filepath=com/google/guava/guava/18.0/guava-18.0.jar GUAVA_JAR=src/java/guava/guava-18.0.jar
+    - GUAVA_URL=http://search.maven.org/remotecontent?filepath=com/google/guava/guava/18.0/guava-18.0.jar GUAVA_JAR=src/java/guava/guava-18.0.jar
   matrix:
     - SCALAR=32bit    RECOVERY=yes
     - SCALAR=32bit    FIELD=32bit       ECDH=yes  EXPERIMENTAL=yes
diff --git a/src/secp256k1/Makefile.am b/src/secp256k1/Makefile.am
index b407f88..81022a7 100644
--- a/src/secp256k1/Makefile.am
+++ b/src/secp256k1/Makefile.am
@@ -128,7 +128,7 @@ if USE_JNI
 
 $(JAVA_GUAVA):
 	@echo Guava is missing. Fetch it via: \
-	wget https://search.maven.org/remotecontent?filepath=com/google/guava/guava/18.0/guava-18.0.jar -O $(@)
+	wget http://search.maven.org/remotecontent?filepath=com/google/guava/guava/18.0/guava-18.0.jar -O $(@)
 	@false
 
 .stamp-java: $(JAVA_FILES)
diff --git a/src/secp256k1/README.md b/src/secp256k1/README.md
index f113d43..bc37cba 100644
--- a/src/secp256k1/README.md
+++ b/src/secp256k1/README.md
@@ -1,7 +1,7 @@
 libsecp256k1
 ============
 
-[![Build Status](https://travis-ci.org/fabcoin-core/secp256k1.svg?branch=master)](https://travis-ci.org/fabcoin-core/secp256k1)
+[![Build Status](http://travis-ci.org/fabcoin-core/secp256k1.svg?branch=master)](http://travis-ci.org/fabcoin-core/secp256k1)
 
 Optimized C library for EC operations on curve secp256k1.
 
diff --git a/src/secp256k1/contrib/lax_der_parsing.h b/src/secp256k1/contrib/lax_der_parsing.h
index 4914959..c0879d7 100644
--- a/src/secp256k1/contrib/lax_der_parsing.h
+++ b/src/secp256k1/contrib/lax_der_parsing.h
@@ -48,14 +48,14 @@
  *   8.3.1.
  */
 
-#ifndef _SECP256K1_CONTRIB_LAX_DER_PARSING_H_
-#define _SECP256K1_CONTRIB_LAX_DER_PARSING_H_
+#ifndef SECP256K1_CONTRIB_LAX_DER_PARSING_H
+#define SECP256K1_CONTRIB_LAX_DER_PARSING_H
 
 #include <secp256k1.h>
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 extern "C" {
-# endif
+#endif
 
 /** Parse a signature in "lax DER" format
  *
@@ -88,4 +88,4 @@ int ecdsa_signature_parse_der_lax(
 }
 #endif
 
-#endif
+#endif /* SECP256K1_CONTRIB_LAX_DER_PARSING_H */
diff --git a/src/secp256k1/contrib/lax_der_privatekey_parsing.h b/src/secp256k1/contrib/lax_der_privatekey_parsing.h
index 2fd088f..fece261 100644
--- a/src/secp256k1/contrib/lax_der_privatekey_parsing.h
+++ b/src/secp256k1/contrib/lax_der_privatekey_parsing.h
@@ -25,14 +25,14 @@
  * library are sufficient.
  */
 
-#ifndef _SECP256K1_CONTRIB_BER_PRIVATEKEY_H_
-#define _SECP256K1_CONTRIB_BER_PRIVATEKEY_H_
+#ifndef SECP256K1_CONTRIB_BER_PRIVATEKEY_H
+#define SECP256K1_CONTRIB_BER_PRIVATEKEY_H
 
 #include <secp256k1.h>
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 extern "C" {
-# endif
+#endif
 
 /** Export a private key in DER format.
  *
@@ -87,4 +87,4 @@ SECP256K1_WARN_UNUSED_RESULT int ec_privkey_import_der(
 }
 #endif
 
-#endif
+#endif /* SECP256K1_CONTRIB_BER_PRIVATEKEY_H */
diff --git a/src/secp256k1/include/secp256k1.h b/src/secp256k1/include/secp256k1.h
index fc4c5ce..3e9c098 100644
--- a/src/secp256k1/include/secp256k1.h
+++ b/src/secp256k1/include/secp256k1.h
@@ -1,9 +1,9 @@
-#ifndef _SECP256K1_
-# define _SECP256K1_
+#ifndef SECP256K1_H
+#define SECP256K1_H
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 extern "C" {
-# endif
+#endif
 
 #include <stddef.h>
 
@@ -61,7 +61,7 @@ typedef struct {
  *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
  *  If you need to convert to a format suitable for storage, transmission, or
  *  comparison, use the secp256k1_ecdsa_signature_serialize_* and
- *  secp256k1_ecdsa_signature_serialize_* functions.
+ *  secp256k1_ecdsa_signature_parse_* functions.
  */
 typedef struct {
     unsigned char data[64];
@@ -159,6 +159,13 @@ typedef int (*secp256k1_nonce_function)(
 #define SECP256K1_EC_COMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION | SECP256K1_FLAGS_BIT_COMPRESSION)
 #define SECP256K1_EC_UNCOMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION)
 
+/** Prefix byte used to tag various encoded curvepoints for specific purposes */
+#define SECP256K1_TAG_PUBKEY_EVEN 0x02
+#define SECP256K1_TAG_PUBKEY_ODD 0x03
+#define SECP256K1_TAG_PUBKEY_UNCOMPRESSED 0x04
+#define SECP256K1_TAG_PUBKEY_HYBRID_EVEN 0x06
+#define SECP256K1_TAG_PUBKEY_HYBRID_ODD 0x07
+
 /** Create a secp256k1 context object.
  *
  *  Returns: a newly created context object.
@@ -607,8 +614,8 @@ SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_combine(
     size_t n
 ) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 }
-# endif
-
 #endif
+
+#endif /* SECP256K1_H */
diff --git a/src/secp256k1/include/secp256k1_ecdh.h b/src/secp256k1/include/secp256k1_ecdh.h
index 4b84d7a..88492dc 100644
--- a/src/secp256k1/include/secp256k1_ecdh.h
+++ b/src/secp256k1/include/secp256k1_ecdh.h
@@ -1,11 +1,11 @@
-#ifndef _SECP256K1_ECDH_
-# define _SECP256K1_ECDH_
+#ifndef SECP256K1_ECDH_H
+#define SECP256K1_ECDH_H
 
-# include "secp256k1.h"
+#include "secp256k1.h"
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 extern "C" {
-# endif
+#endif
 
 /** Compute an EC Diffie-Hellman secret in constant time
  *  Returns: 1: exponentiation was successful
@@ -24,8 +24,8 @@ SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdh(
   const unsigned char *privkey
 ) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 }
-# endif
-
 #endif
+
+#endif /* SECP256K1_ECDH_H */
diff --git a/src/secp256k1/include/secp256k1_recovery.h b/src/secp256k1/include/secp256k1_recovery.h
index 0553797..cf6c5ed 100644
--- a/src/secp256k1/include/secp256k1_recovery.h
+++ b/src/secp256k1/include/secp256k1_recovery.h
@@ -1,11 +1,11 @@
-#ifndef _SECP256K1_RECOVERY_
-# define _SECP256K1_RECOVERY_
+#ifndef SECP256K1_RECOVERY_H
+#define SECP256K1_RECOVERY_H
 
-# include "secp256k1.h"
+#include "secp256k1.h"
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 extern "C" {
-# endif
+#endif
 
 /** Opaque data structured that holds a parsed ECDSA signature,
  *  supporting pubkey recovery.
@@ -103,8 +103,8 @@ SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_recover(
     const unsigned char *msg32
 ) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
 
-# ifdef __cplusplus
+#ifdef __cplusplus
 }
-# endif
-
 #endif
+
+#endif /* SECP256K1_RECOVERY_H */
diff --git a/src/secp256k1/libsecp256k1.pc.in b/src/secp256k1/libsecp256k1.pc.in
index a0d006f..21375c6 100644
--- a/src/secp256k1/libsecp256k1.pc.in
+++ b/src/secp256k1/libsecp256k1.pc.in
@@ -5,7 +5,7 @@ includedir=@includedir@
 
 Name: libsecp256k1
 Description: Optimized C library for EC operations on curve secp256k1
-URL: https://github.com/bitcoin-core/secp256k1
+URL: http:///github.com/blockchaingate/secp256k1
 Version: @PACKAGE_VERSION@
 Cflags: -I${includedir}
 Libs.private: @SECP_LIBS@
diff --git a/src/secp256k1/sage/group_prover.sage b/src/secp256k1/sage/group_prover.sage
index 5198724..8521f07 100644
--- a/src/secp256k1/sage/group_prover.sage
+++ b/src/secp256k1/sage/group_prover.sage
@@ -17,7 +17,7 @@
 #   - A constraint describing the requirements of the law, called "require"
 # * Implementations are transliterated into functions that operate as well on
 #   algebraic input points, and are called once per combination of branches
-#   exectured. Each execution returns:
+#   executed. Each execution returns:
 #   - A constraint describing the assumptions this implementation requires
 #     (such as Z1=1), called "assumeFormula"
 #   - A constraint describing the assumptions this specific branch requires,
diff --git a/src/secp256k1/src/asm/field_10x26_arm.s b/src/secp256k1/src/asm/field_10x26_arm.s
index bd2b629..5a9cc3f 100644
--- a/src/secp256k1/src/asm/field_10x26_arm.s
+++ b/src/secp256k1/src/asm/field_10x26_arm.s
@@ -23,7 +23,7 @@ Note:
 	.eabi_attribute 10, 0 @ Tag_FP_arch = none
 	.eabi_attribute 24, 1 @ Tag_ABI_align_needed = 8-byte
 	.eabi_attribute 25, 1 @ Tag_ABI_align_preserved = 8-byte, except leaf SP
-	.eabi_attribute 30, 2 @ Tag_ABI_optimization_goals = Agressive Speed
+	.eabi_attribute 30, 2 @ Tag_ABI_optimization_goals = Aggressive Speed
 	.eabi_attribute 34, 1 @ Tag_CPU_unaligned_access = v6
 	.text
 
diff --git a/src/secp256k1/src/basic-config.h b/src/secp256k1/src/basic-config.h
index c4c16eb..fc58806 100644
--- a/src/secp256k1/src/basic-config.h
+++ b/src/secp256k1/src/basic-config.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_BASIC_CONFIG_
-#define _SECP256K1_BASIC_CONFIG_
+#ifndef SECP256K1_BASIC_CONFIG_H
+#define SECP256K1_BASIC_CONFIG_H
 
 #ifdef USE_BASIC_CONFIG
 
@@ -28,5 +28,6 @@
 #define USE_FIELD_10X26 1
 #define USE_SCALAR_8X32 1
 
-#endif // USE_BASIC_CONFIG
-#endif // _SECP256K1_BASIC_CONFIG_
+#endif /* USE_BASIC_CONFIG */
+
+#endif /* SECP256K1_BASIC_CONFIG_H */
diff --git a/src/secp256k1/src/bench.h b/src/secp256k1/src/bench.h
index d67f08a..d5ebe01 100644
--- a/src/secp256k1/src/bench.h
+++ b/src/secp256k1/src/bench.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_BENCH_H_
-#define _SECP256K1_BENCH_H_
+#ifndef SECP256K1_BENCH_H
+#define SECP256K1_BENCH_H
 
 #include <stdio.h>
 #include <math.h>
@@ -63,4 +63,4 @@ void run_benchmark(char *name, void (*benchmark)(void*), void (*setup)(void*), v
     printf("us\n");
 }
 
-#endif
+#endif /* SECP256K1_BENCH_H */
diff --git a/src/secp256k1/src/ecdsa.h b/src/secp256k1/src/ecdsa.h
index 54ae101..80590c7 100644
--- a/src/secp256k1/src/ecdsa.h
+++ b/src/secp256k1/src/ecdsa.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECDSA_
-#define _SECP256K1_ECDSA_
+#ifndef SECP256K1_ECDSA_H
+#define SECP256K1_ECDSA_H
 
 #include <stddef.h>
 
@@ -18,4 +18,4 @@ static int secp256k1_ecdsa_sig_serialize(unsigned char *sig, size_t *size, const
 static int secp256k1_ecdsa_sig_verify(const secp256k1_ecmult_context *ctx, const secp256k1_scalar* r, const secp256k1_scalar* s, const secp256k1_ge *pubkey, const secp256k1_scalar *message);
 static int secp256k1_ecdsa_sig_sign(const secp256k1_ecmult_gen_context *ctx, secp256k1_scalar* r, secp256k1_scalar* s, const secp256k1_scalar *seckey, const secp256k1_scalar *message, const secp256k1_scalar *nonce, int *recid);
 
-#endif
+#endif /* SECP256K1_ECDSA_H */
diff --git a/src/secp256k1/src/ecdsa_impl.h b/src/secp256k1/src/ecdsa_impl.h
index 453bb11..c340004 100644
--- a/src/secp256k1/src/ecdsa_impl.h
+++ b/src/secp256k1/src/ecdsa_impl.h
@@ -5,8 +5,8 @@
  **********************************************************************/
 
 
-#ifndef _SECP256K1_ECDSA_IMPL_H_
-#define _SECP256K1_ECDSA_IMPL_H_
+#ifndef SECP256K1_ECDSA_IMPL_H
+#define SECP256K1_ECDSA_IMPL_H
 
 #include "scalar.h"
 #include "field.h"
@@ -81,8 +81,6 @@ static int secp256k1_der_read_len(const unsigned char **sigp, const unsigned cha
         return -1;
     }
     while (lenleft > 0) {
-        if ((ret >> ((sizeof(size_t) - 1) * 8)) != 0) {
-        }
         ret = (ret << 8) | **sigp;
         if (ret + lenleft > (size_t)(sigend - *sigp)) {
             /* Result exceeds the length of the passed array. */
@@ -312,4 +310,4 @@ static int secp256k1_ecdsa_sig_sign(const secp256k1_ecmult_gen_context *ctx, sec
     return 1;
 }
 
-#endif
+#endif /* SECP256K1_ECDSA_IMPL_H */
diff --git a/src/secp256k1/src/eckey.h b/src/secp256k1/src/eckey.h
index 42739a3..b621f1e 100644
--- a/src/secp256k1/src/eckey.h
+++ b/src/secp256k1/src/eckey.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECKEY_
-#define _SECP256K1_ECKEY_
+#ifndef SECP256K1_ECKEY_H
+#define SECP256K1_ECKEY_H
 
 #include <stddef.h>
 
@@ -22,4 +22,4 @@ static int secp256k1_eckey_pubkey_tweak_add(const secp256k1_ecmult_context *ctx,
 static int secp256k1_eckey_privkey_tweak_mul(secp256k1_scalar *key, const secp256k1_scalar *tweak);
 static int secp256k1_eckey_pubkey_tweak_mul(const secp256k1_ecmult_context *ctx, secp256k1_ge *key, const secp256k1_scalar *tweak);
 
-#endif
+#endif /* SECP256K1_ECKEY_H */
diff --git a/src/secp256k1/src/eckey_impl.h b/src/secp256k1/src/eckey_impl.h
index ce38071..1ab9a68 100644
--- a/src/secp256k1/src/eckey_impl.h
+++ b/src/secp256k1/src/eckey_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECKEY_IMPL_H_
-#define _SECP256K1_ECKEY_IMPL_H_
+#ifndef SECP256K1_ECKEY_IMPL_H
+#define SECP256K1_ECKEY_IMPL_H
 
 #include "eckey.h"
 
@@ -15,16 +15,17 @@
 #include "ecmult_gen.h"
 
 static int secp256k1_eckey_pubkey_parse(secp256k1_ge *elem, const unsigned char *pub, size_t size) {
-    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
+    if (size == 33 && (pub[0] == SECP256K1_TAG_PUBKEY_EVEN || pub[0] == SECP256K1_TAG_PUBKEY_ODD)) {
         secp256k1_fe x;
-        return secp256k1_fe_set_b32(&x, pub+1) && secp256k1_ge_set_xo_var(elem, &x, pub[0] == 0x03);
+        return secp256k1_fe_set_b32(&x, pub+1) && secp256k1_ge_set_xo_var(elem, &x, pub[0] == SECP256K1_TAG_PUBKEY_ODD);
     } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
         secp256k1_fe x, y;
         if (!secp256k1_fe_set_b32(&x, pub+1) || !secp256k1_fe_set_b32(&y, pub+33)) {
             return 0;
         }
         secp256k1_ge_set_xy(elem, &x, &y);
-        if ((pub[0] == 0x06 || pub[0] == 0x07) && secp256k1_fe_is_odd(&y) != (pub[0] == 0x07)) {
+        if ((pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_EVEN || pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD) &&
+            secp256k1_fe_is_odd(&y) != (pub[0] == SECP256K1_TAG_PUBKEY_HYBRID_ODD)) {
             return 0;
         }
         return secp256k1_ge_is_valid_var(elem);
@@ -42,10 +43,10 @@ static int secp256k1_eckey_pubkey_serialize(secp256k1_ge *elem, unsigned char *p
     secp256k1_fe_get_b32(&pub[1], &elem->x);
     if (compressed) {
         *size = 33;
-        pub[0] = 0x02 | (secp256k1_fe_is_odd(&elem->y) ? 0x01 : 0x00);
+        pub[0] = secp256k1_fe_is_odd(&elem->y) ? SECP256K1_TAG_PUBKEY_ODD : SECP256K1_TAG_PUBKEY_EVEN;
     } else {
         *size = 65;
-        pub[0] = 0x04;
+        pub[0] = SECP256K1_TAG_PUBKEY_UNCOMPRESSED;
         secp256k1_fe_get_b32(&pub[33], &elem->y);
     }
     return 1;
@@ -96,4 +97,4 @@ static int secp256k1_eckey_pubkey_tweak_mul(const secp256k1_ecmult_context *ctx,
     return 1;
 }
 
-#endif
+#endif /* SECP256K1_ECKEY_IMPL_H */
diff --git a/src/secp256k1/src/ecmult.h b/src/secp256k1/src/ecmult.h
index 2048413..6d44aba 100644
--- a/src/secp256k1/src/ecmult.h
+++ b/src/secp256k1/src/ecmult.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_
-#define _SECP256K1_ECMULT_
+#ifndef SECP256K1_ECMULT_H
+#define SECP256K1_ECMULT_H
 
 #include "num.h"
 #include "group.h"
@@ -28,4 +28,4 @@ static int secp256k1_ecmult_context_is_built(const secp256k1_ecmult_context *ctx
 /** Double multiply: R = na*A + ng*G */
 static void secp256k1_ecmult(const secp256k1_ecmult_context *ctx, secp256k1_gej *r, const secp256k1_gej *a, const secp256k1_scalar *na, const secp256k1_scalar *ng);
 
-#endif
+#endif /* SECP256K1_ECMULT_H */
diff --git a/src/secp256k1/src/ecmult_const.h b/src/secp256k1/src/ecmult_const.h
index 2b00976..72bf7d7 100644
--- a/src/secp256k1/src/ecmult_const.h
+++ b/src/secp256k1/src/ecmult_const.h
@@ -4,12 +4,12 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_CONST_
-#define _SECP256K1_ECMULT_CONST_
+#ifndef SECP256K1_ECMULT_CONST_H
+#define SECP256K1_ECMULT_CONST_H
 
 #include "scalar.h"
 #include "group.h"
 
 static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, const secp256k1_scalar *q);
 
-#endif
+#endif /* SECP256K1_ECMULT_CONST_H */
diff --git a/src/secp256k1/src/ecmult_const_impl.h b/src/secp256k1/src/ecmult_const_impl.h
index 0db314c..7d7a172 100644
--- a/src/secp256k1/src/ecmult_const_impl.h
+++ b/src/secp256k1/src/ecmult_const_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_CONST_IMPL_
-#define _SECP256K1_ECMULT_CONST_IMPL_
+#ifndef SECP256K1_ECMULT_CONST_IMPL_H
+#define SECP256K1_ECMULT_CONST_IMPL_H
 
 #include "scalar.h"
 #include "group.h"
@@ -42,11 +42,12 @@
 } while(0)
 
 
-/** Convert a number to WNAF notation. The number becomes represented by sum(2^{wi} * wnaf[i], i=0..return_val)
- *  with the following guarantees:
+/** Convert a number to WNAF notation.
+ *  The number becomes represented by sum(2^{wi} * wnaf[i], i=0..WNAF_SIZE(w)+1) - return_val.
+ *  It has the following guarantees:
  *  - each wnaf[i] an odd integer between -(1 << w) and (1 << w)
  *  - each wnaf[i] is nonzero
- *  - the number of words set is returned; this is always (WNAF_BITS + w - 1) / w
+ *  - the number of words set is always WNAF_SIZE(w) + 1
  *
  *  Adapted from `The Width-w NAF Method Provides Small Memory and Fast Elliptic Scalar
  *  Multiplications Secure against Side Channel Attacks`, Okeya and Tagaki. M. Joye (Ed.)
@@ -236,4 +237,4 @@ static void secp256k1_ecmult_const(secp256k1_gej *r, const secp256k1_ge *a, cons
     }
 }
 
-#endif
+#endif /* SECP256K1_ECMULT_CONST_IMPL_H */
diff --git a/src/secp256k1/src/ecmult_gen.h b/src/secp256k1/src/ecmult_gen.h
index eb2cc9e..7564b70 100644
--- a/src/secp256k1/src/ecmult_gen.h
+++ b/src/secp256k1/src/ecmult_gen.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_GEN_
-#define _SECP256K1_ECMULT_GEN_
+#ifndef SECP256K1_ECMULT_GEN_H
+#define SECP256K1_ECMULT_GEN_H
 
 #include "scalar.h"
 #include "group.h"
@@ -40,4 +40,4 @@ static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context* ctx, secp25
 
 static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const unsigned char *seed32);
 
-#endif
+#endif /* SECP256K1_ECMULT_GEN_H */
diff --git a/src/secp256k1/src/ecmult_gen_impl.h b/src/secp256k1/src/ecmult_gen_impl.h
index 35f2546..b24f3a2 100644
--- a/src/secp256k1/src/ecmult_gen_impl.h
+++ b/src/secp256k1/src/ecmult_gen_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_GEN_IMPL_H_
-#define _SECP256K1_ECMULT_GEN_IMPL_H_
+#ifndef SECP256K1_ECMULT_GEN_IMPL_H
+#define SECP256K1_ECMULT_GEN_IMPL_H
 
 #include "scalar.h"
 #include "group.h"
@@ -140,7 +140,7 @@ static void secp256k1_ecmult_gen(const secp256k1_ecmult_gen_context *ctx, secp25
              *   sidechannels, even when the cache-line access patterns are uniform.
              *  See also:
              *   "A word of warning", CHES 2013 Rump Session, by Daniel J. Bernstein and Peter Schwabe
-             *    (https://cryptojedi.org/peter/data/chesrump-20130822.pdf) and
+             *    (http://cryptojedi.org/peter/data/chesrump-20130822.pdf) and
              *   "Cache Attacks and Countermeasures: the Case of AES", RSA 2006,
              *    by Dag Arne Osvik, Adi Shamir, and Eran Tromer
              *    (http://www.tau.ac.il/~tromer/papers/cache.pdf)
@@ -207,4 +207,4 @@ static void secp256k1_ecmult_gen_blind(secp256k1_ecmult_gen_context *ctx, const
     secp256k1_gej_clear(&gb);
 }
 
-#endif
+#endif /* SECP256K1_ECMULT_GEN_IMPL_H */
diff --git a/src/secp256k1/src/ecmult_impl.h b/src/secp256k1/src/ecmult_impl.h
index 4e40104..93d3794 100644
--- a/src/secp256k1/src/ecmult_impl.h
+++ b/src/secp256k1/src/ecmult_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_ECMULT_IMPL_H_
-#define _SECP256K1_ECMULT_IMPL_H_
+#ifndef SECP256K1_ECMULT_IMPL_H
+#define SECP256K1_ECMULT_IMPL_H
 
 #include <string.h>
 
@@ -403,4 +403,4 @@ static void secp256k1_ecmult(const secp256k1_ecmult_context *ctx, secp256k1_gej
     }
 }
 
-#endif
+#endif /* SECP256K1_ECMULT_IMPL_H */
diff --git a/src/secp256k1/src/field.h b/src/secp256k1/src/field.h
index bbb1ee8..bb6692a 100644
--- a/src/secp256k1/src/field.h
+++ b/src/secp256k1/src/field.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_
-#define _SECP256K1_FIELD_
+#ifndef SECP256K1_FIELD_H
+#define SECP256K1_FIELD_H
 
 /** Field element module.
  *
@@ -129,4 +129,4 @@ static void secp256k1_fe_storage_cmov(secp256k1_fe_storage *r, const secp256k1_f
 /** If flag is true, set *r equal to *a; otherwise leave it. Constant-time. */
 static void secp256k1_fe_cmov(secp256k1_fe *r, const secp256k1_fe *a, int flag);
 
-#endif
+#endif /* SECP256K1_FIELD_H */
diff --git a/src/secp256k1/src/field_10x26.h b/src/secp256k1/src/field_10x26.h
index 61ee1e0..727c526 100644
--- a/src/secp256k1/src/field_10x26.h
+++ b/src/secp256k1/src/field_10x26.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_REPR_
-#define _SECP256K1_FIELD_REPR_
+#ifndef SECP256K1_FIELD_REPR_H
+#define SECP256K1_FIELD_REPR_H
 
 #include <stdint.h>
 
@@ -44,4 +44,5 @@ typedef struct {
 
 #define SECP256K1_FE_STORAGE_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {{ (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) }}
 #define SECP256K1_FE_STORAGE_CONST_GET(d) d.n[7], d.n[6], d.n[5], d.n[4],d.n[3], d.n[2], d.n[1], d.n[0]
-#endif
+
+#endif /* SECP256K1_FIELD_REPR_H */
diff --git a/src/secp256k1/src/field_10x26_impl.h b/src/secp256k1/src/field_10x26_impl.h
index 234c13a..94f8132 100644
--- a/src/secp256k1/src/field_10x26_impl.h
+++ b/src/secp256k1/src/field_10x26_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_REPR_IMPL_H_
-#define _SECP256K1_FIELD_REPR_IMPL_H_
+#ifndef SECP256K1_FIELD_REPR_IMPL_H
+#define SECP256K1_FIELD_REPR_IMPL_H
 
 #include "util.h"
 #include "num.h"
@@ -1158,4 +1158,4 @@ static SECP256K1_INLINE void secp256k1_fe_from_storage(secp256k1_fe *r, const se
 #endif
 }
 
-#endif
+#endif /* SECP256K1_FIELD_REPR_IMPL_H */
diff --git a/src/secp256k1/src/field_5x52.h b/src/secp256k1/src/field_5x52.h
index 8e69a56..bccd8fe 100644
--- a/src/secp256k1/src/field_5x52.h
+++ b/src/secp256k1/src/field_5x52.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_REPR_
-#define _SECP256K1_FIELD_REPR_
+#ifndef SECP256K1_FIELD_REPR_H
+#define SECP256K1_FIELD_REPR_H
 
 #include <stdint.h>
 
@@ -44,4 +44,4 @@ typedef struct {
     (d6) | (((uint64_t)(d7)) << 32) \
 }}
 
-#endif
+#endif /* SECP256K1_FIELD_REPR_H */
diff --git a/src/secp256k1/src/field_5x52_asm_impl.h b/src/secp256k1/src/field_5x52_asm_impl.h
index 98cc004..1fc3171 100644
--- a/src/secp256k1/src/field_5x52_asm_impl.h
+++ b/src/secp256k1/src/field_5x52_asm_impl.h
@@ -11,8 +11,8 @@
  * - December 2014, Pieter Wuille: converted from YASM to GCC inline assembly
  */
 
-#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
-#define _SECP256K1_FIELD_INNER5X52_IMPL_H_
+#ifndef SECP256K1_FIELD_INNER5X52_IMPL_H
+#define SECP256K1_FIELD_INNER5X52_IMPL_H
 
 SECP256K1_INLINE static void secp256k1_fe_mul_inner(uint64_t *r, const uint64_t *a, const uint64_t * SECP256K1_RESTRICT b) {
 /**
@@ -499,4 +499,4 @@ __asm__ __volatile__(
 );
 }
 
-#endif
+#endif /* SECP256K1_FIELD_INNER5X52_IMPL_H */
diff --git a/src/secp256k1/src/field_5x52_impl.h b/src/secp256k1/src/field_5x52_impl.h
index 8e8b286..957c61b 100644
--- a/src/secp256k1/src/field_5x52_impl.h
+++ b/src/secp256k1/src/field_5x52_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_REPR_IMPL_H_
-#define _SECP256K1_FIELD_REPR_IMPL_H_
+#ifndef SECP256K1_FIELD_REPR_IMPL_H
+#define SECP256K1_FIELD_REPR_IMPL_H
 
 #if defined HAVE_CONFIG_H
 #include "libsecp256k1-config.h"
@@ -493,4 +493,4 @@ static SECP256K1_INLINE void secp256k1_fe_from_storage(secp256k1_fe *r, const se
 #endif
 }
 
-#endif
+#endif /* SECP256K1_FIELD_REPR_IMPL_H */
diff --git a/src/secp256k1/src/field_5x52_int128_impl.h b/src/secp256k1/src/field_5x52_int128_impl.h
index 0bf22bd..95a0d17 100644
--- a/src/secp256k1/src/field_5x52_int128_impl.h
+++ b/src/secp256k1/src/field_5x52_int128_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
-#define _SECP256K1_FIELD_INNER5X52_IMPL_H_
+#ifndef SECP256K1_FIELD_INNER5X52_IMPL_H
+#define SECP256K1_FIELD_INNER5X52_IMPL_H
 
 #include <stdint.h>
 
@@ -274,4 +274,4 @@ SECP256K1_INLINE static void secp256k1_fe_sqr_inner(uint64_t *r, const uint64_t
     /* [r4 r3 r2 r1 r0] = [p8 p7 p6 p5 p4 p3 p2 p1 p0] */
 }
 
-#endif
+#endif /* SECP256K1_FIELD_INNER5X52_IMPL_H */
diff --git a/src/secp256k1/src/field_impl.h b/src/secp256k1/src/field_impl.h
index 5127b27..2042864 100644
--- a/src/secp256k1/src/field_impl.h
+++ b/src/secp256k1/src/field_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_FIELD_IMPL_H_
-#define _SECP256K1_FIELD_IMPL_H_
+#ifndef SECP256K1_FIELD_IMPL_H
+#define SECP256K1_FIELD_IMPL_H
 
 #if defined HAVE_CONFIG_H
 #include "libsecp256k1-config.h"
@@ -312,4 +312,4 @@ static int secp256k1_fe_is_quad_var(const secp256k1_fe *a) {
 #endif
 }
 
-#endif
+#endif /* SECP256K1_FIELD_IMPL_H */
diff --git a/src/secp256k1/src/group.h b/src/secp256k1/src/group.h
index 4957b24..ea1302d 100644
--- a/src/secp256k1/src/group.h
+++ b/src/secp256k1/src/group.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_GROUP_
-#define _SECP256K1_GROUP_
+#ifndef SECP256K1_GROUP_H
+#define SECP256K1_GROUP_H
 
 #include "num.h"
 #include "field.h"
@@ -141,4 +141,4 @@ static void secp256k1_ge_storage_cmov(secp256k1_ge_storage *r, const secp256k1_g
 /** Rescale a jacobian point by b which must be non-zero. Constant-time. */
 static void secp256k1_gej_rescale(secp256k1_gej *r, const secp256k1_fe *b);
 
-#endif
+#endif /* SECP256K1_GROUP_H */
diff --git a/src/secp256k1/src/group_impl.h b/src/secp256k1/src/group_impl.h
index 7d72353..414355d 100644
--- a/src/secp256k1/src/group_impl.h
+++ b/src/secp256k1/src/group_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_GROUP_IMPL_H_
-#define _SECP256K1_GROUP_IMPL_H_
+#ifndef SECP256K1_GROUP_IMPL_H
+#define SECP256K1_GROUP_IMPL_H
 
 #include "num.h"
 #include "field.h"
@@ -302,7 +302,7 @@ static void secp256k1_gej_double_var(secp256k1_gej *r, const secp256k1_gej *a, s
     /* Operations: 3 mul, 4 sqr, 0 normalize, 12 mul_int/add/negate.
      *
      * Note that there is an implementation described at
-     *     https://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-0.html#doubling-dbl-2009-l
+     *     http://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-0.html#doubling-dbl-2009-l
      * which trades a multiply for a square, but in practice this is actually slower,
      * mainly because it requires more normalizations.
      */
@@ -697,4 +697,4 @@ static int secp256k1_gej_has_quad_y_var(const secp256k1_gej *a) {
     return secp256k1_fe_is_quad_var(&yz);
 }
 
-#endif
+#endif /* SECP256K1_GROUP_IMPL_H */
diff --git a/src/secp256k1/src/hash.h b/src/secp256k1/src/hash.h
index fca98ca..e08d25d 100644
--- a/src/secp256k1/src/hash.h
+++ b/src/secp256k1/src/hash.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_HASH_
-#define _SECP256K1_HASH_
+#ifndef SECP256K1_HASH_H
+#define SECP256K1_HASH_H
 
 #include <stdlib.h>
 #include <stdint.h>
@@ -38,4 +38,4 @@ static void secp256k1_rfc6979_hmac_sha256_initialize(secp256k1_rfc6979_hmac_sha2
 static void secp256k1_rfc6979_hmac_sha256_generate(secp256k1_rfc6979_hmac_sha256_t *rng, unsigned char *out, size_t outlen);
 static void secp256k1_rfc6979_hmac_sha256_finalize(secp256k1_rfc6979_hmac_sha256_t *rng);
 
-#endif
+#endif /* SECP256K1_HASH_H */
diff --git a/src/secp256k1/src/hash_impl.h b/src/secp256k1/src/hash_impl.h
index b47e65f..4c9964e 100644
--- a/src/secp256k1/src/hash_impl.h
+++ b/src/secp256k1/src/hash_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_HASH_IMPL_H_
-#define _SECP256K1_HASH_IMPL_H_
+#ifndef SECP256K1_HASH_IMPL_H
+#define SECP256K1_HASH_IMPL_H
 
 #include "hash.h"
 
@@ -278,4 +278,4 @@ static void secp256k1_rfc6979_hmac_sha256_finalize(secp256k1_rfc6979_hmac_sha256
 #undef Maj
 #undef Ch
 
-#endif
+#endif /* SECP256K1_HASH_IMPL_H */
diff --git a/src/secp256k1/src/java/org/fabcoin/NativeSecp256k1.java b/src/secp256k1/src/java/org/fabcoin/NativeSecp256k1.java
index 9e9c7ac..f9da881 100644
--- a/src/secp256k1/src/java/org/fabcoin/NativeSecp256k1.java
+++ b/src/secp256k1/src/java/org/fabcoin/NativeSecp256k1.java
@@ -29,7 +29,7 @@ import static org.fabcoin.NativeSecp256k1Util.*;
 /**
  * <p>This class holds native methods to handle ECDSA verification.</p>
  *
- * <p>You can find an example library that can be used for this at https://github.com/fabcoin/secp256k1</p>
+ * <p>You can find an example library that can be used for this at http://github.com/fabcoin/secp256k1</p>
  *
  * <p>To build secp256k1 for use with fabcoinj, run
  * `./configure --enable-jni --enable-experimental --enable-module-ecdh`
diff --git a/src/secp256k1/src/modules/ecdh/main_impl.h b/src/secp256k1/src/modules/ecdh/main_impl.h
index 9e30fb7..01ecba4 100644
--- a/src/secp256k1/src/modules/ecdh/main_impl.h
+++ b/src/secp256k1/src/modules/ecdh/main_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_MODULE_ECDH_MAIN_
-#define _SECP256K1_MODULE_ECDH_MAIN_
+#ifndef SECP256K1_MODULE_ECDH_MAIN_H
+#define SECP256K1_MODULE_ECDH_MAIN_H
 
 #include "include/secp256k1_ecdh.h"
 #include "ecmult_const_impl.h"
@@ -51,4 +51,4 @@ int secp256k1_ecdh(const secp256k1_context* ctx, unsigned char *result, const se
     return ret;
 }
 
-#endif
+#endif /* SECP256K1_MODULE_ECDH_MAIN_H */
diff --git a/src/secp256k1/src/modules/ecdh/tests_impl.h b/src/secp256k1/src/modules/ecdh/tests_impl.h
index 85a5d0a..cec30b6 100644
--- a/src/secp256k1/src/modules/ecdh/tests_impl.h
+++ b/src/secp256k1/src/modules/ecdh/tests_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_MODULE_ECDH_TESTS_
-#define _SECP256K1_MODULE_ECDH_TESTS_
+#ifndef SECP256K1_MODULE_ECDH_TESTS_H
+#define SECP256K1_MODULE_ECDH_TESTS_H
 
 void test_ecdh_api(void) {
     /* Setup context that just counts errors */
@@ -102,4 +102,4 @@ void run_ecdh_tests(void) {
     test_bad_scalar();
 }
 
-#endif
+#endif /* SECP256K1_MODULE_ECDH_TESTS_H */
diff --git a/src/secp256k1/src/modules/recovery/main_impl.h b/src/secp256k1/src/modules/recovery/main_impl.h
index c6fbe23..2f6691c 100644
--- a/src/secp256k1/src/modules/recovery/main_impl.h
+++ b/src/secp256k1/src/modules/recovery/main_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_MODULE_RECOVERY_MAIN_
-#define _SECP256K1_MODULE_RECOVERY_MAIN_
+#ifndef SECP256K1_MODULE_RECOVERY_MAIN_H
+#define SECP256K1_MODULE_RECOVERY_MAIN_H
 
 #include "include/secp256k1_recovery.h"
 
@@ -190,4 +190,4 @@ int secp256k1_ecdsa_recover(const secp256k1_context* ctx, secp256k1_pubkey *pubk
     }
 }
 
-#endif
+#endif /* SECP256K1_MODULE_RECOVERY_MAIN_H */
diff --git a/src/secp256k1/src/modules/recovery/tests_impl.h b/src/secp256k1/src/modules/recovery/tests_impl.h
index 765c7dd..5c9bbe8 100644
--- a/src/secp256k1/src/modules/recovery/tests_impl.h
+++ b/src/secp256k1/src/modules/recovery/tests_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_MODULE_RECOVERY_TESTS_
-#define _SECP256K1_MODULE_RECOVERY_TESTS_
+#ifndef SECP256K1_MODULE_RECOVERY_TESTS_H
+#define SECP256K1_MODULE_RECOVERY_TESTS_H
 
 static int recovery_test_nonce_function(unsigned char *nonce32, const unsigned char *msg32, const unsigned char *key32, const unsigned char *algo16, void *data, unsigned int counter) {
     (void) msg32;
@@ -390,4 +390,4 @@ void run_recovery_tests(void) {
     test_ecdsa_recovery_edge_cases();
 }
 
-#endif
+#endif /* SECP256K1_MODULE_RECOVERY_TESTS_H */
diff --git a/src/secp256k1/src/num.h b/src/secp256k1/src/num.h
index 7bb9c5b..49f2dd7 100644
--- a/src/secp256k1/src/num.h
+++ b/src/secp256k1/src/num.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_NUM_
-#define _SECP256K1_NUM_
+#ifndef SECP256K1_NUM_H
+#define SECP256K1_NUM_H
 
 #ifndef USE_NUM_NONE
 
@@ -71,4 +71,4 @@ static void secp256k1_num_negate(secp256k1_num *r);
 
 #endif
 
-#endif
+#endif /* SECP256K1_NUM_H */
diff --git a/src/secp256k1/src/num_gmp.h b/src/secp256k1/src/num_gmp.h
index 7dd8130..3619844 100644
--- a/src/secp256k1/src/num_gmp.h
+++ b/src/secp256k1/src/num_gmp.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_NUM_REPR_
-#define _SECP256K1_NUM_REPR_
+#ifndef SECP256K1_NUM_REPR_H
+#define SECP256K1_NUM_REPR_H
 
 #include <gmp.h>
 
@@ -17,4 +17,4 @@ typedef struct {
     int limbs;
 } secp256k1_num;
 
-#endif
+#endif /* SECP256K1_NUM_REPR_H */
diff --git a/src/secp256k1/src/num_gmp_impl.h b/src/secp256k1/src/num_gmp_impl.h
index 3a46495..0ae2a8b 100644
--- a/src/secp256k1/src/num_gmp_impl.h
+++ b/src/secp256k1/src/num_gmp_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_NUM_REPR_IMPL_H_
-#define _SECP256K1_NUM_REPR_IMPL_H_
+#ifndef SECP256K1_NUM_REPR_IMPL_H
+#define SECP256K1_NUM_REPR_IMPL_H
 
 #include <string.h>
 #include <stdlib.h>
@@ -285,4 +285,4 @@ static void secp256k1_num_negate(secp256k1_num *r) {
     r->neg ^= 1;
 }
 
-#endif
+#endif /* SECP256K1_NUM_REPR_IMPL_H */
diff --git a/src/secp256k1/src/num_impl.h b/src/secp256k1/src/num_impl.h
index 0b0e3a0..c45193b 100644
--- a/src/secp256k1/src/num_impl.h
+++ b/src/secp256k1/src/num_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_NUM_IMPL_H_
-#define _SECP256K1_NUM_IMPL_H_
+#ifndef SECP256K1_NUM_IMPL_H
+#define SECP256K1_NUM_IMPL_H
 
 #if defined HAVE_CONFIG_H
 #include "libsecp256k1-config.h"
@@ -21,4 +21,4 @@
 #error "Please select num implementation"
 #endif
 
-#endif
+#endif /* SECP256K1_NUM_IMPL_H */
diff --git a/src/secp256k1/src/scalar.h b/src/secp256k1/src/scalar.h
index 27e9d83..59304cb 100644
--- a/src/secp256k1/src/scalar.h
+++ b/src/secp256k1/src/scalar.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_
-#define _SECP256K1_SCALAR_
+#ifndef SECP256K1_SCALAR_H
+#define SECP256K1_SCALAR_H
 
 #include "num.h"
 
@@ -103,4 +103,4 @@ static void secp256k1_scalar_split_lambda(secp256k1_scalar *r1, secp256k1_scalar
 /** Multiply a and b (without taking the modulus!), divide by 2**shift, and round to the nearest integer. Shift must be at least 256. */
 static void secp256k1_scalar_mul_shift_var(secp256k1_scalar *r, const secp256k1_scalar *a, const secp256k1_scalar *b, unsigned int shift);
 
-#endif
+#endif /* SECP256K1_SCALAR_H */
diff --git a/src/secp256k1/src/scalar_4x64.h b/src/secp256k1/src/scalar_4x64.h
index cff4060..19c7495 100644
--- a/src/secp256k1/src/scalar_4x64.h
+++ b/src/secp256k1/src/scalar_4x64.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_
-#define _SECP256K1_SCALAR_REPR_
+#ifndef SECP256K1_SCALAR_REPR_H
+#define SECP256K1_SCALAR_REPR_H
 
 #include <stdint.h>
 
@@ -16,4 +16,4 @@ typedef struct {
 
 #define SECP256K1_SCALAR_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {{((uint64_t)(d1)) << 32 | (d0), ((uint64_t)(d3)) << 32 | (d2), ((uint64_t)(d5)) << 32 | (d4), ((uint64_t)(d7)) << 32 | (d6)}}
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_H */
diff --git a/src/secp256k1/src/scalar_4x64_impl.h b/src/secp256k1/src/scalar_4x64_impl.h
index 56e7bd8..db1ebf9 100644
--- a/src/secp256k1/src/scalar_4x64_impl.h
+++ b/src/secp256k1/src/scalar_4x64_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_IMPL_H_
-#define _SECP256K1_SCALAR_REPR_IMPL_H_
+#ifndef SECP256K1_SCALAR_REPR_IMPL_H
+#define SECP256K1_SCALAR_REPR_IMPL_H
 
 /* Limbs of the secp256k1 order. */
 #define SECP256K1_N_0 ((uint64_t)0xBFD25E8CD0364141ULL)
@@ -946,4 +946,4 @@ SECP256K1_INLINE static void secp256k1_scalar_mul_shift_var(secp256k1_scalar *r,
     secp256k1_scalar_cadd_bit(r, 0, (l[(shift - 1) >> 6] >> ((shift - 1) & 0x3f)) & 1);
 }
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_IMPL_H */
diff --git a/src/secp256k1/src/scalar_8x32.h b/src/secp256k1/src/scalar_8x32.h
index 1319664..2c9a348 100644
--- a/src/secp256k1/src/scalar_8x32.h
+++ b/src/secp256k1/src/scalar_8x32.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_
-#define _SECP256K1_SCALAR_REPR_
+#ifndef SECP256K1_SCALAR_REPR_H
+#define SECP256K1_SCALAR_REPR_H
 
 #include <stdint.h>
 
@@ -16,4 +16,4 @@ typedef struct {
 
 #define SECP256K1_SCALAR_CONST(d7, d6, d5, d4, d3, d2, d1, d0) {{(d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7)}}
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_H */
diff --git a/src/secp256k1/src/scalar_8x32_impl.h b/src/secp256k1/src/scalar_8x32_impl.h
index aae4f35..4f9ed61 100644
--- a/src/secp256k1/src/scalar_8x32_impl.h
+++ b/src/secp256k1/src/scalar_8x32_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_IMPL_H_
-#define _SECP256K1_SCALAR_REPR_IMPL_H_
+#ifndef SECP256K1_SCALAR_REPR_IMPL_H
+#define SECP256K1_SCALAR_REPR_IMPL_H
 
 /* Limbs of the secp256k1 order. */
 #define SECP256K1_N_0 ((uint32_t)0xD0364141UL)
@@ -718,4 +718,4 @@ SECP256K1_INLINE static void secp256k1_scalar_mul_shift_var(secp256k1_scalar *r,
     secp256k1_scalar_cadd_bit(r, 0, (l[(shift - 1) >> 5] >> ((shift - 1) & 0x1f)) & 1);
 }
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_IMPL_H */
diff --git a/src/secp256k1/src/scalar_impl.h b/src/secp256k1/src/scalar_impl.h
index 2690d86..fa79057 100644
--- a/src/secp256k1/src/scalar_impl.h
+++ b/src/secp256k1/src/scalar_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_IMPL_H_
-#define _SECP256K1_SCALAR_IMPL_H_
+#ifndef SECP256K1_SCALAR_IMPL_H
+#define SECP256K1_SCALAR_IMPL_H
 
 #include "group.h"
 #include "scalar.h"
@@ -330,4 +330,4 @@ static void secp256k1_scalar_split_lambda(secp256k1_scalar *r1, secp256k1_scalar
 #endif
 #endif
 
-#endif
+#endif /* SECP256K1_SCALAR_IMPL_H */
diff --git a/src/secp256k1/src/scalar_low.h b/src/secp256k1/src/scalar_low.h
index 5574c44..5836feb 100644
--- a/src/secp256k1/src/scalar_low.h
+++ b/src/secp256k1/src/scalar_low.h
@@ -4,12 +4,12 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_
-#define _SECP256K1_SCALAR_REPR_
+#ifndef SECP256K1_SCALAR_REPR_H
+#define SECP256K1_SCALAR_REPR_H
 
 #include <stdint.h>
 
 /** A scalar modulo the group order of the secp256k1 curve. */
 typedef uint32_t secp256k1_scalar;
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_H */
diff --git a/src/secp256k1/src/scalar_low_impl.h b/src/secp256k1/src/scalar_low_impl.h
index 4f94441..c80e70c 100644
--- a/src/secp256k1/src/scalar_low_impl.h
+++ b/src/secp256k1/src/scalar_low_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_SCALAR_REPR_IMPL_H_
-#define _SECP256K1_SCALAR_REPR_IMPL_H_
+#ifndef SECP256K1_SCALAR_REPR_IMPL_H
+#define SECP256K1_SCALAR_REPR_IMPL_H
 
 #include "scalar.h"
 
@@ -111,4 +111,4 @@ SECP256K1_INLINE static int secp256k1_scalar_eq(const secp256k1_scalar *a, const
     return *a == *b;
 }
 
-#endif
+#endif /* SECP256K1_SCALAR_REPR_IMPL_H */
diff --git a/src/secp256k1/src/testrand.h b/src/secp256k1/src/testrand.h
index f8efa93..f1f9be0 100644
--- a/src/secp256k1/src/testrand.h
+++ b/src/secp256k1/src/testrand.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_TESTRAND_H_
-#define _SECP256K1_TESTRAND_H_
+#ifndef SECP256K1_TESTRAND_H
+#define SECP256K1_TESTRAND_H
 
 #if defined HAVE_CONFIG_H
 #include "libsecp256k1-config.h"
@@ -35,4 +35,4 @@ static void secp256k1_rand256_test(unsigned char *b32);
 /** Generate pseudorandom bytes with long sequences of zero and one bits. */
 static void secp256k1_rand_bytes_test(unsigned char *bytes, size_t len);
 
-#endif
+#endif /* SECP256K1_TESTRAND_H */
diff --git a/src/secp256k1/src/testrand_impl.h b/src/secp256k1/src/testrand_impl.h
index 15c7b9f..1255574 100644
--- a/src/secp256k1/src/testrand_impl.h
+++ b/src/secp256k1/src/testrand_impl.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_TESTRAND_IMPL_H_
-#define _SECP256K1_TESTRAND_IMPL_H_
+#ifndef SECP256K1_TESTRAND_IMPL_H
+#define SECP256K1_TESTRAND_IMPL_H
 
 #include <stdint.h>
 #include <string.h>
@@ -107,4 +107,4 @@ static void secp256k1_rand256_test(unsigned char *b32) {
     secp256k1_rand_bytes_test(b32, 32);
 }
 
-#endif
+#endif /* SECP256K1_TESTRAND_IMPL_H */
diff --git a/src/secp256k1/src/tests.c b/src/secp256k1/src/tests.c
index 3d9bd5e..c2e4af9 100644
--- a/src/secp256k1/src/tests.c
+++ b/src/secp256k1/src/tests.c
@@ -2091,7 +2091,7 @@ void test_add_neg_y_diff_x(void) {
      * of the sum to be wrong (since infinity has no xy coordinates).
      * HOWEVER, if the x-coordinates are different, infinity is the
      * wrong answer, and such degeneracies are exposed. This is the
-     * root of https://github.com/bitcoin-core/secp256k1/issues/257
+     * root of http:///github.com/blockchaingate/secp256k1/issues/257
      * which this test is a regression test for.
      *
      * These points were generated in sage as
diff --git a/src/secp256k1/src/util.h b/src/secp256k1/src/util.h
index 4092a86..b0441d8 100644
--- a/src/secp256k1/src/util.h
+++ b/src/secp256k1/src/util.h
@@ -4,8 +4,8 @@
  * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
  **********************************************************************/
 
-#ifndef _SECP256K1_UTIL_H_
-#define _SECP256K1_UTIL_H_
+#ifndef SECP256K1_UTIL_H
+#define SECP256K1_UTIL_H
 
 #if defined HAVE_CONFIG_H
 #include "libsecp256k1-config.h"
@@ -110,4 +110,4 @@ static SECP256K1_INLINE void *checked_malloc(const secp256k1_callback* cb, size_
 SECP256K1_GNUC_EXT typedef unsigned __int128 uint128_t;
 #endif
 
-#endif
+#endif /* SECP256K1_UTIL_H */
diff --git a/test/util/data/tt-delin1-out.json b/test/util/data/tt-delin1-out.json
index 0b3235d..de647f9 100644
--- a/test/util/data/tt-delin1-out.json
+++ b/test/util/data/tt-delin1-out.json
@@ -14,7 +14,7 @@
                 "hex": "493046022100b4251ecd63778a3dde0155abe4cd162947620ae9ee45a874353551092325b116022100db307baf4ff3781ec520bd18f387948cedd15dc27bafe17c894b0fe6ffffcafa012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "752f7f69b915637dc1c2f7aed1466ad676f6f3e24cf922809705f664e97ab3c1",
             "vout": 1,
@@ -23,7 +23,7 @@
                 "hex": "473044022079bd62ee09621a3be96b760c39e8ef78170101d46313923c6b07ae60a95c90670220238e51ea29fc70b04b65508450523caedbb11cb4dd5aa608c81487de798925ba0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b0ac9cca2e69cd02410e31b1f4402a25758e71abd1ab06c265ef9077dc05d0ed",
             "vout": 209,
@@ -32,7 +32,7 @@
                 "hex": "48304502207722d6f9038673c86a1019b1c4de2d687ae246477cd4ca7002762be0299de385022100e594a11e3a313942595f7666dcf7078bcb14f1330f4206b95c917e7ec0e82fac012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a135eafb595eaf4c1ea59ccb111cdc0eae1b2c979b226a1e5aa8b76fe2d628df",
             "vout": 0,
@@ -41,7 +41,7 @@
                 "hex": "483045022100a63a4788027b79b65c6f9d9e054f68cf3b4eed19efd82a2d53f70dcbe64683390220526f243671425b2bd05745fcf2729361f985cfe84ea80c7cfc817b93d8134374012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a5d6bf53ba21140b8a4d554feb00fe8bb9a62430ff9e4624aa2f58a120232aae",
             "vout": 1,
@@ -50,7 +50,7 @@
                 "hex": "493046022100b200ac6db16842f76dab9abe807ce423c992805879bc50abd46ed8275a59d9cf022100c0d518e85dd345b3c29dd4dc47b9a420d3ce817b18720e94966d2fe23413a408012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "1b299cf14f1a22e81ea56d71b7affbd7cf386807bf2b4d4b79a18a54125accb3",
             "vout": 0,
@@ -59,7 +59,7 @@
                 "hex": "483045022100ededc441c3103a6f2bd6cab7639421af0f6ec5e60503bce1e603cf34f00aee1c02205cb75f3f519a13fb348783b21db3085cb5ec7552c59e394fdbc3e1feea43f967012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "071df1cdcb3f0070f9d6af7b0274f02d0be2324a274727cfd288383167531485",
             "vout": 21,
@@ -68,7 +68,7 @@
                 "hex": "483045022100d9eed5413d2a4b4b98625aa6e3169edc4fb4663e7862316d69224454e70cd8ca022061e506521d5ced51dd0ea36496e75904d756a4c4f9fb111568555075d5f68d9a012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b012e500eb7adf7a13ed332dd6ece849f94f7a62bb3eac5babab356d1fc19282",
             "vout": 9,
@@ -77,7 +77,7 @@
                 "hex": "48304502207e84b27139c4c19c828cb1e30c349bba88e4d9b59be97286960793b5ddc0a2af0221008cdc7a951e7f31c20953ed5635fbabf228e80b7047f32faaa0313e7693005177012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "58840fee9c833f2f2d40575842f30f4b8d2553094d06ad88b03d06869acf3d88",
             "vout": 30,
@@ -86,7 +86,7 @@
                 "hex": "4730440220426540dfed9c4ab5812e5f06df705b8bcf307dd7d20f7fa6512298b2a6314f420220064055096e3ca62f6c7352c66a5447767c53f946acdf35025ab3807ddb2fa404012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "e69f9cd16946e570a665245354428a3f507ea69f4568b581e4af98edb3db9766",
             "vout": 114,
@@ -95,7 +95,7 @@
                 "hex": "47304402200a5e673996f2fc88e21cc8613611f08a650bc0370338803591d85d0ec5663764022040b6664a0d1ec83a7f01975b8fde5232992b8ca58bf48af6725d2f92a936ab2e012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "595d1257f654ed2cbe5a65421e8aefd2b4d70b5b6c89a03f1d7e518221fc3f02",
             "vout": 103,
@@ -104,7 +104,7 @@
                 "hex": "493046022100d93b30219c5735f673be5c3b4688366d96f545561c74cb62c6958c00f6960806022100ec8200adcb028f2184fa2a4f6faac7f8bb57cb4503bb7584ac11051fece31b3d012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "06fc818f9555a261248ecd7aad0993eafb5a82ceb2b5c87c3ddfb06671c7f816",
             "vout": 1,
@@ -113,7 +113,7 @@
                 "hex": "483045022100a13934e68d3f5b22b130c4cb33f4da468cffc52323a47fbfbe06b64858162246022047081e0a70ff770e64a2e2d31e5d520d9102268b57a47009a72fe73ec766901801210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "fb416c8155d6bb1d43f9395466ca90a638a7c2dd3ff617aadf3a7ac8f3967b19",
             "vout": 0,
@@ -122,7 +122,7 @@
                 "hex": "49304602210097f1f35d5bdc1a3a60390a1b015b8e7c4f916aa3847aafd969e04975e15bbe70022100a9052eb25517d481f1fda1b129eb1b534da50ea1a51f3ee012dca3601c11b86a0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3940b9683bd6104ad24c978e640ba4095993cafdb27d2ed91baa27ee61a2d920",
             "vout": 221,
@@ -131,7 +131,7 @@
                 "hex": "483045022012b3138c591bf7154b6fef457f2c4a3c7162225003788ac0024a99355865ff13022100b71b125ae1ffb2e1d1571f580cd3ebc8cd049a2d7a8a41f138ba94aeb982106f012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "711b5714d3b5136147c02194cd95bde94a4648c4263ca6f972d86cd1d579f150",
             "vout": 1,
@@ -140,7 +140,7 @@
                 "hex": "483045022100f834ccc8b22ee72712a3e5e6ef4acb8b2fb791b5385b70e2cd4332674d6667f4022024fbda0a997e0c253503f217501f508a4d56edce2c813ecdd9ad796dbeba907401210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "6364b5c5efe018430789e7fb4e338209546cae5d9c5f5e300aac68155d861b55",
             "vout": 27,
@@ -149,7 +149,7 @@
                 "hex": "48304502203b2fd1e39ae0e469d7a15768f262661b0de41470daf0fe8c4fd0c26542a0870002210081c57e331f9a2d214457d953e3542904727ee412c63028113635d7224da3dccc012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "0bb57f6e38012c86d4c5a28c904f2675082859147921a707d48961015a3e5057",
             "vout": 1095,
@@ -158,7 +158,7 @@
                 "hex": "48304502206947a9c54f0664ece4430fd4ae999891dc50bb6126bc36b6a15a3189f29d25e9022100a86cfc4e2fdd9e39a20e305cfd1b76509c67b3e313e0f118229105caa0e823c9012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "9b34274814a2540bb062107117f8f3e75ef85d953e9372d8261a3e9dfbc1163f",
             "vout": 37,
@@ -167,7 +167,7 @@
                 "hex": "483045022100c7128fe10b2d38744ae8177776054c29fc8ec13f07207723e70766ab7164847402201d2cf09009b9596de74c0183d1ab832e5edddb7a9965880bb400097e850850f8012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b86b5cc0d8a7374d94e277850b0a249cb26a7b42ddf014f28a49b8859da64241",
             "vout": 20,
@@ -176,7 +176,7 @@
                 "hex": "48304502203b89a71628a28cc3703d170ca3be77786cff6b867e38a18b719705f8a326578f022100b2a9879e1acf621faa6466c207746a7f3eb4c8514c1482969aba3f2a957f1321012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3d0a2353eeec44d3c10aed259038db321912122cd4150048f7bfa4c0ecfee236",
             "vout": 242,
@@ -200,7 +200,7 @@
                     "1E7SGgAZFCHDnVZLuRViX3gUmxpMfdvd2o"
                 ]
             }
-        }, 
+        },
         {
             "value": 0.01000001,
             "n": 1,
diff --git a/test/util/data/tt-delout1-out.json b/test/util/data/tt-delout1-out.json
index 5b69d0c..067ffe7 100644
--- a/test/util/data/tt-delout1-out.json
+++ b/test/util/data/tt-delout1-out.json
@@ -14,7 +14,7 @@
                 "hex": "493046022100b4251ecd63778a3dde0155abe4cd162947620ae9ee45a874353551092325b116022100db307baf4ff3781ec520bd18f387948cedd15dc27bafe17c894b0fe6ffffcafa012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a72ec96bd0d022d1b0c2f9078cdd46b3725b8eecdd001e17b21e3ababad14ecb",
             "vout": 0,
@@ -23,7 +23,7 @@
                 "hex": "493046022100a9b617843b68c284715d3e02fd120479cd0d96a6c43bf01e697fb0a460a21a3a022100ba0a12fbe8b993d4e7911fa3467615765dbe421ddf5c51b57a9c1ee19dcc00ba012103e633b4fa4ceb705c2da712390767199be8ef2448b3095dc01652e11b2b751505"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "752f7f69b915637dc1c2f7aed1466ad676f6f3e24cf922809705f664e97ab3c1",
             "vout": 1,
@@ -32,7 +32,7 @@
                 "hex": "473044022079bd62ee09621a3be96b760c39e8ef78170101d46313923c6b07ae60a95c90670220238e51ea29fc70b04b65508450523caedbb11cb4dd5aa608c81487de798925ba0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b0ac9cca2e69cd02410e31b1f4402a25758e71abd1ab06c265ef9077dc05d0ed",
             "vout": 209,
@@ -41,7 +41,7 @@
                 "hex": "48304502207722d6f9038673c86a1019b1c4de2d687ae246477cd4ca7002762be0299de385022100e594a11e3a313942595f7666dcf7078bcb14f1330f4206b95c917e7ec0e82fac012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a135eafb595eaf4c1ea59ccb111cdc0eae1b2c979b226a1e5aa8b76fe2d628df",
             "vout": 0,
@@ -50,7 +50,7 @@
                 "hex": "483045022100a63a4788027b79b65c6f9d9e054f68cf3b4eed19efd82a2d53f70dcbe64683390220526f243671425b2bd05745fcf2729361f985cfe84ea80c7cfc817b93d8134374012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a5d6bf53ba21140b8a4d554feb00fe8bb9a62430ff9e4624aa2f58a120232aae",
             "vout": 1,
@@ -59,7 +59,7 @@
                 "hex": "493046022100b200ac6db16842f76dab9abe807ce423c992805879bc50abd46ed8275a59d9cf022100c0d518e85dd345b3c29dd4dc47b9a420d3ce817b18720e94966d2fe23413a408012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "1b299cf14f1a22e81ea56d71b7affbd7cf386807bf2b4d4b79a18a54125accb3",
             "vout": 0,
@@ -68,7 +68,7 @@
                 "hex": "483045022100ededc441c3103a6f2bd6cab7639421af0f6ec5e60503bce1e603cf34f00aee1c02205cb75f3f519a13fb348783b21db3085cb5ec7552c59e394fdbc3e1feea43f967012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "071df1cdcb3f0070f9d6af7b0274f02d0be2324a274727cfd288383167531485",
             "vout": 21,
@@ -77,7 +77,7 @@
                 "hex": "483045022100d9eed5413d2a4b4b98625aa6e3169edc4fb4663e7862316d69224454e70cd8ca022061e506521d5ced51dd0ea36496e75904d756a4c4f9fb111568555075d5f68d9a012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b012e500eb7adf7a13ed332dd6ece849f94f7a62bb3eac5babab356d1fc19282",
             "vout": 9,
@@ -86,7 +86,7 @@
                 "hex": "48304502207e84b27139c4c19c828cb1e30c349bba88e4d9b59be97286960793b5ddc0a2af0221008cdc7a951e7f31c20953ed5635fbabf228e80b7047f32faaa0313e7693005177012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "58840fee9c833f2f2d40575842f30f4b8d2553094d06ad88b03d06869acf3d88",
             "vout": 30,
@@ -95,7 +95,7 @@
                 "hex": "4730440220426540dfed9c4ab5812e5f06df705b8bcf307dd7d20f7fa6512298b2a6314f420220064055096e3ca62f6c7352c66a5447767c53f946acdf35025ab3807ddb2fa404012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "e69f9cd16946e570a665245354428a3f507ea69f4568b581e4af98edb3db9766",
             "vout": 114,
@@ -104,7 +104,7 @@
                 "hex": "47304402200a5e673996f2fc88e21cc8613611f08a650bc0370338803591d85d0ec5663764022040b6664a0d1ec83a7f01975b8fde5232992b8ca58bf48af6725d2f92a936ab2e012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "595d1257f654ed2cbe5a65421e8aefd2b4d70b5b6c89a03f1d7e518221fc3f02",
             "vout": 103,
@@ -113,7 +113,7 @@
                 "hex": "493046022100d93b30219c5735f673be5c3b4688366d96f545561c74cb62c6958c00f6960806022100ec8200adcb028f2184fa2a4f6faac7f8bb57cb4503bb7584ac11051fece31b3d012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "06fc818f9555a261248ecd7aad0993eafb5a82ceb2b5c87c3ddfb06671c7f816",
             "vout": 1,
@@ -122,7 +122,7 @@
                 "hex": "483045022100a13934e68d3f5b22b130c4cb33f4da468cffc52323a47fbfbe06b64858162246022047081e0a70ff770e64a2e2d31e5d520d9102268b57a47009a72fe73ec766901801210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "fb416c8155d6bb1d43f9395466ca90a638a7c2dd3ff617aadf3a7ac8f3967b19",
             "vout": 0,
@@ -131,7 +131,7 @@
                 "hex": "49304602210097f1f35d5bdc1a3a60390a1b015b8e7c4f916aa3847aafd969e04975e15bbe70022100a9052eb25517d481f1fda1b129eb1b534da50ea1a51f3ee012dca3601c11b86a0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3940b9683bd6104ad24c978e640ba4095993cafdb27d2ed91baa27ee61a2d920",
             "vout": 221,
@@ -140,7 +140,7 @@
                 "hex": "483045022012b3138c591bf7154b6fef457f2c4a3c7162225003788ac0024a99355865ff13022100b71b125ae1ffb2e1d1571f580cd3ebc8cd049a2d7a8a41f138ba94aeb982106f012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "711b5714d3b5136147c02194cd95bde94a4648c4263ca6f972d86cd1d579f150",
             "vout": 1,
@@ -149,7 +149,7 @@
                 "hex": "483045022100f834ccc8b22ee72712a3e5e6ef4acb8b2fb791b5385b70e2cd4332674d6667f4022024fbda0a997e0c253503f217501f508a4d56edce2c813ecdd9ad796dbeba907401210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "6364b5c5efe018430789e7fb4e338209546cae5d9c5f5e300aac68155d861b55",
             "vout": 27,
@@ -158,7 +158,7 @@
                 "hex": "48304502203b2fd1e39ae0e469d7a15768f262661b0de41470daf0fe8c4fd0c26542a0870002210081c57e331f9a2d214457d953e3542904727ee412c63028113635d7224da3dccc012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "0bb57f6e38012c86d4c5a28c904f2675082859147921a707d48961015a3e5057",
             "vout": 1095,
@@ -167,7 +167,7 @@
                 "hex": "48304502206947a9c54f0664ece4430fd4ae999891dc50bb6126bc36b6a15a3189f29d25e9022100a86cfc4e2fdd9e39a20e305cfd1b76509c67b3e313e0f118229105caa0e823c9012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "9b34274814a2540bb062107117f8f3e75ef85d953e9372d8261a3e9dfbc1163f",
             "vout": 37,
@@ -176,7 +176,7 @@
                 "hex": "483045022100c7128fe10b2d38744ae8177776054c29fc8ec13f07207723e70766ab7164847402201d2cf09009b9596de74c0183d1ab832e5edddb7a9965880bb400097e850850f8012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b86b5cc0d8a7374d94e277850b0a249cb26a7b42ddf014f28a49b8859da64241",
             "vout": 20,
@@ -185,7 +185,7 @@
                 "hex": "48304502203b89a71628a28cc3703d170ca3be77786cff6b867e38a18b719705f8a326578f022100b2a9879e1acf621faa6466c207746a7f3eb4c8514c1482969aba3f2a957f1321012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3d0a2353eeec44d3c10aed259038db321912122cd4150048f7bfa4c0ecfee236",
             "vout": 242,
diff --git a/test/util/data/tt-locktime317000-out.json b/test/util/data/tt-locktime317000-out.json
index cf1ebcd..af7903d 100644
--- a/test/util/data/tt-locktime317000-out.json
+++ b/test/util/data/tt-locktime317000-out.json
@@ -14,7 +14,7 @@
                 "hex": "493046022100b4251ecd63778a3dde0155abe4cd162947620ae9ee45a874353551092325b116022100db307baf4ff3781ec520bd18f387948cedd15dc27bafe17c894b0fe6ffffcafa012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a72ec96bd0d022d1b0c2f9078cdd46b3725b8eecdd001e17b21e3ababad14ecb",
             "vout": 0,
@@ -23,7 +23,7 @@
                 "hex": "493046022100a9b617843b68c284715d3e02fd120479cd0d96a6c43bf01e697fb0a460a21a3a022100ba0a12fbe8b993d4e7911fa3467615765dbe421ddf5c51b57a9c1ee19dcc00ba012103e633b4fa4ceb705c2da712390767199be8ef2448b3095dc01652e11b2b751505"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "752f7f69b915637dc1c2f7aed1466ad676f6f3e24cf922809705f664e97ab3c1",
             "vout": 1,
@@ -32,7 +32,7 @@
                 "hex": "473044022079bd62ee09621a3be96b760c39e8ef78170101d46313923c6b07ae60a95c90670220238e51ea29fc70b04b65508450523caedbb11cb4dd5aa608c81487de798925ba0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b0ac9cca2e69cd02410e31b1f4402a25758e71abd1ab06c265ef9077dc05d0ed",
             "vout": 209,
@@ -41,7 +41,7 @@
                 "hex": "48304502207722d6f9038673c86a1019b1c4de2d687ae246477cd4ca7002762be0299de385022100e594a11e3a313942595f7666dcf7078bcb14f1330f4206b95c917e7ec0e82fac012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a135eafb595eaf4c1ea59ccb111cdc0eae1b2c979b226a1e5aa8b76fe2d628df",
             "vout": 0,
@@ -50,7 +50,7 @@
                 "hex": "483045022100a63a4788027b79b65c6f9d9e054f68cf3b4eed19efd82a2d53f70dcbe64683390220526f243671425b2bd05745fcf2729361f985cfe84ea80c7cfc817b93d8134374012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "a5d6bf53ba21140b8a4d554feb00fe8bb9a62430ff9e4624aa2f58a120232aae",
             "vout": 1,
@@ -59,7 +59,7 @@
                 "hex": "493046022100b200ac6db16842f76dab9abe807ce423c992805879bc50abd46ed8275a59d9cf022100c0d518e85dd345b3c29dd4dc47b9a420d3ce817b18720e94966d2fe23413a408012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "1b299cf14f1a22e81ea56d71b7affbd7cf386807bf2b4d4b79a18a54125accb3",
             "vout": 0,
@@ -68,7 +68,7 @@
                 "hex": "483045022100ededc441c3103a6f2bd6cab7639421af0f6ec5e60503bce1e603cf34f00aee1c02205cb75f3f519a13fb348783b21db3085cb5ec7552c59e394fdbc3e1feea43f967012103a621f08be22d1bbdcbe4e527ee4927006aa555fc65e2aafa767d4ea2fe9dfa52"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "071df1cdcb3f0070f9d6af7b0274f02d0be2324a274727cfd288383167531485",
             "vout": 21,
@@ -77,7 +77,7 @@
                 "hex": "483045022100d9eed5413d2a4b4b98625aa6e3169edc4fb4663e7862316d69224454e70cd8ca022061e506521d5ced51dd0ea36496e75904d756a4c4f9fb111568555075d5f68d9a012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b012e500eb7adf7a13ed332dd6ece849f94f7a62bb3eac5babab356d1fc19282",
             "vout": 9,
@@ -86,7 +86,7 @@
                 "hex": "48304502207e84b27139c4c19c828cb1e30c349bba88e4d9b59be97286960793b5ddc0a2af0221008cdc7a951e7f31c20953ed5635fbabf228e80b7047f32faaa0313e7693005177012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "58840fee9c833f2f2d40575842f30f4b8d2553094d06ad88b03d06869acf3d88",
             "vout": 30,
@@ -95,7 +95,7 @@
                 "hex": "4730440220426540dfed9c4ab5812e5f06df705b8bcf307dd7d20f7fa6512298b2a6314f420220064055096e3ca62f6c7352c66a5447767c53f946acdf35025ab3807ddb2fa404012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "e69f9cd16946e570a665245354428a3f507ea69f4568b581e4af98edb3db9766",
             "vout": 114,
@@ -104,7 +104,7 @@
                 "hex": "47304402200a5e673996f2fc88e21cc8613611f08a650bc0370338803591d85d0ec5663764022040b6664a0d1ec83a7f01975b8fde5232992b8ca58bf48af6725d2f92a936ab2e012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "595d1257f654ed2cbe5a65421e8aefd2b4d70b5b6c89a03f1d7e518221fc3f02",
             "vout": 103,
@@ -113,7 +113,7 @@
                 "hex": "493046022100d93b30219c5735f673be5c3b4688366d96f545561c74cb62c6958c00f6960806022100ec8200adcb028f2184fa2a4f6faac7f8bb57cb4503bb7584ac11051fece31b3d012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "06fc818f9555a261248ecd7aad0993eafb5a82ceb2b5c87c3ddfb06671c7f816",
             "vout": 1,
@@ -122,7 +122,7 @@
                 "hex": "483045022100a13934e68d3f5b22b130c4cb33f4da468cffc52323a47fbfbe06b64858162246022047081e0a70ff770e64a2e2d31e5d520d9102268b57a47009a72fe73ec766901801210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "fb416c8155d6bb1d43f9395466ca90a638a7c2dd3ff617aadf3a7ac8f3967b19",
             "vout": 0,
@@ -131,7 +131,7 @@
                 "hex": "49304602210097f1f35d5bdc1a3a60390a1b015b8e7c4f916aa3847aafd969e04975e15bbe70022100a9052eb25517d481f1fda1b129eb1b534da50ea1a51f3ee012dca3601c11b86a0121027a759be8df971a6a04fafcb4f6babf75dc811c5cdaa0734cddbe9b942ce75b34"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3940b9683bd6104ad24c978e640ba4095993cafdb27d2ed91baa27ee61a2d920",
             "vout": 221,
@@ -140,7 +140,7 @@
                 "hex": "483045022012b3138c591bf7154b6fef457f2c4a3c7162225003788ac0024a99355865ff13022100b71b125ae1ffb2e1d1571f580cd3ebc8cd049a2d7a8a41f138ba94aeb982106f012103091137f3ef23f4acfc19a5953a68b2074fae942ad3563ef28c33b0cac9a93adc"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "711b5714d3b5136147c02194cd95bde94a4648c4263ca6f972d86cd1d579f150",
             "vout": 1,
@@ -149,7 +149,7 @@
                 "hex": "483045022100f834ccc8b22ee72712a3e5e6ef4acb8b2fb791b5385b70e2cd4332674d6667f4022024fbda0a997e0c253503f217501f508a4d56edce2c813ecdd9ad796dbeba907401210234b9d9413f247bb78cd3293b7b65a2c38018ba5621ea9ee737f3a6a3523fb4cd"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "6364b5c5efe018430789e7fb4e338209546cae5d9c5f5e300aac68155d861b55",
             "vout": 27,
@@ -158,7 +158,7 @@
                 "hex": "48304502203b2fd1e39ae0e469d7a15768f262661b0de41470daf0fe8c4fd0c26542a0870002210081c57e331f9a2d214457d953e3542904727ee412c63028113635d7224da3dccc012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "0bb57f6e38012c86d4c5a28c904f2675082859147921a707d48961015a3e5057",
             "vout": 1095,
@@ -167,7 +167,7 @@
                 "hex": "48304502206947a9c54f0664ece4430fd4ae999891dc50bb6126bc36b6a15a3189f29d25e9022100a86cfc4e2fdd9e39a20e305cfd1b76509c67b3e313e0f118229105caa0e823c9012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "9b34274814a2540bb062107117f8f3e75ef85d953e9372d8261a3e9dfbc1163f",
             "vout": 37,
@@ -176,7 +176,7 @@
                 "hex": "483045022100c7128fe10b2d38744ae8177776054c29fc8ec13f07207723e70766ab7164847402201d2cf09009b9596de74c0183d1ab832e5edddb7a9965880bb400097e850850f8012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "b86b5cc0d8a7374d94e277850b0a249cb26a7b42ddf014f28a49b8859da64241",
             "vout": 20,
@@ -185,7 +185,7 @@
                 "hex": "48304502203b89a71628a28cc3703d170ca3be77786cff6b867e38a18b719705f8a326578f022100b2a9879e1acf621faa6466c207746a7f3eb4c8514c1482969aba3f2a957f1321012103f1575d6124ac78be398c25b31146d08313c6072d23a4d7df5ac6a9f87346c64c"
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "3d0a2353eeec44d3c10aed259038db321912122cd4150048f7bfa4c0ecfee236",
             "vout": 242,
@@ -209,7 +209,7 @@
                     "1E7SGgAZFCHDnVZLuRViX3gUmxpMfdvd2o"
                 ]
             }
-        }, 
+        },
         {
             "value": 0.01000001,
             "n": 1,
diff --git a/test/util/data/txcreate1.json b/test/util/data/txcreate1.json
index edb091f..83a8664 100644
--- a/test/util/data/txcreate1.json
+++ b/test/util/data/txcreate1.json
@@ -14,7 +14,7 @@
                 "hex": ""
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "bf829c6bcf84579331337659d31f89dfd138f7f7785802d5501c92333145ca7c",
             "vout": 18,
@@ -23,7 +23,7 @@
                 "hex": ""
             },
             "sequence": 4294967295
-        }, 
+        },
         {
             "txid": "22a6f904655d53ae2ff70e701a0bbd90aa3975c0f40bfc6cc996a9049e31cdfc",
             "vout": 1,
@@ -47,7 +47,7 @@
                     "13tuJJDR2RgArmgfv6JScSdreahzgc4T6o"
                 ]
             }
-        }, 
+        },
         {
             "value": 4.00000000,
             "n": 1,
diff --git a/test/util/data/txcreatedata1.json b/test/util/data/txcreatedata1.json
index e66a6bb..15a4246 100644
--- a/test/util/data/txcreatedata1.json
+++ b/test/util/data/txcreatedata1.json
@@ -29,7 +29,7 @@
                     "13tuJJDR2RgArmgfv6JScSdreahzgc4T6o"
                 ]
             }
-        }, 
+        },
         {
             "value": 4.00000000,
             "n": 1,
diff --git a/test/util/data/txcreatedata2.json b/test/util/data/txcreatedata2.json
index 0f8edca..cb93c27 100644
--- a/test/util/data/txcreatedata2.json
+++ b/test/util/data/txcreatedata2.json
@@ -29,7 +29,7 @@
                     "13tuJJDR2RgArmgfv6JScSdreahzgc4T6o"
                 ]
             }
-        }, 
+        },
         {
             "value": 0.00000000,
             "n": 1,
diff --git a/test/util/data/txcreatedata_seq1.json b/test/util/data/txcreatedata_seq1.json
index 771ff1b..dea48ba 100644
--- a/test/util/data/txcreatedata_seq1.json
+++ b/test/util/data/txcreatedata_seq1.json
@@ -14,7 +14,7 @@
                 "hex": ""
             },
             "sequence": 4294967293
-        }, 
+        },
         {
             "txid": "5897de6bd6027a475eadd57019d4e6872c396d0716c4875a5f1a6fcfdf385c1f",
             "vout": 0,
diff --git a/test/util/data/txcreatemultisig1.json b/test/util/data/txcreatemultisig1.json
index 7c814da..72e20c8 100644
--- a/test/util/data/txcreatemultisig1.json
+++ b/test/util/data/txcreatemultisig1.json
@@ -17,8 +17,8 @@
                 "reqSigs": 2,
                 "type": "multisig",
                 "addresses": [
-                    "1FoG2386FG2tAJS9acMuiDsKy67aGg9MKz", 
-                    "1FXtz9KU8JNmQDyHdiEm5HDiALuP3zdHvV", 
+                    "1FoG2386FG2tAJS9acMuiDsKy67aGg9MKz",
+                    "1FXtz9KU8JNmQDyHdiEm5HDiALuP3zdHvV",
                     "14LuavcBbXZYJ6Tsz3cAUQj9SuQoL2xCQX"
                 ]
             }
diff --git a/test/util/fabcoin-util-test.py b/test/util/fabcoin-util-test.py
old mode 100644
new mode 100755
index 9081cb4..2ec76c2
--- a/test/util/fabcoin-util-test.py
+++ b/test/util/fabcoin-util-test.py
@@ -48,7 +48,7 @@ def main():
 
 def bctester(testDir, input_basename, buildenv):
     """ Loads and parses the input file, runs all tests and reports results"""
-    input_filename = testDir + "/" + input_basename
+    input_filename = os.path.join(testDir, input_basename)
     raw_data = open(input_filename).read()
     input_data = json.loads(raw_data)
 
@@ -77,7 +77,7 @@ def bctest(testDir, testObj, buildenv):
     are not as expected. Error is caught by bctester() and reported.
     """
     # Get the exec names and arguments
-    execprog = buildenv["BUILDDIR"] + "/src/" + testObj['exec'] + buildenv["EXEEXT"]
+    execprog = os.path.join(buildenv["BUILDDIR"], "src", testObj["exec"] + buildenv["EXEEXT"])
     execargs = testObj['args']
     execrun = [execprog] + execargs
 
@@ -85,24 +85,28 @@ def bctest(testDir, testObj, buildenv):
     stdinCfg = None
     inputData = None
     if "input" in testObj:
-        filename = testDir + "/" + testObj['input']
+        filename = os.path.join(testDir, testObj["input"])
         inputData = open(filename).read()
         stdinCfg = subprocess.PIPE
 
     # Read the expected output data (if there is any)
     outputFn = None
     outputData = None
+    outputType = None
     if "output_cmp" in testObj:
         outputFn = testObj['output_cmp']
         outputType = os.path.splitext(outputFn)[1][1:]  # output type from file extension (determines how to compare)
         try:
-            outputData = open(testDir + "/" + outputFn).read()
+            outputData = open(os.path.join(testDir, outputFn)).read()
         except:
             logging.error("Output file " + outputFn + " can not be opened")
             raise
         if not outputData:
             logging.error("Output data missing for " + outputFn)
             raise Exception
+        if not outputType:
+            logging.error("Output file %s does not have a file extension" % outputFn)
+            raise Exception
 
     # Run the test
     proc = subprocess.Popen(execrun, stdin=stdinCfg, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
