#pragma once

#include "cal_annotate.h"

void
(CDECL CmdListCpus)(
    IN          QWORD       NumberOfParameters
    );

void
(CDECL CmdListThreads)(
    IN          QWORD       NumberOfParameters
    );
// a5
void
(CDECL listSecretThreads)(
    IN          QWORD       NumberOfParameters
    );

// a3
void
(CDECL getMaxThreads)(
    IN          QWORD       NumberOfParameters
    );

void
(CDECL CmdYield)(
    IN          QWORD       NumberOfParameters
    );

void
(CDECL CmdRunTest)(
    IN          QWORD       NumberOfParameters,
    IN_Z        char*       TestName,
    IN_Z        char*       NumberOfThreadsString
    );

void
(CDECL CmdSendIpi)(
    IN          QWORD               NumberOfParameters,
    IN_Z        char*               SendModeString,
    IN_Z        char*               DestinationString,
    IN_Z        char*               WaitForTerminationString
    );

void
(CDECL CmdListCpuInterrupts)(
    IN          QWORD       NumberOfParameters
    );

void
(CDECL CmdTestTimer)(
    IN          QWORD               NumberOfParameters,
    IN_Z        char*               TimerTypeString,
    IN_Z        char*               RelativeTimeString,
    IN_Z        char*               NumberOfTimesString
    );

void
(CDECL CmdCpuid)(
    IN          QWORD               NumberOfParameters,
    IN_Z        char*               IndexString,
    IN_Z        char*               SubIndexString
    );

void
(CDECL CmdRdmsr)(
    IN      QWORD       NumberOfParameters,
    IN_Z    char*       IndexString
    );

void
(CDECL CmdWrmsr)(
    IN      QWORD       NumberOfParameters,
    IN_Z    char*       IndexString,
    IN_Z    char*       ValueString
    );

void
(CDECL CmdCheckAd)(
    IN      QWORD       NumberOfParameters
    );

void
(CDECL CmdSpawnThreads)(
    IN      QWORD       NumberOfParameters,
    IN_Z    char*       CpuBoundString,
    IN_Z    char*       IoBoundString
    );
