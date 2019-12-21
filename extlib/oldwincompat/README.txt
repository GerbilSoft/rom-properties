These stubs are provided to allow use of MSVC 2010 and later to compile
working binaries for Windows 2000 and possibly earlier versions of Windows.

These stubs require static CRT linkage.

These stubs should NOT be linked for either of the following:
- Release distributions: Using these stubs implicitly reduces the security
  of the program due to the reimplementation of security functions, e.g.
  EncodePointer() and DecodePointer().
- 64-bit: The minimum for AMD64 Windows is Windows Server 2003, so there
  is no point in using these stubs on 64-bit. Also, the assembly stubs are
  written for 32-bit only.

References:
- https://stackoverflow.com/questions/19516796/visual-studio-2012-win32-project-targeting-windows-2000/53548116
- https://stackoverflow.com/a/53548116

Some changes were made compared to the original:

- Reformatted the code.
- Use C, not C++.
