#pragma once
#ifndef ARGPARSE_H_INCLUDED
#define ARGPARSE_H_INCLUDED

/*
ArgParse - A header-only command line argument parser for C++

Features:
	* Single file header-only C++11 library (450 lines)
	* Short form and long form of arguments
	* Default values for optional arguments
	* Can nest commands one level deep
	* Automatically generated help text
	* Tested on clang, gcc, MSVC
	* No exceptions

Options have a long name, and an optional single-character short name:

	--force
	-f

Options can be boolean, like above, or they can have a value associated with them:

	--outfile filename
	-o filename

Anything that is not part of an option is collected in 'Params'

Commands can be added with `AddCommand`. If one or more commands have been defined,
then the user _must_ enter a valid command as the first parameter. Subsequent parameters
are fed to the chosen command. Commands can have options and switches, just like the
master `Args` object.

To print out full help for all options, use `args.ShowHelp()`. This is invoked
automatically when the user types `help`.

Simple example:
	argparse::Args args("Usage: myprogram [options...] param1");
	args.AddSwitch("f", "force", "Force a certain thing");
	args.AddValue("o", "outfile", "Write output to file");
	args.AddValue("t", "timeout", "Timeout in seconds", "60");
	if (!args.Parse(argc, argv))
		return 1;

	if (args.Has("force")) { ... }
	int timeout = args.GetInt("timeout");

Example using commands:

int fooFunc(argparse::Args& args) {
	return 0;
}

int main(int argc, char** argv) {
	argparse::Args args("myprogram [options...] <command>");
	auto cmdFoo = args.AddCommand("foo <src> <dst>", "Copy from src to dst", fooFunc);
	auto cmdBar = args.AddCommand("bar", "Do something else", barFunc);
	cmdFoo->AddSwitch("v", "verbose", "Max verbosity");
	if (!args.Parse(argc, (const char**) argv))
		return 1;

	return args.ExecCommand();
}

Copyright: IMQS Software
License: MIT
*/

#include <vector>
#include <string>
#include <string.h>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <stdint.h>

namespace argparse {

class Args;

typedef std::function<int(Args& args)> CmdFunc;

class Option {
public:
	bool        ExpectsValue = false; // True if flag has an associated value
	std::string Short;
	std::string Long;
	std::string Summary;
	std::string Default;

	bool        Toggled = false; // Set to true if the value is present. Works for Switch and Value options.
	std::string Value;           // Value of option, if ExpectsValue=true

	bool HasShort() const { return Short.length() != 0; }

	bool operator<(const Option& b) const { return Long < b.Long; }
};

class Args {
public:
	std::string              Usage; // Main usage text. Everything before the first \n is the "short" usage text.
	std::vector<Option>      Options;
	std::vector<std::string> Params;
	std::vector<Args*>       Commands;
	bool                     WasHelpShown = false; // True if Parse() returns false, and showed help text

	// Command parameters
	std::string       CmdName;                  // Name of a command
	std::string       CmdParams;                // Help text describing parameters of command
	argparse::CmdFunc CmdFunc;                  // Function to execute for command
	bool              CmdEnforceParams = true;  // If CmdParams text is "<param1> <param2>", then make sure two parameters are passed to command
	bool              CmdWasChosen     = false; // True if this command was chosen

	Args(std::string usage) : Usage(usage) {}                             // Set main usage text for a command
	Args(std::string cmdName, std::string usage, argparse::CmdFunc func); // Set title and description for a command
	~Args();

	// Setup
	void  AddSwitch(std::string _short, std::string _long, std::string summary);                               // Add a binary on/off option that has no value (eg --nocache)
	void  AddValue(std::string _short, std::string _long, std::string summary, std::string defaultValue = ""); // Add an option that has an associated value (eg -f outfile)
	Args* AddCommand(std::string name, std::string description, argparse::CmdFunc func = nullptr);             // Add a command

	// Help
	void ShowHelp();

	// Parse
	// startAt: Start parsing at this argument. You normally skip the first argument, because it's typically the name of the program
	// Returns true if parse succeeded, or false if parse failed, or if help was shown. See WasHelpShown to know if the parse failed
	// because the user requested help.
	bool Parse(int argc, const char** argv, int startAt = 1);

	// Results
	int         ExecCommand();                                     // Execute the command that was chosen. Returns value from exec function.
	Args*       WhichCommand();                                    // Returns the command that was chosen, or null.
	bool        Has(const std::string& _short_or_long) const;      // Returns true if the option was specified
	std::string Get(const std::string& _short_or_long) const;      // Get an option's value. Returns default value if not specified.
	int         GetInt(const std::string& _short_or_long) const;   // Get an option and convert to int. Returns default value if not specified.
	int64_t     GetInt64(const std::string& _short_or_long) const; // Get an option and convert to int64. Returns default value if not specified.

private:
	Option*     FindOption(const char* arg);
	bool        ValidateSanity(int depth) const;
	void        Reset();
	void        ShowHelpInternal(int depth, std::string forCmd);
	std::string UsageShort() const;     // Returns everything before the first \n from Usage
	std::string UsageDetails() const;   // Returns everything after the first \n from Usage
	size_t      CmdParamsCount() const; // If CmdParams is "<param1> <param2>" then return 2 (ie the number of objects inside <angled brackets>)
	static void WriteFormattedText(int indent, std::string text, int lineLength);
	static bool IsNumeric(const char* s);
};

inline Args::Args(std::string cmdName, std::string usage, argparse::CmdFunc func) : Usage(usage), CmdFunc(func) {
	// Allow cmdName to be specified as 'command <param1> <param2>'
	auto space = cmdName.find(' ');
	if (space != -1) {
		CmdName   = cmdName.substr(0, space);
		CmdParams = cmdName.substr(space + 1);
	} else {
		CmdName = cmdName;
	}
}

inline Args::~Args() {
	for (auto a : Commands)
		delete a;
}

inline void Args::AddSwitch(std::string _short, std::string _long, std::string summary) {
	Option opt;
	opt.ExpectsValue = false;
	opt.Short        = _short;
	opt.Long         = _long;
	opt.Summary      = summary;
	opt.Default      = "0";
	Options.push_back(opt);
}

inline void Args::AddValue(std::string _short, std::string _long, std::string summary, std::string defaultValue) {
	Option opt;
	opt.ExpectsValue = true;
	opt.Short        = _short;
	opt.Long         = _long;
	opt.Summary      = summary;
	opt.Default      = defaultValue;
	Options.push_back(opt);
}

inline Args* Args::AddCommand(std::string name, std::string description, argparse::CmdFunc func) {
	Commands.push_back(new Args(name, description, func));
	return Commands.back();
}

inline void Args::ShowHelp() {
	ShowHelpInternal(0, "");
}

inline std::string Args::UsageShort() const {
	return Usage.substr(0, Usage.find('\n'));
}

inline std::string Args::UsageDetails() const {
	auto pos = Usage.find('\n');
	if (pos != -1 && pos < Usage.length() - 1)
		return Usage.substr(pos + 1);
	else
		return "";
}

inline size_t Args::CmdParamsCount() const {
	size_t count = 0;
	for (size_t i = 0; i < CmdParams.size(); i++) {
		if (CmdParams[i] == '<')
			count++;
	}
	return count;
}

inline bool Args::Parse(int argc, const char** argv, int startAt) {
	if (!ValidateSanity(0))
		return false;
	Reset();
	Args* cmd = nullptr;
	for (int i = startAt; i < argc; i++) {
		bool        atEnd = i == argc - 1;
		std::string arg   = argv[i];
		if (arg.length() != 0 && arg[0] == '-') {
			// option
			auto opt = cmd ? cmd->FindOption(arg.c_str()) : FindOption(arg.c_str());
			if (opt) {
				if (opt->ExpectsValue && atEnd) {
					printf("Option %s expects a value, eg --%s <something>\n", arg.c_str(), opt->Long.c_str());
					return false;
				}
				if (opt->ExpectsValue) {
					i++;
					opt->Value   = argv[i];
					opt->Toggled = true;
				} else {
					opt->Toggled = true;
				}
				continue;
			} else {
				auto a = arg;
				if (a == "-h" || a == "-help" || a == "--help" || a == "-?" || a == "/?" || a == "/h" || a == "/help") {
					if (atEnd)
						ShowHelp();
					else
						ShowHelpInternal(0, argv[i + 1]);
					return false;
				}
				if (IsNumeric(arg.c_str())) {
					// If this is a negative number, then fall through to positional parameter
				} else {
					printf("Unknown option '%s'\n", arg.c_str());
					return false;
				}
			}
		}
		if (Commands.size() != 0 && !cmd) {
			// command
			for (Args* c : Commands) {
				if (c->CmdName == arg) {
					cmd               = c;
					cmd->CmdWasChosen = true;
					break;
				}
			}
			if (!cmd) {
				if (arg == "help" && !atEnd)
					ShowHelpInternal(0, argv[i + 1]);
				else if (arg == "help")
					ShowHelpInternal(0, "");
				else
					printf("Unknown command '%s'\n", arg.c_str());
				return false;
			}
			continue;
		}

		// positional parameter
		if (cmd)
			cmd->Params.push_back(arg);
		else
			Params.push_back(arg);
	}

	if (cmd && cmd->CmdEnforceParams) {
		auto nparams = cmd->Params.size();
		if (nparams != cmd->CmdParamsCount()) {
			printf("%s expects %d parameters: %s, but there are %d parameters\n", cmd->CmdName.c_str(), (int) cmd->CmdParamsCount(), cmd->CmdParams.c_str(), (int) nparams);
			return false;
		}
	}

	return true;
}

inline int Args::ExecCommand() {
	auto cmd = WhichCommand();
	if (!cmd)
		return 1;
	return cmd->CmdFunc(*cmd);
}

inline Args* Args::WhichCommand() {
	for (Args* c : Commands) {
		if (c->CmdWasChosen)
			return c;
	}
	return nullptr;
}

inline bool Args::Has(const std::string& _short_or_long) const {
	for (const auto& opt : Options) {
		if ((opt.HasShort() && opt.Short == _short_or_long) || opt.Long == _short_or_long)
			return opt.Toggled;
	}
	printf("Option %s does not exist\n", _short_or_long.c_str());
	return false;
}

inline std::string Args::Get(const std::string& _short_or_long) const {
	for (auto& opt : Options) {
		if ((opt.HasShort() && opt.Short == _short_or_long) || opt.Long == _short_or_long) {
			if (!opt.ExpectsValue) {
				printf("Cannot use Get() on a Switch parameter. Use Has() instead.\n");
				return opt.Toggled ? "1" : "0";
			}
			if (opt.Toggled)
				return opt.Value;
			else
				return opt.Default;
		}
	}
	return "";
}

inline int Args::GetInt(const std::string& _short_or_long) const {
	return atoi(Get(_short_or_long).c_str());
}

inline int64_t Args::GetInt64(const std::string& _short_or_long) const {
#ifdef _MSC_VER
	return (int64_t) _atoi64(Get(_short_or_long).c_str());
#else
	return (int64_t) atoll(Get(_short_or_long).c_str());
#endif
}

inline Option* Args::FindOption(const char* arg) {
	for (auto& opt : Options) {
		if (opt.HasShort() && strcmp(opt.Short.c_str(), arg + 1) == 0)
			return &opt;
		if (arg[1] == '-' && strcmp(opt.Long.c_str(), arg + 2) == 0)
			return &opt;
	}
	return nullptr;
}

inline bool Args::ValidateSanity(int depth) const {
	std::unordered_set<std::string> seen;
	for (const auto& opt : Options) {
		if (opt.HasShort() && opt.Short.size() != 1) {
			printf("Short options must be one character exactly (not %s)\n", opt.Short.c_str());
			return false;
		}
		if (opt.HasShort() && seen.find(opt.Short) != seen.end()) {
			printf("Option %s appears twice\n", opt.Short.c_str());
			return false;
		}
		if (seen.find(opt.Long) != seen.end()) {
			printf("Option %s appears twice\n", opt.Long.c_str());
			return false;
		}
		if (opt.HasShort())
			seen.insert(opt.Short);
		seen.insert(opt.Long);
	}
	if (depth == 1 && Commands.size() != 0) {
		printf("Commands cannot be nested. Command '%s' has commands beneath it.\n", CmdName.c_str());
		return false;
	}
	if (Commands.size() != 0 && Params.size() != 0) {
		printf("You cannot mix commands and parameters on the top-level object. Commands go on top, and parameters on the children.");
		return false;
	}
	for (auto c : Commands) {
		if (!c->ValidateSanity(depth + 1))
			return false;
	}
	return true;
}

inline void Args::Reset() {
	for (auto& opt : Options) {
		opt.Toggled = false;
		opt.Value   = "";
	}
	for (auto c : Commands) {
		c->CmdWasChosen = false;
		c->Reset();
	}
}

inline void Args::WriteFormattedText(int indent, std::string text, int lineLength) {
	std::string line;
	for (size_t i = 0; i < text.size(); i++) {
		bool tooLong            = line.length() > lineLength && line.back() == ' ';
		bool hasExplicitNewline = line.length() > 0 && line.back() == '\n';
		if (tooLong || hasExplicitNewline) {
			printf("%*s%s", indent, " ", line.c_str());
			if (!hasExplicitNewline)
				printf("\n");
			line = "";
		}
		line += text[i];
	}
	if (line != "")
		printf("%*s%s\n", indent, " ", line.c_str());
}

inline bool Args::IsNumeric(const char* s) {
	for (; *s; s++) {
		if (!((*s >= '0' && *s <= '9') || *s == '-' || *s == '+' || *s == '.' || *s == 'e'))
			return false;
	}
	return true;
}

inline void Args::ShowHelpInternal(int depth, std::string forCmd) {
	const int maxLineLength = 80;
	if (forCmd != "") {
		for (auto c : Commands) {
			if (c->CmdName == forCmd) {
				c->ShowHelpInternal(1, "");
				return;
			}
		}
		printf("Unknown command '%s'\n", forCmd.c_str());
		return;
	}

	int maxLong = 0;
	for (const auto& opt : Options)
		maxLong = (int) opt.Long.length() > maxLong ? (int) opt.Long.length() : maxLong;

	if (depth == 1) {
		if (CmdParams != "")
			printf("%s %s\n\n %s\n", CmdName.c_str(), CmdParams.c_str(), UsageShort().c_str());
		else
			printf("%s %s\n\n", CmdName.c_str(), UsageShort().c_str());
		auto details = UsageDetails();
		if (details != "") {
			printf("\n");
			WriteFormattedText(1, details, maxLineLength);
		}
	} else {
		printf("%s\n", UsageShort().c_str());
		auto details = UsageDetails();
		if (details != "") {
			printf("\n");
			WriteFormattedText(1, details, maxLineLength);
		}

		if (Commands.size() != 0) {
			int maxCmd = 0;
			for (auto c : Commands) {
				maxCmd = (int) c->CmdName.length() > maxCmd ? (int) c->CmdName.length() : maxCmd;
			}

			printf("\n");
			for (auto c : Commands) {
				printf(" %-*s %s\n", maxCmd, c->CmdName.c_str(), c->UsageShort().c_str());
			}
		}
	}
	printf("\n");

	auto copy = Options;
	std::sort(copy.begin(), copy.end());
	for (const auto& opt : copy) {
		if (opt.HasShort())
			printf(" -%s --%-*s %s", opt.Short.c_str(), maxLong, opt.Long.c_str(), opt.Summary.c_str());
		else
			printf("    --%-*s %s", maxLong, opt.Long.c_str(), opt.Summary.c_str());
		if (opt.ExpectsValue && opt.Default != "") {
			printf(" (%s)", opt.Default.c_str());
		}
		printf("\n");
	}
	WasHelpShown = true;
}

} // namespace argparse

#endif
