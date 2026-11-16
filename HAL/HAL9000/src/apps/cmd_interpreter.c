#include "HAL9000.h"
#include "cmd_interpreter.h"

#include "keyboard_utils.h"
#include "display.h"
#include "print.h"

#include "cmd_common.h"
#include "cmd_fs_helper.h"
#include "cmd_thread_helper.h"
#include "cmd_proc_helper.h"
#include "cmd_sys_helper.h"
#include "cmd_net_helper.h"
#include "cmd_basic.h"
#include "boot_module.h"
#include "cal_annotate.h"
#include "cal_warning.h"

WARNING_PUSH

// warning C4029: declared formal parameter list different from definition
MSVC_WARNING_DISABLE(4029)

#define CMD_MAX_ARGS        10

#define CMD_EXIT            "exit"

static
void
(CDECL _CmdPrintHelp)(
    const QWORD NumberOfParameters
    );

// warning C4212: nonstandard extension used: function declaration used ellipsis
WARNING_PUSH
MSVC_WARNING_DISABLE(4212)

typedef struct _COMMAND_DEFINITION
{
    char*                   CommandName;
    char*                   Description;

    PFUNC_GenericCommand    CommandFunction;

    QWORD                   MinParameters;
    QWORD                   MaxParameters;
} COMMAND_DEFINITION, *PCOMMAND_DEFINITION;

static const COMMAND_DEFINITION COMMANDS[] =
{
    { "reset", "Restarts the system", (PFUNC_GenericCommand) CmdResetSystem, 0, 0},
    { "shutdown", "Shuts the system down", (PFUNC_GenericCommand) CmdShutdownSystem, 0, 0},

    { "log", "[ON|OFF] - enables or disables logging", (PFUNC_GenericCommand) CmdLogSetState, 1, 1},
    { "loglevel", "$LOG_LEVEL - decimal value from enum", (PFUNC_GenericCommand) CmdSetLogLevel, 1, 1},
    { "logcomp", "$LOG_COMPONENT - hexadecimal value from enum", (PFUNC_GenericCommand) CmdSetLogComponents, 1, 1},

    { "cls", "Clears screen", (PFUNC_GenericCommand) CmdClearScreen, 0, 0},

    { "vol", "Displays volumes", (PFUNC_GenericCommand) CmdPrintVolumeInformation, 0, 0},
    { "less", "$FILENAME [async]\n\tdisplay $FILENAME contents\n\tasync - use DMA read instead of polling", (PFUNC_GenericCommand) CmdReadFile, 1, 2 },
    { "fwrite", "$FILENAME [char] [ext] [async]\n\twrite predefined buffer into $FILENAME\n\text - if 'ext' then extend file size\n\tasync - use DMA read instead of polling", (PFUNC_GenericCommand) CmdWriteFile, 1, 4},
    { "stat", "$FILENAME\n\tdisplays $FILENAME information", (PFUNC_GenericCommand) CmdStatFile, 1, 1},
    { "mkdir", "$DIRECTORY\n\tcreates a new directory", (PFUNC_GenericCommand) CmdMakeDirectory, 1, 1},
    { "touch", "$FILENAME\n\tcreates a new file", (PFUNC_GenericCommand) CmdMakeFile, 1, 1},
    { "ls", "$DIRECTORY [-R]\n\tlists directory contents\n\tif -R specified goes recursively", (PFUNC_GenericCommand) CmdListDirectory, 1, 2},

    { "swap", "R|W [0x$OFFSET]\n\t$OFFSET - offset inside swap where to perform operation", (PFUNC_GenericCommand) CmdSwap, 1, 2},

    { "cpu", "Displays CPU related information", (PFUNC_GenericCommand) CmdListCpus, 0, 0},
    { "int", "List interrupts received", (PFUNC_GenericCommand) CmdListCpuInterrupts, 0, 0},
    { "yield", "Yields processor", (PFUNC_GenericCommand) CmdYield, 0, 0},
    { "timer", "$MODE [$TIME_IN_US] [$TIMES]\n\tSee EX_TIMER_TYPE for timer types\n\t$TIME_IN_US time in uS until timer fires"
                "\n\t$TIMES - number of times to wait for timer, valid only if periodic", (PFUNC_GenericCommand) CmdTestTimer, 1, 3},

    // a3
    { "maxThreads", "Display thread with greatest TID", (PFUNC_GenericCommand) getMaxThreads, 0, 0},
    { "threads", "Displays all threads", (PFUNC_GenericCommand) CmdListThreads, 0, 0},
    { "run", "$TEST [$NO_OF_THREADS]\n\tRuns the $TEST specified"
             "\n\t$NO_OF_THREADS the number of threads for running the test,"
             "if the number is not specified then it will run on 2 * NumberOfProcessors",
             (PFUNC_GenericCommand) CmdRunTest, 1, 2},

    // a5
    { "secretThreads", "List secret threads", (PFUNC_GenericCommand) listSecretThreads, 0, 0},

    { "processes", "Displays all processes", (PFUNC_GenericCommand) CmdListProcesses, 0, 0},
    { "procstat", "0x$PID - displays information about a process", (PFUNC_GenericCommand) CmdProcessDump, 1, 1},
    { "procstart", "$PATH_TO_EXE - starts a process", (PFUNC_GenericCommand) CmdStartProcess, 1, 1},
    { "proctest", "$TEST_NAME - runs a process test", (PFUNC_GenericCommand) CmdTestProcess, 1, 1},

    { "sysinfo", "Retrieves system information", (PFUNC_GenericCommand) CmdDisplaySysInfo, 0, 0},
    { "getidle", "Retrieves idle timeout", (PFUNC_GenericCommand) CmdGetIdle, 0, 0},
    { "setidle", "$PERIOD_IN_SECONDS - Sets idle timeout", (PFUNC_GenericCommand) CmdSetIdle, 1, 1},

    { "rdmsr", "0x$INDEX\n\t$INDEX is the MSR to read", (PFUNC_GenericCommand) CmdRdmsr, 1, 1},
    { "wrmsr", "0x$INDEX 0x$VALUE\n\t$INDEX is the MSR to write\n\t$VALUE is the value to place in the MSR", (PFUNC_GenericCommand) CmdWrmsr, 2, 2},
    { "chkad", "Check if paging accessed/dirty bits mechanism is working", (PFUNC_GenericCommand) CmdCheckAd, 0, 0},
    { "spawn", "$CPU_BOUND $IO_BOUND\n\tNumber of CPU bound threads to spawn\n\tNumber of IO bound threads to spawn", (PFUNC_GenericCommand) CmdSpawnThreads, 2, 2},
    { "cpuid", "[0x$INDEX] [0x$SUBINDEX]\n\tIf index is not specified lists all available CPUID values"
                "\n\tIf subindex is specified displays subleaf information", (PFUNC_GenericCommand) CmdCpuid, 0, 2},
    { "ipi", "$MODE [$DEST] {$WAIT]\n\tSee SMP_IPI_SEND_MODE for destination mode\n\t$DEST - processor IDs"
              "\n\tIf last parameter is specified will wait until all CPUs acknowledge IPI", (PFUNC_GenericCommand) CmdSendIpi, 1, 3},

    { "networks", "Displays network information", (PFUNC_GenericCommand) CmdListNetworks, 0, 0},
    { "netrecv", "[YES|NO] - receive network packets\n\tIf yes will resend the packets received, if no it will not", (PFUNC_GenericCommand) CmdNetRecv, 0, 1},
    { "netsend", "Send network packets", (PFUNC_GenericCommand) CmdNetSend, 0, 0},
    { "netstatus", "$DEV_ID $RX_EN $TX_EN - changes the state of a network device"
                   "\n\tDevice ID\n\tIf $RX_EN is 1 => will enable receive on device\n\tIf $TX_EN is 1 => will enable send on device",
                    (PFUNC_GenericCommand) CmdChangeDevStatus, 3, 3},

    { "tests", "Runs functional tests", (PFUNC_GenericCommand) CmdRunAllFunctionalTests, 0, 0},
    { "perf", "Runs performance tests", (PFUNC_GenericCommand) CmdRunAllPerformanceTests, 0, 0},

    { "recursion", "Generates an infinite recursion", (PFUNC_GenericCommand) CmdInfiniteRecursion, 0, 0},
    { "rtcfail", "Causes an RTC check stack to assert", (PFUNC_GenericCommand) CmdRtcFail, 0, 0},
    { "rangefail", "Causes a range check failure to assert", (PFUNC_GenericCommand) CmdRangeFail, 0, 0},
    { "bitecookie", "Causes a GS cookie corruption to assert", (PFUNC_GenericCommand) CmdBiteCookie, 0, 0},

    { "help", "Displays this help menu", (PFUNC_GenericCommand) _CmdPrintHelp, 0, 0}
};

#define NO_OF_COMMANDS      ARRAYSIZE(COMMANDS)


static
BOOLEAN
_CmdExecLine(
    _Inout_updates_z_(Length)
                    char*   CommandLine,
    IN              DWORD   Length
    );

static
BOOLEAN
_CmdExecuteModuleCommands(
    void
    );

// SAL simply doesn't want to let me tell him that each pointer in argv is NULL terminated
// _At_buffer_(argv, i, argc,
//             _Pre_satisfies_(argv[i] _Null_terminated_))
BOOLEAN
ExecCmd(
    IN      DWORD       argc,
    IN_READS(CMD_MAX_ARGS)
            char**      argv
    )
{
    char* pCommand;
    BOOLEAN bFoundCommand;
    DWORD noOfParameters;

    ASSERT(1 <= argc && argc <= CMD_MAX_ARGS);
    ASSERT(NULL != argv);

    pCommand = (char*) argv[0];
    bFoundCommand = FALSE;
    noOfParameters = argc - 1;

    ASSERT(NULL != pCommand);

    // check for exit command
    if (0 == stricmp(pCommand, CMD_EXIT))
    {
        return TRUE;
    }

    for (DWORD i = 0; i < NO_OF_COMMANDS; ++i)
    {
        if (stricmp(pCommand, COMMANDS[i].CommandName) == 0)
        {
            bFoundCommand = TRUE;

            if (COMMANDS[i].MinParameters <= noOfParameters && noOfParameters <= COMMANDS[i].MaxParameters)
            {
                COMMANDS[i].CommandFunction(noOfParameters,
                                            argv[1],
                                            argv[2],
                                            argv[3],
                                            argv[4],
                                            argv[5],
                                            argv[6],
                                            argv[7],
                                            argv[8],
                                            argv[CMD_MAX_ARGS - 1]);

                break;
            }
            else
            {
                LOG_ERROR("Tried to call command [%s] which requires between %u and %u parameters with %u parameters!\n",
                          COMMANDS[i].CommandName, COMMANDS[i].MinParameters, COMMANDS[i].MaxParameters, noOfParameters);
                LOG("%s\n", COMMANDS[i].Description);
            }
        }
    }

    if (!bFoundCommand)
    {
        _CmdPrintHelp(0);
    }

    return FALSE;
}

void
CmdRun(
    void
    )
{
    BOOLEAN exit;
    char buffer[CHARS_PER_LINE];
    DWORD bytesRead;

    bytesRead = 0;

    exit = _CmdExecuteModuleCommands();
    while (!exit)
    {
        gets_s(buffer, CHARS_PER_LINE, &bytesRead);
        exit = _CmdExecLine(buffer, bytesRead);
    }

    return;
}

static
BOOLEAN
_CmdExecLine(
    _Inout_updates_z_(Length)
                    char*   CommandLine,
    IN              DWORD   Length
    )
{
    BOOLEAN bExit;
    char* pCmdArgs[CMD_MAX_ARGS];

    bExit = FALSE;

    if (CommandLine[0] == '/')
    {
        char* context;
        char* pCurArg;
        DWORD argIndex;

        memzero(pCmdArgs, sizeof(char*) * CMD_MAX_ARGS);
        context = NULL;
        pCurArg = NULL;
        argIndex = 0;

        if (Length <= 1)
        {
            pwarn("A command must succeed the '/' character\n");
            return FALSE;
        }

        // warning C4127: conditional expression is constant
MSVC_WARNING_SUPPRESS(4127)
        while (TRUE)
        {
            pCurArg = (char*)strtok_s(&CommandLine[1], " ", &context);
            if (pCurArg == NULL)
            {
                break;
            }

            ASSERT(argIndex < CMD_MAX_ARGS);

            pCmdArgs[argIndex] = pCurArg;
            argIndex = argIndex + 1;
        }

        // we have a command
GNU_WARNING_IGNORE("-Wincompatible-pointer-types")
        bExit = ExecCmd(argIndex, pCmdArgs);
GNU_WARNING_ERROR("-Wincompatible-pointer-types")
    }
    else
    {
        // just plain old text
        printf("%s\n", CommandLine);
    }

    return bExit;
}

static
void
(CDECL _CmdPrintHelp)(
    IN      QWORD           NumberOfParameters
    )
{
    ASSERT(NumberOfParameters == 0);

    LOG("All commands are prefixed with the '/' character and are case insensitive\n");
    LOG("Available commands:\n");
    LOG("exit - stops the OS\n");
    for (DWORD i = 0; i < NO_OF_COMMANDS; ++i)
    {
        LOG("%s - %s\n", COMMANDS[i].CommandName, COMMANDS[i].Description);
    }
}

static
BOOLEAN
_CmdExecuteModuleCommands(
    void
    )
{
    STATUS status;
    char* pBaseAddress;
    QWORD modLen;
    char* pCurrentLine;
    char* context;
    BOOLEAN bExit;

    context = NULL;
    bExit = FALSE;

GNU_WARNING_IGNORE("-Wincompatible-pointer-types")
    status = BootModuleGet("Tests", &pBaseAddress, &modLen);
GNU_WARNING_ERROR("-Wincompatible-pointer-types")
    if (!SUCCEEDED(status))
    {
        LOG_WARNING("BootModuleGet failed with status 0x%x for Tests module\n", status);
        return FALSE;
    }

    do
    {
        pCurrentLine = (char*)strtok_s(pBaseAddress, "\n\r", &context);

        if (pCurrentLine != NULL && strlen(pCurrentLine) != 0)
        {
            LOG("Current line is [%s] of length 0x%x\n", pCurrentLine,
                strlen(pCurrentLine));
            bExit = _CmdExecLine(pCurrentLine, strlen(pCurrentLine));
        }

    } while (pCurrentLine != NULL && !bExit);

    return bExit;
}

WARNING_POP
