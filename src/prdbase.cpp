#include "common.h"
#include "dbase.h"


int main(int argc, char **argv)
{
	production_rule_set prs;

	int fidx = 3;

	for (int j = fidx; j < argc; j++)
		prs.filter.push_back(argv[j]);
	
	vector<string> allowed;
	int count = 0;
	bool has_prs = false;
	while (!feof(stdin))
	{
		string line = getline(stdin);

		if ((int)line.size() > 0)
		{
			vector<string> names = split(line, "&| \"\'->+\n\r\t~()");
			bool filtered = false;
			for (int i = 0; i < (int)names.size() && !filtered; i++)
				for (int j = 0; j < (int)prs.filter.size() && !filtered; j++)
					filtered = (strncmp(names[i].c_str(), prs.filter[j].c_str(), prs.filter[j].size()) == 0);

			if ((line.size() > 0 && line[0] == '=') || !filtered)
				prs.add_pr(line);
			if (line.size() > 0 && line[0] != '=' && !filtered)
			{
				printf("%s", line.c_str());
				has_prs = true;
			}
		}
	}

	if (argc > 1)
		prs.preview_script(string(argv[1]));
	
	for (pr_index i = prs.variables.begin(); i != prs.variables.end(); i++)
	{
		if ((!i->written && !i->read && !i->scripted && !i->asserted) || i->names.size() == 0)
		{
			pr_index temp = i;
			i--;
			prs.delete_var(temp);
		}
		else
		{
			for (int j = 0; j < (int)i->names.size()-1; j++)
				printf("= \"%s\" \"%s\"\n", i->names[j].c_str(), i->names[j+1].c_str());
		}
	}

	if (argc > 2)
		prs.write_dbase(to_string(argv[2]));

	return 0;
}

