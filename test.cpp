#include "argparse.h"
#include <assert.h>

int main(int argc, char** argv) {
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

	return 0;
}