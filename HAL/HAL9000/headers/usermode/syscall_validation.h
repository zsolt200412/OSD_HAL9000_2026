#pragma once

#include "syscall_no.h"

BOOLEAN
SyscallGetParameterCount(
    IN      SYSCALL_ID      SyscallId,
    OUT     DWORD*          ParameterCount
    );

STATUS
SyscallValidateParameterArea(
    IN      PVOID           Parameters,
    IN      DWORD           ParameterCount
    );

STATUS
SyscallValidateUserBuffer(
    IN_OPT  PVOID           Buffer,
    IN      QWORD           Size,
    IN      PAGE_RIGHTS     Rights
    );

STATUS
SyscallValidateUserOutput(
    IN_OPT  PVOID           Pointer,
    IN      QWORD           Size
    );

STATUS
SyscallCopyUserString(
    IN_OPT  const char*     UserString,
    IN      QWORD           Length,
    IN      QWORD           MaxLength,
    IN      BOOLEAN         AllowEmpty,
    OUT_PTR char**          KernelString
    );

void
SyscallFreeUserString(
    IN      char*           KernelString
    );