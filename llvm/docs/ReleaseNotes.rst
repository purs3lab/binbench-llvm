============================
LLVM |release| Release Notes
============================

.. contents::
    :local:

.. only:: PreRelease

  .. warning::
     These are in-progress notes for the upcoming LLVM |version| release.
     Release notes for previous releases can be found on
     `the Download Page <https://releases.llvm.org/download.html>`_.


Introduction
============

This document contains the release notes for the LLVM Compiler Infrastructure,
release |release|.  Here we describe the status of LLVM, including major improvements
from the previous release, improvements in various subprojects of LLVM, and
some of the current users of the code.  All LLVM releases may be downloaded
from the `LLVM releases web site <https://llvm.org/releases/>`_.

For more information about LLVM, including information about the latest
release, please check out the `main LLVM web site <https://llvm.org/>`_.  If you
have questions or comments, the `LLVM Developer's Mailing List
<https://lists.llvm.org/mailman/listinfo/llvm-dev>`_ is a good place to send
them.

Note that if you are reading this file from a Git checkout or the main
LLVM web page, this document applies to the *next* release, not the current
one.  To see the release notes for a specific release, please see the `releases
page <https://llvm.org/releases/>`_.

Non-comprehensive list of changes in this release
=================================================
.. NOTE
   For small 1-3 sentence descriptions, just add an entry at the end of
   this list. If your description won't fit comfortably in one bullet
   point (e.g. maybe you would like to give an example of the
   functionality, or simply have a lot to talk about), see the `NOTE` below
   for adding a new subsection.

* ...

Update on required toolchains to build LLVM
-------------------------------------------

With LLVM 15.x we will raise the version requirements of the toolchain used
to build LLVM. The new requirements are as follows:

* GCC >= 7.1
* Clang >= 5.0
* Apple Clang >= 9.3
* Visual Studio 2019 >= 16.7

In LLVM 15.x these requirements will be "soft" requirements and the version
check can be skipped by passing ``-DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON``
to CMake.

With the release of LLVM 16.x these requirements will be hard and LLVM developers
can start using C++17 features, making it impossible to build with older
versions of these toolchains.

Changes to the LLVM IR
----------------------

* LLVM now uses `opaque pointers <OpaquePointers.html>`__. This means that
  different pointer types like ``i8*``, ``i32*`` or ``void()**`` are now
  represented as a single ``ptr`` type. See the linked document for migration
  instructions.
* Renamed ``llvm.experimental.vector.extract`` intrinsic to ``llvm.vector.extract``.
* Renamed ``llvm.experimental.vector.insert`` intrinsic to ``llvm.vector.insert``.
* The constant expression variants of the following instructions have been
  removed:

  * ``extractvalue``
  * ``insertvalue``
  * ``udiv``
  * ``sdiv``
  * ``urem``
  * ``srem``
  * ``fadd``
  * ``fsub``
  * ``fmul``
  * ``fdiv``
  * ``frem``

* Added the support for ``fmax`` and ``fmin`` in ``atomicrmw`` instruction. The
  comparison is expected to match the behavior of ``llvm.maxnum.*`` and
  ``llvm.minnum.*`` respectively.
* ``callbr`` instructions no longer use ``blockaddress`` arguments for labels.
  Instead, label constraints starting with ``!`` refer directly to entries in
  the ``callbr`` indirect destination list.

.. code-block:: llvm

    ; Old representation
    %res = callbr i32 asm "", "=r,r,i"(i32 %x, i8 *blockaddress(@foo, %indirect))
          to label %fallthrough [label %indirect]
    ; New representation
    %res = callbr i32 asm "", "=r,r,!i"(i32 %x)
          to label %fallthrough [label %indirect]

Changes to building LLVM
------------------------

* Omitting ``CMAKE_BUILD_TYPE`` when using a single configuration generator is now
  an error. You now have to pass ``-DCMAKE_BUILD_TYPE=<type>`` in order to configure
  LLVM. This is done to help new users of LLVM select the correct type: since building
  LLVM in Debug mode is very resource intensive, we want to make sure that new users
  make the choice that lines up with their usage. We have also improved documentation
  around this setting that should help new users. You can find this documentation
  `here <https://llvm.org/docs/CMake.html#cmake-build-type>`_.

Changes to TableGen
-------------------

Changes to Loop Optimizations
-----------------------------

* Loop interchange legality and cost model improvements


Changes to the AArch64 Backend
------------------------------

Changes to the AMDGPU Backend
-----------------------------

* 8 and 16-bit atomic loads and stores are now supported


Changes to the ARM Backend
--------------------------

* Added support for the Armv9-A, Armv9.1-A and Armv9.2-A architectures.
* Added support for the Armv8.1-M PACBTI-M extension.
* Added support for the Armv9-A, Armv9.1-A and Armv9.2-A architectures.
* Added support for the Armv8.1-M PACBTI-M extension.
* Removed the deprecation of ARMv8-A T32 Complex IT blocks. No deprecation
  warnings will be generated and -mrestrict-it is now always off by default.
  Previously it was on by default for Armv8 and off for all other architecture
  versions.
* Added a pass to workaround Cortex-A57 Erratum 1742098 and Cortex-A72
  Erratum 1655431. This is enabled by default when targeting either CPU.
* Implemented generation of Windows SEH unwind information.
* Switched the MinGW target to use SEH instead of DWARF for unwind information.
* Added support for the Cortex-M85 CPU.
* Added support for a new ``-mframe-chain=(none|aapcs|aapcs+leaf)`` command-line
  option, which controls the generation of AAPCS-compliant Frame Records.

Changes to the AVR Backend
--------------------------

* ...

Changes to the DirectX Backend
------------------------------

* DirectX has been added as an experimental target. Specify
  ``-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=DirectX`` in your CMake configuration
  to enable it. The target is not packaged in pre-built binaries.
* The DirectX backend supports the ``dxil`` architecture which is based on LLVM
  3.6 IR encoded as bitcode and is the format used for DirectX GPU Shader
  programs.

Changes to the Hexagon Backend
------------------------------

* ...

Changes to the MIPS Backend
---------------------------

* ...

Changes to the PowerPC Backend
------------------------------

Common PowerPC improvements:
* Add a new post instruction selection pass to generate CTR loops.
* Add SSE4 and BMI compatible intrinsics implementation.
* Supported 16-byte lock free atomics on PowerPC8 and up.
* Supported atomic load/store for pointer types.
* Supported stack size larger than 2G
* Add __builtin_min/__builtin_max/__abs builtins.
* Code generation improvements for splat load/vector shuffle/mulli, etc.
* Emit VSX instructions for vector loads and stores regardless of alignment.
* The mcpu=future has its own ISA now (FutureISA).
* Added the ppc-set-dscr option to set the Data Stream Control Register (DSCR).
* Bug fixes.

AIX improvements:
* Supported 64 bit XCOFF for integrated-as path.
* Supported X86-compatible vector intrinsics.
* Program code csect default alignment now is 32-byte.
* Supported auxiliary header in integrated-as path.
* Improved alias symbol handling.

Changes to the RISC-V Backend
-----------------------------

* The Zvfh extension was added.

Changes to the WebAssembly Backend
----------------------------------

* ...

Changes to the X86 Backend
--------------------------

* Support ``half`` type on SSE2 and above targets following X86 psABI.
* Support ``rdpru`` instruction on Zen2 and above targets.

During this release, ``half`` type has an ABI breaking change to provide the
support for the ABI of ``_Float16`` type on SSE2 and above following X86 psABI.
(`D107082 <https://reviews.llvm.org/D107082>`_)

The change may affect the current use of ``half`` includes (but is not limited
to):

* Frontends generating ``half`` type in function passing and/or returning
  arguments.
* Downstream runtimes providing any ``half`` conversion builtins assuming the
  old ABI.
* Projects built with LLVM 15.0 but using early versions of compiler-rt.

When you find failures with ``half`` type, check the calling conversion of the
code and switch it to the new ABI.

Changes to the OCaml bindings
-----------------------------


Changes to the C API
--------------------

* Add ``LLVMGetCastOpcode`` function to aid users of ``LLVMBuildCast`` in
  resolving the best cast operation given a source value and destination type.
  This function is a direct wrapper of ``CastInst::getCastOpcode``.

* Add ``LLVMGetAggregateElement`` function as a wrapper for
  ``Constant::getAggregateElement``, which can be used to fetch an element of a
  constant struct, array or vector, independently of the underlying
  representation. The ``LLVMGetElementAsConstant`` function is deprecated in
  favor of the new function, which works on all constant aggregates, rather than
  only instances of ``ConstantDataSequential``.

* The following functions for creating constant expressions have been removed,
  because the underlying constant expressions are no longer supported. Instead,
  an instruction should be created using the ``LLVMBuildXYZ`` APIs, which will
  constant fold the operands if possible and create an instruction otherwise:

  * ``LLVMConstExtractValue``
  * ``LLVMConstInsertValue``
  * ``LLVMConstUDiv``
  * ``LLVMConstExactUDiv``
  * ``LLVMConstSDiv``
  * ``LLVMConstExactSDiv``
  * ``LLVMConstURem``
  * ``LLVMConstSRem``
  * ``LLVMConstFAdd``
  * ``LLVMConstFSub``
  * ``LLVMConstFMul``
  * ``LLVMConstFDiv``
  * ``LLVMConstFRem``

* Add ``LLVMDeleteInstruction`` function which allows deleting instructions that
  are not inserted into a basic block.

* As part of the opaque pointer migration, the following APIs are deprecated and
  will be removed in the next release:

  * ``LLVMBuildLoad`` -> ``LLVMBuildLoad2``
  * ``LLVMBuildCall`` -> ``LLVMBuildCall2``
  * ``LLVMBuildInvoke`` -> ``LLVMBuildInvoke2``
  * ``LLVMBuildGEP`` -> ``LLVMBuildGEP2``
  * ``LLVMBuildInBoundsGEP`` -> ``LLVMBuildInBoundsGEP2``
  * ``LLVMBuildStructGEP`` -> ``LLVMBuildStructGEP2``
  * ``LLVMBuildPtrDiff`` -> ``LLVMBuildPtrDiff2``
  * ``LLVMConstGEP`` -> ``LLVMConstGEP2``
  * ``LLVMConstInBoundsGEP`` -> ``LLVMConstInBoundsGEP2``
  * ``LLVMAddAlias`` -> ``LLVMAddAlias2``

* Refactor compression namespaces across the project, making way for a possible
  introduction of alternatives to zlib compression in the llvm toolchain.
  Changes are as follows:

  * Relocate the ``llvm::zlib`` namespace to ``llvm::compression::zlib``.
  * Remove crc32 from zlib compression namespace, people should use the ``llvm::crc32`` instead.

Changes to the Go bindings
--------------------------


Changes to the FastISel infrastructure
--------------------------------------

* ...

Changes to the DAG infrastructure
---------------------------------


Changes to the Metadata Info
---------------------------------

* Add Module Flags Metadata ``stack-protector-guard-symbol`` which specify a
  symbol for addressing the stack-protector guard.

Changes to the Debug Info
---------------------------------

During this release ...

Changes to the LLVM tools
---------------------------------

* (Experimental) :doc:`llvm-symbolizer <CommandGuide/llvm-symbolizer>` now has ``--filter-markup`` to
  filter :doc:`Symbolizer Markup </SymbolizerMarkupFormat>` into human-readable
  form.
* :doc:`llvm-objcopy <CommandGuide/llvm-objcopy>` has removed support for the legacy ``zlib-gnu`` format.
* :doc:`llvm-objcopy <CommandGuide/llvm-objcopy>` now allows ``--set-section-flags src=... --rename-section src=tst``.
  ``--add-section=.foo1=... --rename-section=.foo1=.foo2`` now adds ``.foo1`` instead of ``.foo2``.
* New features supported on AIX for ``llvm-ar``:

  * AIX big-format archive write operation (`D123949 <https://reviews.llvm.org/D123949>`_)

  * A new object mode option, ``-X`` , to specify the type of object file ``llvm-ar`` should operate upon (`D127864 <https://reviews.llvm.org/D127864>`_)

  * Read global symbols of AIX big archive (`D124865 <https://reviews.llvm.org/D124865>`_)

* New options supported for ``llvm-nm``:

  * ``-X``, to specify the type of object file that ``llvm-nm`` should examine (`D118193 <https://reviews.llvm.org/D118193>`_)

  * ``--export-symbols``, to create a list of symbols to export (`D112735 <https://reviews.llvm.org/D112735>`_)

* The LLVM gold plugin now ignores bitcode from the ``.llvmbc`` section of ELF
  files when doing LTO.  https://github.com/llvm/llvm-project/issues/47216
* llvm-objcopy now supports 32 bit XCOFF.
* llvm-objdump: improved assembly printing for XCOFF.
* llc now parses code-model attribute from input file.

Changes to LLDB
---------------------------------

* The "memory region" command now has a "--all" option to list all
  memory regions (including unmapped ranges). This is the equivalent
  of using address 0 then repeating the command until all regions
  have been listed.
* Added "--show-tags" option to the "memory find" command. This is off by default.
  When enabled, if the target value is found in tagged memory, the tags for that
  memory will be shown inline with the memory contents.
* Various memory related parts of LLDB have been updated to handle
  non-address bits (such as AArch64 pointer signatures):

  * "memory read", "memory write" and "memory find" can now be used with
    addresses with non-address bits.
  * All the read and write memory methods on SBProccess and SBTarget can
    be used with addreses with non-address bits.
  * When printing a pointer expression, LLDB can now dereference the result
    even if it has non-address bits.
  * The memory cache now ignores non-address bits when looking up memory
    locations. This prevents us reading locations multiple times, or not
    writing out new values if the addresses have different non-address bits.

* LLDB now supports reading memory tags from AArch64 Linux core files.

* LLDB now supports the gnu debuglink section for reading debug information
  from a separate file on Windows

* LLDB now allows selecting the C++ ABI to use on Windows (between Itanium,
  used for MingW, and MSVC) via the ``plugin.object-file.pe-coff.abi`` setting.
  In Windows builds of LLDB, this defaults to the style used for LLVM's default
  target.

Changes to Sanitizers
---------------------


Other Changes
-------------
* The code for the `LLVM Visual Studio integration
  <https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.llvm-toolchain>`_
  has been removed. This had been obsolete and abandoned since Visual Studio
  started including an integration by default in 2019.

* Added the unwinder, personality, and helper functions for exception handling
  on AIX. (`D100132 <https://reviews.llvm.org/D100132>`_)
  (`D100504 <https://reviews.llvm.org/D100504>`_)

* PGO on AIX: A new implementation that requires linker support
  (__start_SECTION/__stop_SECTION symbols) available on AIX 7.2 TL5 SP4 and
  AIX 7.3 TL0 SP2.

External Open Source Projects Using LLVM 15
===========================================

* A project...

Additional Information
======================

A wide variety of additional information is available on the `LLVM web page
<https://llvm.org/>`_, in particular in the `documentation
<https://llvm.org/docs/>`_ section.  The web page also contains versions of the
API documentation which is up-to-date with the Git version of the source
code.  You can access versions of these documents specific to this release by
going into the ``llvm/docs/`` directory in the LLVM tree.

If you have any questions or comments about LLVM, please feel free to contact
us via the `mailing lists <https://llvm.org/docs/#mailing-lists>`_.
