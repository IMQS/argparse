# ArgParse
ArgParse is a single-file header-only library for parsing command line arguments in C++11

Features:
* Single file header-only C++11 library (250 lines)
* Short form and long form of arguments
* Default values for optional arguments
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

To print out full help for all options, use args.ShowHelp().

Example:

    int main(int argc, char** argv) {
        argparse::Args args("Usage: myprogram [options...] param1");
        args.AddSwitch("f", "force", "Force a certain thing");
        args.AddValue("o", "outfile", "Write output to file");
        args.AddValue("t", "timeout", "Timeout in seconds", "60");
        if (!args.Parse(argc, argv))
            return 1;

        if (args.Has("force")) { /* ... */ }
        int timeout = args.GetInt("timeout"); // default value of 60, if not specified
        // ...
    }

Copyright: IMQS Software  
License: MIT



