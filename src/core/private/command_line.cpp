#include "core/command_line.h"
#include "core/string.h"
#include "core/vector.h"

namespace Core
{
	struct CommandLineImpl
	{
		Core::String fullCommandline_;
		Core::Vector<Core::String> args_;
		Core::Vector<const char*> argsChar_;
	};
	CommandLine::CommandLine() { impl_ = new CommandLineImpl(); }

	CommandLine::~CommandLine() { delete impl_; }

	CommandLine::CommandLine(const char* cmdLine)
	    : CommandLine()
	{
		impl_->fullCommandline_ = cmdLine;
		bool inQuotes = false;
		bool skippedFirst = false;
		Core::String arg;
		while(char ch = *cmdLine)
		{
			char str[] = {ch, '\0'};
			bool flushArg = false;
			if(ch == '\"')
			{
				inQuotes = !inQuotes;
				if(inQuotes == false)
					flushArg = true;
			}
			else if(inQuotes)
			{
				arg.Append(str);
			}
			else
			{
				if(ch == ' ')
					flushArg = true;
				else
					arg.Append(str);
			}

			if(flushArg)
			{
				if(arg.size() > 0)
				{
					if(skippedFirst)
					{
						impl_->args_.push_back(std::move(arg));
					}
					else
					{
						arg.clear();
						skippedFirst = true;
					}
				}
			}

			++cmdLine;
		}

		if(arg.size() > 0 && skippedFirst)
			impl_->args_.push_back(std::move(arg));

		// Setup CArgs.
		impl_->argsChar_.reserve(impl_->args_.size());
		for(const auto& CArg : impl_->args_)
			impl_->argsChar_.push_back(CArg.c_str());
	}

	CommandLine::CommandLine(int argc, const char* const argv[])
	    : CommandLine()
	{
		impl_->args_.reserve(argc - 1);
		for(int idx = 1; idx < argc; ++idx)
		{
			impl_->args_.push_back(argv[idx]);
			if(idx > 1)
			{
				impl_->fullCommandline_ += " ";
			}
			impl_->fullCommandline_ += argv[idx];
		}

		// Setup CArgs.
		impl_->argsChar_.reserve(impl_->args_.size());
		for(const auto& arg : impl_->args_)
		{
			impl_->argsChar_.push_back(arg.c_str());
		}
	}

	bool CommandLine::HasArg(const char s, const char* l) const
	{
		for(const auto& arg : impl_->args_)
		{
			if(arg.size() == 2 && s != '\0')
			{
				if(arg[0] == '-' && arg[1] == s)
				{
					return true;
				}
			}
			else if(arg.size() > 2 && l != nullptr)
			{
				if(arg[0] == '-' && arg[1] == '-')
				{
					if(arg.substr(2, Core::String::npos) == l)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool CommandLine::GetArg(const char s, const char* l, Core::String& out) const
	{
		if(impl_->args_.size() < 2)
			return false;

		for(i32 idx = 0; idx < impl_->args_.size() - 1; ++idx)
		{
			const auto& arg0(impl_->args_[idx]);
			const auto& arg1(impl_->args_[idx + 1]);

			if(arg0.size() == 2)
			{
				if(arg0[0] == '-' && arg0[1] == s)
				{
					out = arg1;
					return true;
				}
			}
			else if(arg0.size() > 2)
			{
				if(arg0[0] == '-' && arg0[1] == '-')
				{
					if(arg0.substr(2, Core::String::npos) == l)
					{
						out = arg1;
						return true;
					}
				}
			}
		}

		return false;
	}

	const char* CommandLine::c_str() const { return impl_->fullCommandline_.c_str(); }

	const i32 CommandLine::GetArgc() const { return impl_->argsChar_.size(); }

	const char* const* CommandLine::GetArgv() const { return impl_->argsChar_.data(); }

} // namespace Core
