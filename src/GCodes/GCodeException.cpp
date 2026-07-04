/*
 * ParseException.cpp
 *
 *  Created on: 21 Dec 2019
 *      Author: David
 */

#include "GCodeException.h"

#include <General/StringRef.h>
#include <GCodes/GCodeBuffer/GCodeBuffer.h>
#include <Platform/RepRap.h>
#include <Platform/Tasks.h>

GCodeException::GCodeException(const GCodeBuffer *null gb, int col, const char *_ecv_array msg) noexcept : column(col), message(msg)
{
	if (gb != nullptr)
	{
#if HAS_SBC_INTERFACE
		// Don't output the line number in case this exception is thrown from a SBC evaluation request, it's prepended by DSF
		line = (!reprap.UsingSbcInterface() || RTOSIface::GetCurrentTask() == Tasks::GetMainTask()) ? gb->GetLineNumber() : -1;
#else
		line = gb->GetLineNumber();
#endif
		if (gb->IsDoingFileMacro())
		{
			source = GCodeExceptionSource::macro;
		}
		else if (gb->IsDoingFile())
		{
			source = GCodeExceptionSource::file;
		}
		else
		{
			source = GCodeExceptionSource::other;
			line = -1;
		}
	}
	else
	{
		source = GCodeExceptionSource::other;
		line = -1;
	}
}

GCodeException::GCodeException(const GCodeBuffer *null gb, int col, const char *_ecv_array msg, uint32_t uparam) noexcept : GCodeException(gb, col, msg)
{
	param.u = uparam;
}

GCodeException::GCodeException(const GCodeBuffer *null gb, int col, const char *_ecv_array msg, const char *_ecv_array sparam) noexcept : GCodeException(gb, col, msg)
{
	stringParam.copy(sparam);
}

// Construct the error message. This will be prefixed with "Error: " when it is returned to the user.
void GCodeException::GetMessage(const StringRef &reply, const GCodeBuffer *null gb) const noexcept
{
	// Print the file location, if possible. For exceptions that were created without
	// a GCodeBuffer (for example low-level ArrayHandle exceptions), fall back to the
	// buffer passed to GetMessage so that file/macro context is still reported.
	const bool doingFileMacro = source == GCodeExceptionSource::macro
							  || (source == GCodeExceptionSource::other && gb != nullptr && gb->IsDoingFileMacro());
	const bool doingFile = source == GCodeExceptionSource::file
					 || (source == GCodeExceptionSource::other && gb != nullptr && gb->IsDoingFile() && !doingFileMacro);
	const char * const currentFileName = (gb != nullptr) ? gb->GetCurrentFileName() : nullptr;

	if (doingFile)
	{
		if (currentFileName != nullptr)
		{
			reply.printf("in GCode file \"%s\"", currentFileName);
		}
		else
		{
			reply.copy("in GCode file");
		}
	}
	else if (doingFileMacro)
	{
		if (currentFileName != nullptr)
		{
			reply.printf("in file macro \"%s\"", currentFileName);
		}
		else
		{
			reply.copy("in file macro");
		}
	}

	const int effectiveLine = (line >= 0) ? line : ((gb != nullptr && (doingFile || doingFileMacro)) ? gb->GetLineNumber() : -1);
	if (effectiveLine >= 0)
	{
		reply.catf(" line %d", effectiveLine);
		if (column >= 0)
		{
			reply.catf(" column %d", column + 1);
		}
		reply.cat(": ");
	}
	else if (column >= 0)
	{
		reply.catf(" at column %d: ", column + 1);
	}
	else if (!reply.IsEmpty())
	{
		reply.cat(": ");
	}
	else
	{
		reply.Clear();
	}

	// Print the command letter/number, if possible
	if (gb != nullptr)
	{
		switch (gb->GetCommandLetter())
		{
		case 'E':
			reply.cat("meta command: ");
			break;

		case 'G':
		case 'M':
		case 'T':
			if (gb->HasCommandNumber())
			{
				reply.catf("%c%d: ", gb->GetCommandLetter(), gb->GetCommandNumber());
			}
			break;

		case 'Q':		// we use Q0 for comments
		default:
			break;
		}
	}

	// Print the message and any parameter
	if (message == nullptr)
	{
		reply.cat("<null error message>");					// should not happen
	}
	else if (strstr(message, "%s"))
	{
		reply.catf(message, stringParam.c_str());
	}
	else if (strstr(message, "%u") || strstr(message, "%c"))
	{
		reply.catf(message, param.u);
	}
	else
	{
		reply.catf(message, param.i);
	}
}

[[noreturn]] void ThrowGCodeException(const char *errMsg) THROWS(GCodeException)
{
	throw GCodeException(-1, -1, errMsg);
}

[[noreturn]] void ThrowGCodeException(const char *errMsg, uint32_t param) THROWS(GCodeException)
{
	throw GCodeException(-1, -1, errMsg, param);
}

// End
