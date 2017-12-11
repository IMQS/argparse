#pragma once
#ifndef ARGPARSE_H_INCLUDED
#define ARGPARSE_H_INCLUDED

/*
ArgParse - A header-only command line argument parser for C++

Features:
	* Single file header-only C++11 library (250 lines)
	* Short form and long form of arguments
	* Default values for optional arguments
	* Automatically generated help text
	* Tested on clang, gcc, MSVC
	* No exceptions

Options have a long name, and an optional single-character short name

	-f
	--force

Options can be boolean, or they can have a value associated with them

	-o filename
	--outfile filename

Anything that is not part of an option is collected in 'Params'

To print out full help for all options, use args.ShowHelp().

Example:
	argparse::Args args("Usage: myprogram [options...] param1");
	args.AddSwitch("f", "force", "Force a certain thing");
	args.AddValue("o", "outfile", "Write output to file");
	args.AddValue("t", "timeout", "Timeout in seconds", "60");
	if (!args.Parse(argc, argv))
		return 1;

	if (args.Has("force")) { ... }
	int timeout = args.GetInt("timeout");

Copyright: IMQS Software
License: MIT
*/

#include <vector>
#include <string>
#include <string.h>
#include <unordered_set>
#include <algorithm>
#include <stdint.h>

namespace argparse {

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
	std::string              Usage; // Main usage text
	std::vector<Option>      Options;
	std::vector<std::string> Params;

	// Set main usage text
	Args(std::string usage) : Usage(usage) {}

	// Setup
	void AddSwitch(std::string _short, std::string _long, std::string summary);                               // Add a binary on/off option that has no value (eg -nocache)
	void AddValue(std::string _short, std::string _long, std::string summary, std::string defaultValue = ""); // Add an option that has an associated value (eg -f outfile)

	// Help
	void ShowHelp();

	// Parse
	// startAt: Start parsing at this argument. One normally skips the first argument, because it's typically the name of the program
	bool Parse(int argc, const char** argv, int startAt = 1);

	// Results
	bool        Has(const std::string& _short_or_long);      // Returns true if the option was specified
	std::string Get(const std::string& _short_or_long);      // Get an option's value. Returns default value if not specified.
	int         GetInt(const std::string& _short_or_long);   // Get an option and convert to int. Returns default value if not specified.
	int64_t     GetInt64(const std::string& _short_or_long); // Get an option and convert to int64. Returns default value if not specified.

private:
	Option* FindOption(const char* arg);
	bool    ValidateSanity();
};

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

inline void Args::ShowHelp() {
	int maxLong = 0;
	for (const auto& opt : Options)
		maxLong = (int) opt.Long.length() > maxLong ? (int) opt.Long.length() : maxLong;
	printf("%s\n", Usage.c_str());
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
}

inline bool Args::Parse(int argc, const char** argv, int startAt) {
	if (!ValidateSanity())
		return false;
	for (auto& opt : Options) {
		opt.Toggled = false;
		opt.Value   = "";
	}
	for (int i = startAt; i < argc; i++) {
		bool        atEnd = i == argc - 1;
		const char* arg   = argv[i];
		size_t      len   = strlen(arg);
		if (arg[0] == '-') {
			// option
			auto opt = FindOption(arg);
			if (!opt) {
				std::string a = arg;
				if ((a == "-h" || a == "-help" || a == "--help" || a == "-?" || a == "/?" || a == "/h" || a == "/help") && atEnd) {
					ShowHelp();
					return false;
				}
				printf("Unknown option '%s'\n", arg);
				return false;
			}
			if (opt->ExpectsValue && atEnd) {
				printf("Option %s expects a value, eg --%s <something>\n", arg, opt->Long.c_str());
				return false;
			}
			if (opt->ExpectsValue) {
				i++;
				opt->Value   = argv[i];
				opt->Toggled = true;
			} else {
				opt->Toggled = true;
			}
		} else {
			// positional parameter
			Params.push_back(arg);
		}
	}
	return true;
}

inline bool Args::Has(const std::string& _short_or_long) {
	for (const auto& opt : Options) {
		if ((opt.HasShort() && opt.Short == _short_or_long) || opt.Long == _short_or_long)
			return opt.Toggled;
	}
	printf("Option %s does not exist\n", _short_or_long.c_str());
	return false;
}

inline std::string Args::Get(const std::string& _short_or_long) {
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

inline int Args::GetInt(const std::string& _short_or_long) {
	return stoi(Get(_short_or_long));
}

inline int64_t Args::GetInt64(const std::string& _short_or_long) {
	return (int64_t) stoll(Get(_short_or_long));
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

inline bool Args::ValidateSanity() {
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
	return true;
}

} // namespace argparse

#endif
