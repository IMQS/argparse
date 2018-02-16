#include "argparse.h"
#include <assert.h>

void Simple() {
	argparse::Args args("Usage: something [options...] param1 param2");
	args.AddSwitch("f", "force", "Force a thing");
	args.AddSwitch("p", "preserve", "Preserve goodness");
	args.AddValue("o", "outfile", "File to write to");
	args.AddValue("c", "count", "Max count", "7");
	args.AddValue("", "justlong", "This has no short form");

	printf("\n");
	args.ShowHelp();
	printf("\n");

	{
		const char* a[6] = {"thing.exe", "-f", "--outfile", "myfile", "pos1", "pos2"};
		assert(args.Parse(6, a));
		assert(args.Has("f"));
		assert(args.Has("o"));
		assert(!args.Has("preserve"));
		assert(args.Get("outfile") == "myfile");
		assert(args.GetInt("c") == 7);
		assert(args.GetInt64("c") == 7);
		assert(args.Params.size() == 2);
		assert(args.Params[0] == "pos1");
		assert(args.Params[1] == "pos2");
	}

	{
		const char* a[2] = {"thing.exe", "-bad"};
		assert(!args.Parse(2, a));
	}

	{
		// If -h or -? or /? or --help is the only option, then show help
		const char* a[2] = {"thing.exe", "-h"};
		const char* b[2] = {"thing.exe", "--help"};
		printf("\n-- Should show help --\n");
		assert(!args.Parse(2, a));
		printf("\n-- Should show help --\n");
		assert(!args.Parse(2, b));
	}
}

int Foo(argparse::Args& args) {
	printf("Foo %s\n", args.Has("foo1") ? "foo1" : "nothing");
	return 0;
}

int Bar(argparse::Args& args) {
	return 1;
}

void WithCommands() {
	argparse::Args args("thing [options...] <command>");

	// top-level option
	args.AddSwitch("v", "verbose", "More verbose");

	auto cmdFoo = args.AddCommand("foo", "Do the foo thing", Foo);
	cmdFoo->AddSwitch("f", "foo1", "foo1 switch");

	auto cmdBar = args.AddCommand("bar", "Do the bar thing", Bar);

	{
		const char* a[2] = {"thing.exe", "nop"};
		assert(!args.Parse(2, a));
	}
	{
		const char* a[4] = {"thing.exe", "-v", "foo", "--foo1"};
		assert(args.Parse(4, a));
		assert(args.Has("v"));
		assert(args.WhichCommand() == cmdFoo);
		assert(args.ExecCommand() == 0);
	}
	{
		const char* a[3] = {"thing.exe", "bar"};
		assert(args.Parse(2, a));
		assert(args.WhichCommand() == cmdBar);
		assert(args.ExecCommand() == 1);
	}
}

int main(int argc, char** argv) {
	Simple();
	WithCommands();
	return 0;
}