#include "dbase.h"

pr_variable::pr_variable()
{
	depth = 1000000;
	filtered = true;

	written = false;
	read = false;
	scripted = false;
	asserted = false;
	aliased = false;
}
	
pr_variable::~pr_variable()
{
}

bool pr_variable::is(string name)
{
	return names.contains(name);
}

void pr_variable::combine(pr_variable v)
{
	names.merge(v.names);

	if ((!v.filtered || filtered) && (v.depth < depth || v.depth == depth && v.name.size() < name.size()))
	{
		name = v.name;
		depth = v.depth;
		filtered = v.filtered;
	}

	written = written || v.written;
	read = read || v.read;
	scripted = scripted || v.scripted;
	asserted = asserted || v.asserted;
	aliased = aliased || v.aliased;
}

void pr_variable::add_name(string name, bool is_filtered)
{
	int c = count(name.begin(), name.end(), '.');
	if ((!is_filtered || filtered) && (c < depth || c == depth && name.size() < this->name.size()))
	{
		this->name = name;
		depth = c;
		filtered = is_filtered;
	}

	names.insert(name);
}

void pr_variable::add_names(vector<string> name, vector<bool> is_filtered)
{
	for (int i = 0; i < name.size() && i < is_filtered.size(); i++)
		add_name(name[i], is_filtered[i]);
}

production_rule_set::production_rule_set()
{
}

production_rule_set::production_rule_set(string prsfile, string scriptfile, string mangle)
{
	load_prs(prsfile);
	load_script(scriptfile, mangle);
}

production_rule_set::~production_rule_set()
{
}

bool production_rule_set::filter_name(string name)
{
	for (int j = 0; j < (int)filter.size(); j++)
		if (strncmp(name.c_str(), filter[j].c_str(), filter[j].size()) == 0)
			return true;
	return false;
}

vector<bool> production_rule_set::filter_names(vector<string> names)
{
	vector<bool> result;
	result.reserve(names.size());
	for (int i = 0; i < (int)names.size(); i++)
		result.push_back(filter_name(names[i]));
	return result;
}

pr_index production_rule_set::indexof(string name)
{
	map<string, pr_index>::iterator loc;
	if (variable_map.find(name, &loc))
		return loc->second;
	return variables.end();
}

pr_index production_rule_set::set(string name)
{
	pr_index idx = indexof(name);
	if (idx == variables.end())
	{
		idx = new_var();
		idx->add_name(name, filter_name(name));
		variable_map.insert(name, idx);
	}
	return idx;
}

pr_index production_rule_set::set_written(string name)
{
	pr_index index = set(name);
	index->written = true;
	return index;
}

bool production_rule_set::is_written(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->written;
	return false;
}

pr_index production_rule_set::set_read(string name)
{
	pr_index index = set(name);
	index->read = true;
	return index;
}

bool production_rule_set::is_read(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->read;
	return false;
}

pr_index production_rule_set::set_scripted(string name)
{
	pr_index index = set(name);
	index->scripted = true;
	return index;
}

bool production_rule_set::is_scripted(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->scripted;
	return false;
}

pr_index production_rule_set::set_asserted(string name)
{
	pr_index index = set(name);
	index->asserted = true;
	return index;
}

bool production_rule_set::is_asserted(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->asserted;
	return false;
}

pr_index production_rule_set::set_aliased(string name)
{
	pr_index index = set(name);
	index->aliased = true;
	return index;
}

bool production_rule_set::is_aliased(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->aliased;
	return false;
}

void production_rule_set::add_pr(string line)
{
	int loc = -1;
	if (line.size() > 0 && line[0] == '=')
	{
		vector<string> names = split(line, "\" \t\n\r=");
		pr_index left  = indexof(names[0]);
		pr_index right = indexof(names[1]);

		if (left == variables.end() && right == variables.end())
		{
			left = new_var();
			variable_map.insert(names[0], left);
			variable_map.insert(names[1], left);
			left->add_names(names, filter_names(names));
			left->aliased = true;
		}
		else if (left == variables.end())
		{
			variable_map.insert(names[0], right);
			right->add_name(names[0], filter_name(names[0]));
			right->aliased = true;
		}
		else if (right == variables.end())
		{
			variable_map.insert(names[1], left);
			left->add_name(names[1], filter_name(names[0]));
			left->aliased = true;
		}
		else if (left != right)
		{
			left->combine(*right);
			left->aliased = true;
			replace_var(right, left);
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
	while (!feof(fenv))
	{
		string line = getline(fenv);
		
		if (line.size() > 0)
			add_pr(line);
	}

	fclose(fenv);
}

void production_rule_set::load_script(string filename, string mangle)
{
	vector<pr_index> initialized;
	FILE *fscr = fopen(filename.c_str(), "r");
	int delay = 0;
	while (!feof(fscr))
	{
		string line = trim(getline(fscr), "\n\r\t ");
		
		if (line.size() > 0)
		{
			string command = "";
			if (strncmp(line.c_str(), "set", 3) == 0)
			{
				char v;
				char vname[256];
				if (sscanf(line.c_str(), "set %s %c", vname, &v) == 2)
				{
					pr_index id = set_scripted(to_string(vname));
					string name = mangle_name(id->name, mangle);
					
					if (find(initialized.begin(), initialized.end(), id) != initialized.end())
						command = "" + name + " = " + to_string(v) + ";";
					else
					{
						init += "\t\t" + name + " = " + to_string(v) + ";\n";
						initialized.push_back(id);
					}
				}
			}
			else if (strncmp(line.c_str(), "assert", 3) == 0)
			{
				char v;
				char vname[256];
				if (sscanf(line.c_str(), "assert %s %c", vname, &v) == 2)
				{
					pr_index id = set_asserted(to_string(vname));
					string name = mangle_name(id->name, mangle);
					command = "if (" + name + " != " + to_string(v) + ") $display(\"assertion failed " + name + " == " + to_string(v) + "\");";
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
			else
				command = "$prsim_cmd(\"" + line + "\");";

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

struct channel_data
{
	channel_data()
	{
		num_bundles = 0;
		num_rails = 0;
	}

	channel_data(string bundle_name, int num_bundles, string rail_name, int num_rails)
	{
		this->bundle_name = bundle_name;
		this->num_bundles = num_bundles;
		this->rail_name = rail_name;
		this->num_rails = num_rails;
	}

	~channel_data()
	{
	}

	string bundle_name;
	string rail_name;
	int num_bundles;
	int num_rails;
};

struct channel
{
	channel() {}
	channel(string name, int num_req, int num_ack)
	{
		this->name = name;
		this->req.resize(num_req);
		this->ack.resize(num_ack);
	}
	~channel() {}

	string name;
	vector<channel_data> req;
	vector<channel_data> ack;
};

void production_rule_set::preview_script(string filename)
{
	FILE *fscr = fopen(filename.c_str(), "r");
	vector<channel> channels;
	while (!feof(fscr))
	{
		string line = getline(fscr);
		
		if (line.size() > 0)
		{
			if (strncmp(line.c_str(), "set", 3) == 0)
			{
				char v;
				char vname[256];
				if (sscanf(line.c_str(), "set %s %c", vname, &v) == 2)
					set_scripted(to_string(vname));
			}
			else if (strncmp(line.c_str(), "assert", 3) == 0)
			{
				char v;
				char vname[256];
				if (sscanf(line.c_str(), "assert %s %c", vname, &v) == 2)
					set_asserted(to_string(vname));
			}
			else if (strncmp(line.c_str(), "channel", 7) == 0)
			{
				char vname[256] = "";
				char vproto[256] = "";
				char vreq[256] = "";
				char vack[256] = "";
				if (sscanf(line.c_str(), "channel %s %s %s %s", vname, vproto, vreq, vack) >= 2)
				{
					set_read("Reset");
					int num_req = 0;
					for (char *cptr = vreq; cptr && *cptr; cptr++) {
						if (cptr == vreq || *(cptr-1) == '&' || *(cptr-1) == '|') {
							int v = atoi(cptr);
							if (v >= num_req)
								num_req = v+1;
						}
					}

					int num_ack = 0;
					for (char *cptr = vack; cptr && *cptr; cptr++) {
						if (cptr == vack || *(cptr-1) == '&' || *(cptr-1) == '|') {
							int v = atoi(cptr);
							if (v >= num_ack)
								num_ack = v+1;
						}
					}

					channels.push_back(channel(vname, num_req, num_ack));
				}
			}
			else if (strncmp(line.c_str(), "request", 7) == 0 || strncmp(line.c_str(), "enable", 6) == 0 || strncmp(line.c_str(), "acknowledge", 11) == 0)
			{
				char vtype[256];
				char vname[256];
				int vindex = 0;
				char vdef[256];
				if (sscanf(line.c_str(), "%s %s %d %s", vtype, vname, &vindex, vdef) == 4)
				{
					set_read("Reset");
					
					int idx = -1;
					for (int i = 0; i < channels.size() && idx < 0; i++)
						if (channels[i].name == to_string(vname))
							idx = i;

					if (idx >= 0)
					{
						char sRail[32], R[32];
						char sBundle[32], B[32];
						int numBundles = 0, b;
						int numRails = 0, r;
						if (sscanf(vdef, "%[^;];%[^-;]-;%dx1of%d", B, R, &b, &r) == 4)
						{
							strcpy(sBundle, B);
							strcpy(sRail, R);
							numBundles = b;
							numRails = r;
						}
						else if (sscanf(vdef, "%[^;];%[^-;];%dx1of%d", B, R, &b, &r) == 4)
						{
							strcpy(sBundle, B);
							strcpy(sRail, R);
							numBundles = b;
							numRails = r;
						}
						else if (sscanf(vdef, "%dx1of%d", &b, &r) == 2)
						{
							numBundles = b;
							numRails = r;
						}
						else if (sscanf(vdef, "%[^-;]-;1of%d", R, &r) == 2)
						{
							strcpy(sRail, R);
							numRails = r;
						}
						else if (sscanf(vdef, "%[^-;];1of%d", R, &r) == 2)
						{
							strcpy(sRail, R);
							numRails = r;
						}
						else if (sscanf(vdef, "1of%d", &r) == 1)
							numRails = r;

						char nodeName[256];
	
						for (int i = 0; i < numBundles; i++) {
							for (int j = 0; j < numRails; j++) {
								if (numBundles > 0 && numRails > 0)
									sprintf(nodeName, "%s.%s[%d].%s[%d]", vname, sBundle, i, sRail, j);
								else if (numBundles == 0 && numRails > 0)
									sprintf(nodeName, "%s.%s[%d]", vname, sRail, j);
								else if (numBundles > 0 && numRails == 0)
									sprintf(nodeName, "%s.%s[%d].%s", vname, sBundle, i, sRail);
								else if (numBundles == 0 && numRails == 0)
									sprintf(nodeName, "%s.%s", vname, sRail);
								
								set_read(nodeName);
							}
						}

						if (strncmp(vtype, "request", 7) == 0)
							channels[idx].req[vindex] = channel_data(sBundle, numBundles, sRail, numRails);
						else
							channels[idx].ack[vindex] = channel_data(sBundle, numBundles, sRail, numRails);
					}

				}
			}
			else if (strncmp(line.c_str(), "inject", 6) == 0)
			{
				char vname[256];
				char vtype[256];
				if (sscanf(line.c_str(), "inject %s %s", vname, vtype) == 2)
				{
					set_read("Reset");

					int idx = -1;
					for (int i = 0; i < channels.size() && idx < 0; i++)
						if (channels[i].name == to_string(vname))
							idx = i;

					if (idx >= 0)
					{
						vector<channel_data> *dat = NULL;
						if (strncmp(vtype, "request", 7) == 0)
							dat = &channels[idx].req;
						else
							dat = &channels[idx].ack;

						for (int c = 0; c < dat->size(); c++)
						{
							char nodeName[256];
							int numBundles = (*dat)[c].num_bundles;
							int numRails = (*dat)[c].num_rails;
							const char *sBundle = (*dat)[c].bundle_name.c_str();
							const char *sRail = (*dat)[c].rail_name.c_str();
							
							for (int i = 0; i < numBundles; i++) {
								for (int j = 0; j < numRails; j++) {
									if (numBundles > 0 && numRails > 0)
										sprintf(nodeName, "%s.%s[%d].%s[%d]", vname, sBundle, i, sRail, j);
									else if (numBundles == 0 && numRails > 0)
										sprintf(nodeName, "%s.%s[%d]", vname, sRail, j);
									else if (numBundles > 0 && numRails == 0)
										sprintf(nodeName, "%s.%s[%d].%s", vname, sBundle, i, sRail);
									else if (numBundles == 0 && numRails == 0)
										sprintf(nodeName, "%s.%s", vname, sRail);
									
									set_written(nodeName);
								}
							}
						}
					}
				}
			}
		}
	}

	fclose(fscr);	
}

void production_rule_set::write_dbase(string filename)
{
	FILE *fdbase = fopen(filename.c_str(), "w");
	fprintf(fdbase, "%s\n", join(filter, " ").c_str());

	for (pr_index i = variables.begin(); i != variables.end(); i++)
	{
		fprintf(fdbase, "%d%d%d%d%d", i->read, i->written, i->scripted, i->asserted, i->aliased);
		for (int j = 0; j < i->names.capacity; j++)
			for (vector<string>::iterator k = i->names.buckets[j].begin(); k != i->names.buckets[j].end(); k++)
				fprintf(fdbase, " %s", k->c_str());
		fprintf(fdbase, "\n");
	}
	fflush(fdbase);
	fclose(fdbase);
}

void production_rule_set::load_dbase(string filename)
{
	FILE *fdbase = fopen(filename.c_str(), "r");
	bool first = true;
	while (!feof(fdbase))
	{
		string line = getline(fdbase);
		
		if (line.size() > 0)
		{
			if (first)
			{
				filter = split(line, " \n\r\t");
				first = false;
			}
			else
			{
				pr_index var = new_var();
		
				if (line[0] == '1')
					var->read = true;
				if (line[1] == '1')
					var->written = true;
				if (line[2] == '1')
					var->scripted = true;
				if (line[3] == '1')
					var->asserted = true;
				if (line[4] == '1')
					var->aliased = true;
	
				vector<string> names = split(line.substr(6), " \n\t");
				var->add_names(names, filter_names(names));
				for (int i = 0; i < (int)names.size(); i++)
					variable_map.insert(names[i], var);
			}
		}
	}

	fclose(fdbase);
}

pr_index production_rule_set::new_var()
{
	variables.push_back(pr_variable());
	pr_index result = variables.end();
	result--;	
	return result;
}

void production_rule_set::delete_var(pr_index index)
{
	for (int i = 0; i < index->names.capacity; i++)
		for (int j = 0; j < (int)index->names.buckets[i].size(); j++)
			variable_map.erase(index->names.buckets[i][j]);

	variables.erase(index);
}

void production_rule_set::replace_var(pr_index from, pr_index to)
{
	for (int i = 0; i < from->names.capacity; i++)
		for (int j = 0; j < (int)from->names.buckets[i].size(); j++)
		{
			map<string, pr_index>::iterator temp;
			if (variable_map.find(from->names.buckets[i][j], &temp))
				temp->second = to;
		}

	variables.erase(from);
}

bool production_rule_set::has_prs()
{
	for (pr_index i = variables.begin(); i != variables.end(); i++)
		if (i->read || i->written)
			return true;
	return false;
}

