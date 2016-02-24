#include "dbase.h"

pr_variable::pr_variable()
{
	written = false;
	read = false;
	scripted = false;
	aliased = false;
}
	
pr_variable::~pr_variable()
{
}

bool pr_variable::is(string name)
{
	return find(names.begin(), names.end(), name) != names.end();
}

void pr_variable::combine(pr_variable v)
{
	names.insert(names.end(), v.names.begin(), v.names.end());
	sort(names.begin(), names.end());
	names.resize(unique(names.begin(), names.end()) - names.begin());
	written = written || v.written;
	read = read || v.read;
	scripted = scripted || v.scripted;
	aliased = aliased || v.aliased;
}

void pr_variable::add_name(string name)
{
	names.push_back(name);
	sort(names.begin(), names.end());
	names.resize(unique(names.begin(), names.end()) - names.begin());
}

void pr_variable::add_names(vector<string> name)
{
	names.insert(names.end(), name.begin(), name.end());
	sort(names.begin(), names.end());
	names.resize(unique(names.begin(), names.end()) - names.begin());
}

string production_rule_set::name(int var)
{
	int index = -1;
	int length = 1000;
	int num = 1000;

	//int found_index = -1;
	//int found_length = 1000;
	//int found_num = 1000;
	for (int i = 0; i < variables[var].names.size(); i++)
	{
		bool found = false;
		for (int j = 0; j < (int)filter.size() && !found; j++)
			found = (strncmp(variables[var].names[i].c_str(), filter[j].c_str(), filter[j].size()) == 0);

		int c = count(variables[var].names[i].begin(), variables[var].names[i].end(), '.');
		if (!found)
		{
			if (c < num || (c == num && variables[var].names[i].size() < length))
			{
				num = c;
				index = i;
				length = variables[var].names[i].size();
			}
		}
		/*else
		{
			if (c < found_num || (c == found_num && variables[var].names[i].size() < found_length))
			{
				found_num = c;
				found_index = i;
				found_length = variables[var].names[i].size();
			}
		}*/
	}
	if (index != -1)
		return variables[var].names[index];
	//else if (found_index != -1)
	//	return variables[var].names[found_index];
	else
		return "";
}

production_rule_set::production_rule_set() {}
production_rule_set::production_rule_set(string prsfile, string scriptfile, string mangle)
{
	load_prs(prsfile);
	load_script(scriptfile, mangle);
}
production_rule_set::~production_rule_set() {}

int production_rule_set::indexof(string name)
{
	for (int i = 0; i < variables.size(); i++)
		if (variables[i].is(name))
			return i;
	return -1;
}

int production_rule_set::set(string name)
{
	int idx = indexof(name);
	if (idx == -1)
	{
		idx = variables.size();
		variables.push_back(pr_variable());
		variables.back().names.push_back(name);
	}
	return idx;
}

void production_rule_set::set_written(string name)
{
	variables[set(name)].written = true;
}

bool production_rule_set::is_written(string name)
{
	for (int j = 0; j < (int)variables.size(); j++)
		if (variables[j].is(name))
			return variables[j].written;
	return false;
}

void production_rule_set::set_read(string name)
{
	variables[set(name)].read = true;
}

bool production_rule_set::is_read(string name)
{
	for (int j = 0; j < (int)variables.size(); j++)
		if (variables[j].is(name))
			return variables[j].read;
	return false;
}

void production_rule_set::set_scripted(string name)
{
	variables[set(name)].scripted = true;
}

bool production_rule_set::is_scripted(string name)
{
	for (int j = 0; j < (int)variables.size(); j++)
		if (variables[j].is(name))
			return variables[j].scripted;
	return false;
}

void production_rule_set::set_aliased(string name)
{
	variables[set(name)].aliased = true;
}

bool production_rule_set::is_aliased(string name)
{
	for (int j = 0; j < (int)variables.size(); j++)
		if (variables[j].is(name))
			return variables[j].aliased;
	return false;
}

void production_rule_set::add_pr(string line)
{
	int loc = -1;
	if (line.size() > 0 && line[0] == '=')
	{
		vector<string> names = split(line, "\" \t\n\r=");
		int left = indexof(names[0]);
		int right = indexof(names[1]);

		if (left == -1 && right == -1)
		{
			variables.push_back(pr_variable());
			variables.back().add_names(names);
			variables.back().aliased = true;
		}
		else if (left == -1)
		{
			variables[right].add_name(names[0]);
			variables[right].aliased = true;
		}
		else if (right == -1)
		{
			variables[left].add_name(names[1]);
			variables[left].aliased = true;
		}
		else if (left != right)
		{
			variables[left].combine(variables[right]);
			variables[left].aliased = true;
			variables.erase(variables.begin() + right);
		}
	}
	else if ((loc = line.find("->")) != -1)
	{
		bool done = false;
		while (!done)
		{
			if (strncmp(line.c_str(), "weak", 4) == 0)
				line = line.substr(5);
			else if (strncmp(line.c_str(), "after", 5) == 0)
				line = line.substr(8);
			else
				done = true;
		}

		vector<string> names = split(line, " \t\n\"\'+->&|~()");
		for (int i = 0; i < names.size(); i++)
		{
			if (i == (int)names.size()-1)
				set_written(names[i]);
			else
				set_read(names[i]);
		}
	}
}

void production_rule_set::load_prs(string filename)
{
	// parse the environment prs file and figure out what is driven by the environment
	FILE *fenv = fopen(filename.c_str(), "r");
	char buffer[1024];
	string line;
	while (!feof(fenv))
	{
		line.clear();
		while (!feof(fenv) && (line.size() == 0 || (line[line.size()-1] != '\n' && line[line.size()-1] != '\r')))
			if (fgets(buffer, 1023, fenv) > 0)
				line += string(buffer);
		
		if (line.size() > 0)
			add_pr(line);
	}

	fclose(fenv);
}

void production_rule_set::load_script(string filename, string mangle)
{
	vector<int> initialized;
	FILE *fscr = fopen(filename.c_str(), "r");
	char buffer[256];
	string line;
	int delay = 0;
	while (!feof(fscr))
	{
		line.clear();
		while (!feof(fscr) && (line.size() == 0 || (line[line.size()-1] != '\n' && line[line.size()-1] != '\r')))
			if (fgets(buffer, 1023, fscr) > 0)
				line += string(buffer);
		
		if (line.size() > 0)
		{
			string command = "";
			if (strncmp(line.c_str(), "mode run", 8) == 0)
				command = "$prsim_resetmode(0);";
			else if (strncmp(line.c_str(), "mode reset", 10) == 0)
				command = "$prsim_resetmode(1);";
			else if (strncmp(line.c_str(), "norandom", 8) == 0)
				command = "$prsim_mkrandom(0);";
			else if (strncmp(line.c_str(), "random", 6) == 0)
			{
				command = "$prsim_mkrandom(1);";
				int m0 = -1, m1 = -1;
				if (sscanf(line.c_str(), "random %d %d", &m0, &m1) == 2)
					command += "\n\t\t$prsim_random_setrange(" + to_string(m0) + ", " + to_string(m1) + ");";
			}
			else if (strncmp(line.c_str(), "dumptc", 6) == 0)
			{
				char dumptc_file[256];
				if (sscanf(line.c_str(), "dumptc %s", dumptc_file) == 1)
					command = "$prsim_dump_tc(\"" + string(dumptc_file) + "\");";
			}
			else if (strncmp(line.c_str(), "status", 6) == 0)
			{
				char v;
				if (sscanf(line.c_str(), "status %c", &v) == 1)
				{
					if (v == 'X')
						command = "$prsim_status(2);";
					else
						command = "$prsim_status(" + to_string(v) + ");";
				}
			}
			else if (strncmp(line.c_str(), "watchall", 8) == 0)
				command = "$prsim_watchall();";
			else if (strncmp(line.c_str(), "watch", 5) == 0)
			{
				char vname[256];
				if (sscanf(line.c_str(), "watch %s", vname) == 1)
				{
					string name = this->name(set(to_string(vname)));
					command = "$prsim_watch(\"" + name + "\");";
				}
			}
			else if (strncmp(line.c_str(), "set", 3) == 0)
			{
				char v;
				char vname[256];
				if (sscanf(line.c_str(), "set %s %c", vname, &v) == 2)
				{
					int id = set(to_string(vname));
					string name = this->name(id);
					set_scripted(name);
					
					if (find(initialized.begin(), initialized.end(), id) != initialized.end())
						command = "" + mangle_name(name, mangle) + " = " + to_string(v) + ";";
					else
					{
						init += "\t\t" + mangle_name(name, mangle) + " = " + to_string(v) + ";\n";
						initialized.push_back(id);
					}
				}
			}
			else if (strncmp(line.c_str(), "advance", 7) == 0)
			{
				int n = -1;
				if (sscanf(line.c_str(), "advance %d", &n) == 1)
					delay += n;
			}
			else if (strncmp(line.c_str(), "step", 4) == 0)
			{
				int n = -1;
				if (sscanf(line.c_str(), "step %d", &n) == 1)
					delay += n*10;
			}
			else if (strncmp(line.c_str(), "cycle", 5) == 0)
				delay += 20;

			if (command != "")
			{
				script += "\t\t";
				if (delay > 0)
				{
					script += "#" + to_string(delay) + " ";
					delay = 0;
				}
				script += command + "\n";
			}
		}
	}

	script += "\t\t";
	if (delay > 0)
	{
		script += "#" + to_string(delay) + " ";
		delay = 0;
	}
	script += "$finish;\n";

	fclose(fscr);	
}

void production_rule_set::write_dbase(string filename)
{
	FILE *fdbase = fopen(filename.c_str(), "w");
	fprintf(fdbase, "%s\n", join(filter, " ").c_str());

	for (int i = 0; i < variables.size(); i++)
	{
		fprintf(fdbase, "%d%d%d", variables[i].read, variables[i].written, variables[i].aliased);
		for (int j = 0; j < (int)variables[i].names.size(); j++)
			fprintf(fdbase, " %s", variables[i].names[j].c_str());
		fprintf(fdbase, "\n");
	}
	fclose(fdbase);
}

void production_rule_set::load_dbase(string filename)
{
	FILE *fdbase = fopen(filename.c_str(), "r");
	char buffer[1024];
	string line;
	bool first = true;
	while (!feof(fdbase))
	{
		line.clear();
		while (!feof(fdbase) && (line.size() == 0 || (line[line.size()-1] != '\n' && line[line.size()-1] != '\r')))
			if (fgets(buffer, 1023, fdbase) > 0)
				line += string(buffer);
		
		if (line.size() > 0)
		{
			if (first)
			{
				filter = split(line, " \n\r\t");
				first = false;
			}
			else
			{
				variables.push_back(pr_variable());
		
				if (line[0] == '1')
					variables.back().read = true;
				if (line[1] == '1')
					variables.back().written = true;
				if (line[2] == '1')
					variables.back().aliased = true;
	
				variables.back().names = split(line.substr(4), " \n\t");
			}
		}
	}
	fclose(fdbase);
}
