#pragma once

/**
 * Forward declarations
 */
class UNetConnection;

/**
 * Used to help identify what type of log is being processed
 *
 * NOTE: This enum is not compatible with UENUM (that requires uint8)
 */
enum class ERobotLogType : uint32
{
    None				= 0x00000000,					// Not set

    /** What part of the engine the log message originates from locally (may not be set) */
    OriginUnitTest		= 0x00000001,					// Log originating from unit test code
    OriginEngine		= 0x00000002,					// Log originating from an engine event in the unit test (e.g. unit test Tick)
    OriginNet			= 0x00000004,					// Log originating from netcode for current unit test (specifically net receive)
    OriginConsole		= 0x00000008,					// Log originating from a console command (typically from the unit test window)
    OriginVoid			= 0x00000010,					// Log which should have no origin, and which should be ignored by log capturing

    OriginMask			= OriginUnitTest | OriginEngine | OriginNet | OriginConsole | OriginVoid,

    /** What class of unit test log this is */
    Local				= 0x00000080,					// Log from locally executed code (displayed in 'Local' tab)
    Server				= 0x00000100,					// Log from a server instance (displayed in 'Server' tab)
    Client				= 0x00000200,					// Log from a client instance (displayed in 'Client' tab)

    /** What class of unit test status log this is (UNIT_LOG vs UNIT_STATUS_LOG/STATUS_LOG) */
    GlobalStatus		= 0x00000400,					// Status placed within the overall status window
    UnitStatus			= 0x00000800,					// Status placed within the unit test window

    /** Status log modifiers (used with the UNIT_LOG and UNIT_STATUS_LOG/STATUS_LOG macro's) */
    StatusImportant		= 0x00001000,					// An important status event (displayed in the 'Summary' tab)
    StatusSuccess		= 0x00002000 | StatusImportant,	// Success event status
    StatusWarning		= 0x00004000 | StatusImportant,	// Warning event status
    StatusFailure		= 0x00008000 | StatusImportant,	// Failure event status
    StatusError			= 0x00010000 | StatusFailure,	// Error/Failure event status, that triggers an overall unit test failure
    StatusDebug			= 0x00020000,					// Debug status (displayed in the 'Debug' tab)
    StatusAdvanced		= 0x00040000,					// Status event containing advanced/technical information
    StatusVerbose		= 0x00080000,					// Status event containing verbose information
    StatusAutomation	= 0x00100000,					// Status event which should be printed out to the automation tool

    /** Log output style modifiers */
    StyleBold			= 0x00200000,					// Output text in bold
    StyleItalic			= 0x00400000,					// Output text in italic
    StyleUnderline		= 0x00800000,					// Output pseudo-underline text (add newline and --- chars; UE4 can't underline)
    StyleMonospace		= 0x01000000,					// Output monospaced text (e.g. for list tab formatting); can't use bold/italic

    All					= 0xFFFFFFFF,

    /** Log lines that should request focus when logged (i.e. if not displayed in the current tab, switch to a tab that does display) */
    FocusMask			= OriginConsole,
};

ENUM_CLASS_FLAGS(ERobotLogType);

// IMPORTANT: If you add more engine-log-capture globals, you must add them to the 'UNIT_EVENT_CLEAR' and related macros

/** Used by to aid in hooking log messages triggered by unit tests */
//extern NETCODEROBOTTEST_API URobotTestBase* GActiveLogRobotTest;

//extern UWorld* GActiveRobotLogWorld;				// @todo JohnB: Does this make other log hooks redundant?

/** The current ERobotLogType flag modifiers, associated with a UNIT_LOG or UNIT_STATUS_LOG/STATUS_LOG call */
extern NETCODEROBOTTEST_API ERobotLogType GActiveLogTypeFlags;

/**
 * Declarations
 */

NETCODEROBOTTEST_API DECLARE_LOG_CATEGORY_EXTERN(LogRobotTest, Log, All);

/**
 * Helper function, for macro defines - allows the 'UnitLogTypeFlags' macro parameters below, to be optional
 */
inline ERobotLogType OptionalFlags(ERobotLogType InFlags= ERobotLogType::None)
{
    return InFlags;
}
