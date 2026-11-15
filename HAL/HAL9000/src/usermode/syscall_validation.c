#include "HAL9000.h"
#include "syscall_validation.h"
#include "syscall_defs.h"
#include "process_internal.h"
#include "exe_loader.h"
#include "mmu.h"

// First virtual address which is NOT in user space: user space is
// [0, 0x0000'7FFF'FFFF'FFFF]. Everything at or above this is rejected.
#define UM_VA_LIMIT                 0x0000800000000000ULL

BOOLEAN
SyscallGetParameterCount(
    IN      SYSCALL_ID      SyscallId,
    OUT     DWORD*          ParameterCount
    )
{
    ASSERT(NULL != ParameterCount);

    switch (SyscallId)
    {
    case SyscallIdIdentifyVersion:              *ParameterCount = 1; break;

    case SyscallIdThreadExit:                   *ParameterCount = 1; break;
    case SyscallIdThreadCreate:                 *ParameterCount = 3; break;
    case SyscallIdThreadGetTid:                 *ParameterCount = 2; break;
    case SyscallIdThreadWaitForTermination:     *ParameterCount = 2; break;
    case SyscallIdThreadCloseHandle:            *ParameterCount = 1; break;

    case SyscallIdProcessExit:                  *ParameterCount = 1; break;
    case SyscallIdProcessCreate:                *ParameterCount = 5; break;
    case SyscallIdProcessGetPid:                *ParameterCount = 2; break;
    case SyscallIdProcessWaitForTermination:    *ParameterCount = 2; break;
    case SyscallIdProcessCloseHandle:           *ParameterCount = 1; break;

    case SyscallIdVirtualAlloc:                 *ParameterCount = 7; break;
    case SyscallIdVirtualFree:                  *ParameterCount = 3; break;

    case SyscallIdFileCreate:                   *ParameterCount = 5; break;
    case SyscallIdFileClose:                    *ParameterCount = 1; break;
    case SyscallIdFileRead:                     *ParameterCount = 4; break;
    case SyscallIdFileWrite:                    *ParameterCount = 4; break;

    default:
        *ParameterCount = 0;
        return FALSE;
    }

    return TRUE;
}

STATUS
SyscallValidateParameterArea(
    IN      PVOID           Parameters,
    IN      DWORD           ParameterCount
    )
{
    // slot 0 is the syscall id, skip it
    return SyscallValidateUserBuffer(Parameters,
                                     ((QWORD) ParameterCount + 1) * sizeof(QWORD),
                                     PAGE_RIGHTS_READ);
}

STATUS
SyscallValidateUserBuffer(
    IN_OPT  PVOID           Buffer,
    IN      QWORD           Size,
    IN      PAGE_RIGHTS     Rights
    )
{
    PPROCESS pProcess;
    PVOID imageBase;
    DWORD imageSize;
    QWORD start;
    QWORD imageStart;
    QWORD imageEnd;

    // Nothing is accessed for an empty buffer (FileReadZero expects success).
    // The caller must not dereference the pointer in this case.
    if (0 == Size)
    {
        return STATUS_SUCCESS;
    }

    if (NULL == Buffer)
    {
        return STATUS_INVALID_POINTER;
    }

    start = (QWORD) Buffer;

    // The whole range must be in the user half of the address space. The second
    // comparison also rejects start + Size wrapping around.
    if (start >= UM_VA_LIMIT || Size > UM_VA_LIMIT - start)
    {
        return STATUS_MEMORY_PREVENTS_USERMODE_ACCESS;
    }

    pProcess = GetCurrentProcess();
    ASSERT(NULL != pProcess);

    imageBase = NULL;
    imageSize = 0;
    ExecutableLoaderGetVirtualImageBase(pProcess->LoaderContext, &imageBase);
    ExecutableLoaderGetImageSize(pProcess->LoaderContext, &imageSize);

    imageStart = (QWORD) imageBase;
    imageEnd = imageStart + imageSize;

    if (start >= imageStart && start < imageEnd)
    {
        // MmuIsBufferValid() does not check length and rights

        if (Size > imageEnd - start)
        {
            return STATUS_INVALID_BUFFER;
        }

        // The PE headers (first page of the image) are read-only
        if (IsBooleanFlagOn(Rights, PAGE_RIGHTS_WRITE) && start < imageStart + PAGE_SIZE)
        {
            return STATUS_MEMORY_INSUFFICIENT_ACCESS_RIGHTS;
        }

        return STATUS_SUCCESS;
    }

    return MmuIsBufferValid(Buffer, Size, Rights, pProcess);
}

STATUS
SyscallValidateUserOutput(
    IN_OPT  PVOID           Pointer,
    IN      QWORD           Size
    )
{
    return SyscallValidateUserBuffer(Pointer, Size, PAGE_RIGHTS_WRITE);
}

STATUS
SyscallCopyUserString(
    IN_OPT  const char*     UserString,
    IN      QWORD           Length,
    IN      QWORD           MaxLength,
    IN      BOOLEAN         AllowEmpty,
    OUT_PTR char**          KernelString
    )
{
    STATUS status;
    QWORD stringLength;
    char* pCopy;

    ASSERT(NULL != KernelString);

    *KernelString = NULL;

    if (NULL == UserString)
    {
        return STATUS_INVALID_POINTER;
    }

    if (0 == Length || Length > MaxLength)
    {
        return STATUS_INVALID_BUFFER;
    }

    status = SyscallValidateUserBuffer((PVOID) UserString, Length, PAGE_RIGHTS_READ);
    if (!SUCCEEDED(status))
    {
        return status;
    }

    // The terminating '\0' must be inside the buffer we just validated
    for (stringLength = 0; stringLength < Length && '\0' != UserString[stringLength]; ++stringLength)
    {
    }

    if (stringLength == Length)
    {
        return STATUS_INVALID_BUFFER;
    }

    if (!AllowEmpty && 0 == stringLength)
    {
        return STATUS_INVALID_BUFFER;
    }

    pCopy = ExAllocatePoolWithTag(PoolAllocateZeroMemory, stringLength + 1, HEAP_TEMP_TAG, 0);
    if (NULL == pCopy)
    {
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    memcpy(pCopy, UserString, stringLength);
    pCopy[stringLength] = '\0';

    *KernelString = pCopy;

    return STATUS_SUCCESS;
}

void
SyscallFreeUserString(
    IN      char*           KernelString
    )
{
    if (NULL != KernelString)
    {
        ExFreePoolWithTag(KernelString, HEAP_TEMP_TAG);
    }
}