#pragma once

#include "core/dll.h"
#include "core/types.h"

namespace Core
{
	class String;

	class CORE_DLL CommandLine
	{
	public:
		CommandLine();
		~CommandLine();
		CommandLine(const CommandLine&) = delete;

		/**
		 * Construct with expanded command line.
		 * i.e.
		 * Game.exe -p parameter1 --long-parameter parameter2 "parameter 3"
		 */
		CommandLine(const char* cmdLine);

		/**
		 * Construct with standard command line.
		 */
		CommandLine(int argc, const char* const argv[]);

		/**
		 * @param s Short form of parameter, i.e. 'p'.
		 * @param l Long form of parameter, i.e. 'param'
		 * @return Do we have an argument?
		 */
		bool HasArg(const char s, const char* l) const;

		/**
		 * @param s Short form of parameter, i.e. 'p'.
		 * @param l Long form of parameter, i.e. 'param'
		 * @param out Output argument.
		 * @return Do we have argument? Will return false if there is no argument after short/long form.
		 */
		bool GetArg(const char s, const char* l, Core::String& out) const;

		/**
		 * @return Command line as a c string.
		 */
		const char* c_str() const;

		/**
		 * @return Argc, as would have came from int main().
		 */
		const i32 GetArgc() const;

		/**
		 * @return Argv, as would have came from int main().
		 */
		const char* const* GetArgv() const;

	private:
		struct CommandLineImpl* impl_ = nullptr;
	};
} // namespace Core
