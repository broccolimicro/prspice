#include "common.h"
#include "dbase.h"


int main(int argc, char **argv)
{
	production_rule_set prs;
	
	vector<string> allowed;
	char buffer[1024];
	string line;
	while (!feof(stdin))
	{
		line.clear();
		while (!feof(stdin) && (line.size() == 0 || (line[line.size()-1] != '\n' && line[line.size()-1] != '\r')))
			if (fgets(buffer, 1023, stdin) > 0)
				line += string(buffer);
		
		if (line.size() > 0)
		{
			vector<string> names = split(line, "&| \"\'->+\n\r\t~()");
			bool found = false;
			for (int i = 0; i < (int)names.size() && !found; i++)
				for (int j = 2; j < argc && !found; j++)
					found = (strncmp(names[i].c_str(), argv[j], strlen(argv[j])) == 0);

			if ((line.size() > 0 && line[0] == '=') || !found)
				prs.add_pr(line);
			if (!found)
				printf("%s", line.c_str());
		}
	}

	for (int i = (int)prs.variables.size()-1; i >= 0; i--)
	{
		for (int j = (int)prs.variables[i].names.size()-1; j >= 0; j--)
		{
			bool found = false;
			for (int k = 2; k < argc && !found; k++)
				if (strncmp(prs.variables[i].names[j].c_str(), argv[k], strlen(argv[k])) == 0)
				{
					prs.variables[i].names.erase(prs.variables[i].names.begin() + j);
					found = true;
				}
		}

		if (prs.variables[i].names.size() == 0)
			prs.variables.erase(prs.variables.begin() + i);
	}

	if (argc > 1)
		prs.write_dbase(to_string(argv[1]));
}

