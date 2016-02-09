#include "common.h"
#include "dbase.h"


int main(int argc, char **argv)
{
	production_rule_set prs;
	
	vector<string> allowed;
	char buffer[1024];
	while (!feof(stdin))
	{
		fgets(buffer, 1023, stdin);
		string line = buffer;
		vector<string> names = split(line, "&| \"\'->+\n\r\t~");
		bool found = false;
		for (int i = 0; i < (int)names.size() && !found; i++)
			for (int j = 2; j < argc && !found; j++)
				found = (strncmp(names[i].c_str(), argv[j], strlen(argv[j])) == 0);

		if (!found)
		{
			prs.add_pr(line);
			printf("%s", buffer);
		}
	}

	if (argc > 1)
		prs.write_dbase(to_string(argv[1]));
}

