# ArgParse
ArgParse is a single-file header-only library for parsing command line arguments in C++11

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

Commands can be added with `AddCommand`. If one or more commands have been defined, then the user _must_ enter a valid command as the first parameter. Subsequent parameters are fed to the chosen command. Commands can have options and switches, just like the master `Args` object.

To print out full help for all options, use `args.ShowHelp()`. This is invoked automatically when the user types `help`.

Simple example:
```cpp
int main(int argc, char** argv) {
    argparse::Args args("Usage: myprogram [options...] param1");
    args.AddSwitch("f", "force", "Force a certain thing");
    args.AddValue("o", "outfile", "Write output to file");
    args.AddValue("t", "timeout", "Timeout in seconds", "60");
    if (!args.Parse(argc, (const char**) argv))
        return 1;

    if (args.Has("force")) { /* ... */ }
    int timeout = args.GetInt("timeout"); // default value of 60, if not specified
    // ...
}
```

Example using commands:
```cpp
int fooFunc(argparse::Args& args) {
    // ...
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
```

Copyright: IMQS Software  
License: MIT



