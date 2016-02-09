#include "dbase.h"

pr_variable::pr_variable()
{
	written = false;
	read = false;
	scripted = false;
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

string pr_variable::name()
{
	int index = -1;
	int length = 1000;
	int num = 1000;
	for (int i = 0; i < names.size(); i++)
	{
		int c = count(names[i].begin(), names[i].end(), '.');
		if (c < num || (c == num && names[i].size() < length))
		{
			num = c;
			index = i;
			length = names[i].size();
		}
	}
	if (index != -1)
		return names[index];
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
		vector<string> names = split(line, " \t\n\"\'+->&|~");
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
	while (!feof(fenv))
	{
		fgets(buffer, 1023, fenv);
		string line = buffer;
		add_pr(line);
	}

	fclose(fenv);
}

void production_rule_set::load_script(string filename, string mangle)
{
	FILE *fscr = fopen(filename.c_str(), "r");
	char buffer[256];
	int delay = 0;
	while (!feof(fscr))
	{
		memset(buffer, 0, 256);
		if (fgets(buffer, 256, fscr) == 0)
			break;

		string command = "";
		if (strncmp(buffer, "mode run", 8) == 0)
			command = "$prsim_resetmode(0);";
		else if (strncmp(buffer, "mode reset", 10) == 0)
			command = "$prsim_resetmode(1);";
		else if (strncmp(buffer, "norandom", 8) == 0)
			command = "$prsim_mkrandom(0);";
		else if (strncmp(buffer, "random", 6) == 0)
		{
			command = "$prsim_mkrandom(1);";
			int m0 = -1, m1 = -1;
			if (sscanf(buffer, "random %d %d", &m0, &m1) == 2)
				command += "\n\t\t$prsim_random_setrange(" + to_string(m0) + ", " + to_string(m1) + ");";
		}
		else if (strncmp(buffer, "dumptc", 6) == 0)
		{
			char dumptc_file[256];
			if (sscanf(buffer, "dumptc %s", dumptc_file) == 1)
				command = "$prsim_dump_tc(\"" + string(dumptc_file) + "\");";
		}
		else if (strncmp(buffer, "status", 6) == 0)
		{
			char v;
			if (sscanf(buffer, "status %c", &v) == 1)
			{
				if (v == 'X')
					command = "$prsim_status(2);";
				else
					command = "$prsim_status(" + to_string(v) + ");";
			}
		}
		else if (strncmp(buffer, "watchall", 8) == 0)
			command = "$prsim_watchall();";
		else if (strncmp(buffer, "watch", 5) == 0)
		{
			char vname[256];
			if (sscanf(buffer, "watch %s", vname) == 1)
			{
				string name = variables[set(to_string(vname))].name();
				command = "$prsim_watch(\"" + name + "\");";
			}
		}
		else if (strncmp(buffer, "set", 3) == 0)
		{
			char v;
			char vname[256];
			if (sscanf(buffer, "set %s %c", vname, &v) == 2)
			{
				string name = variables[set(to_string(vname))].name();
				command = "" + mangle_name(name, mangle) + " = " + to_string(v) + ";";
				set_scripted(name);
			}
		}
		else if (strncmp(buffer, "advance", 7) == 0)
		{
			int n = -1;
			if (sscanf(buffer, "advance %d", &n) == 1)
				delay += n;
		}
		else if (strncmp(buffer, "step", 4) == 0)
		{
			int n = -1;
			if (sscanf(buffer, "step %d", &n) == 1)
				delay += n*10;
		}
		else if (strncmp(buffer, "cycle", 5) == 0)
			delay += 50;

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
	while (!feof(fdbase))
	{
		if (fgets(buffer, 1023, fdbase) == 0)
			break;
		
		variables.push_back(pr_variable());

		if (buffer[0] == '1')
			variables.back().read = true;
		if (buffer[1] == '1')
			variables.back().written = true;
		if (buffer[2] == '1')
			variables.back().aliased = true;

		variables.back().names = split(string(buffer).substr(4), " \n\t");
	}
	fclose(fdbase);
}
