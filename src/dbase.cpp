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
	reset = "Reset";
}

production_rule_set::production_rule_set(string prsfile, string scriptfile, string mangle)
{
	reset = "Reset";
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

vector<pr_index> production_rule_set::set_all_recurse(char *pre, int pre_length, char *post, int post_length)
{
	int start = -1;
	int end = -1;
	int step = -1;
	char *arr = NULL;
	char *cptr = NULL;
	for (cptr = post; *cptr != '\0'; cptr++)
	{
		if (*cptr == ']') {
			if (end < 0) {
				start = -1;
				arr = NULL;
			} else
				break;
		} if (start < 0) {
			if (*cptr == '[') {
				arr = cptr+1;
				start = atoi(cptr+1);
			}
		} else if (end < 0) {
			if (*cptr == ':')
				end = atoi(cptr+1);
		} else if (step < 0) {
			if (*cptr == ':')
				step = atoi(cptr+1);
		}
	}

	if (arr)
		*(arr-1) = '\0';

	int last = pre_length + snprintf(pre+pre_length, post_length - pre_length, "%s", post);

	if (arr)
		*(arr-1) = '[';

	if (start < 0)
		return vector<pr_index>(1, set(string(pre)));
	else
	{
		if (step < 0)
			step = 1;
		if (end < 0)
			end = start+1;

		vector<pr_index> result;
		int i;
		for (i = start; i < end; i += step)
		{
			pre_length = last + snprintf(pre+last, post_length-last, "[%d]", i);
			vector<pr_index> temp = set_all_recurse(pre, pre_length, cptr+1, post_length);
			result.insert(result.end(), temp.begin(), temp.end());
		}
		return result;
	}
	return vector<pr_index>();
}

vector<pr_index> production_rule_set::set_all(string name)
{
	char sNodes[1024];
	strcpy(sNodes, name.c_str());
	int length = strlen(sNodes)+1;
	char pre[length];
	pre[0] = '\0';
	return set_all_recurse(pre, 0, sNodes, length);
}

vector<pr_index> production_rule_set::set_written(string name)
{
	vector<pr_index> index = set_all(name);
	for (int i = 0; i < (int)index.size(); i++)
		index[i]->written = true;
	return index;
}

bool production_rule_set::is_written(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->written;
	return false;
}

vector<pr_index> production_rule_set::set_read(string name)
{
	vector<pr_index> index = set_all(name);
	for (int i = 0; i < (int)index.size(); i++)
		index[i]->read = true;
	return index;
}

bool production_rule_set::is_read(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->read;
	return false;
}

vector<pr_index> production_rule_set::set_scripted(string name)
{
	vector<pr_index> index = set_all(name);
	for (int i = 0; i < (int)index.size(); i++)
		index[i]->scripted = true;
	return index;
}

bool production_rule_set::is_scripted(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->scripted;
	return false;
}

vector<pr_index> production_rule_set::set_asserted(string name)
{
	vector<pr_index> index = set_all(name);
	for (int i = 0; i < (int)index.size(); i++)
		index[i]->asserted = true;
	return index;
}

bool production_rule_set::is_asserted(string name)
{
	pr_index index = indexof(name);
	if (index != variables.end())
		return index->asserted;
	return false;
}

vector<pr_index> production_rule_set::set_aliased(string name)
{
	vector<pr_index> index = set_all(name);
	for (int i = 0; i < (int)index.size(); i++)
		index[i]->aliased = true;
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
			/*if (strncmp(line.c_str(), "set", 3) == 0)
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
			else*/ if (strncmp(line.c_str(), "advance", 7) == 0)
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

/*
== General ==
  help - display this message
  exit - terminate
  source <file> - read in a script file
  array <variable> <range> <command> - execute multiple of one command
  save <file> - save a simulation checkpoint to the specified file
  restore <file> - restore simulation from a checkpoint

== Timing ==
  random [min max] - random timings
  random_seed <seed> - set random number seed
  norandom - deterministic timings
  random_excl on|off - turn on/off random exclhi/lo firings
  after <n> <minu> <maxu> <mind> <maxd> - node set to random times within range

== Running Simulation ==
  mode reset|run|unstab|nounstab - set running mode
  initialize - initialize the simulation
  step [<steps>] - run for <steps> simulation steps (default is 1)
  advance [<duration>] - run for <duration> units of simulation time (default is 1)
  cycle [<node>] - run until the next transition on <node> (default is forever)
  break <node> - set a breakpoint on <node>
  break-on-warn - stops/doesn't stop simulation on instability/inteference
  exit-on-warn - like break-on-warn, but exits prsim

== Setting/Getting Node Values ==
  set <variable> <value> - set the node, bus, or vector <variable> to specified <value>
  get <variable> - get value of a node, bus, or vector <variable>
  assert <variable> <value> - assert that the node, bus, or vector <variable> is <value>
  seu <node> 0|1|X <start-delay> <dur> - Delayed SEU event on node lasting for <dur> units
  uget <n> - get value of node <n> but report its canonical name
  watch <n> - add watchpoint for <n>
  unwatch <n> - delete watchpoint for <n>
  watchall - watch all nodes

== Channel Commands ==
  bundle <nodes> [low|high] - specify a bundle (1ofN)
  vector <nodes> [low|high] - specify a vector (NofN)
  channel <name> <protocol> [<request> [<acknowledge>]] - create channel
  clocked_bus <nodes> <clk> posedge|negedge [half|full] [invert] [signed] - create clocked bus
  clock_source <node> [wait|<delay_up> <delay_dn>] [low|high] - set a clock source that toggles the node
  set_reset <name> - set the reset node that's used to reset the channel interfaces
  inject <name> (request|acknowledge) [init] [loop] <file> - inject values in <file> into channel <name>
  expect <name> (request|acknowledge) [loop] <file> - check channel outputs against values in file
  dump <name> (request|acknowledge) <file> - dump channel output to file

== Debug ==
  status 0|1|X [[^]str] - list all nodes with specified value, optional prefix/string match
  pending - dump pending events
  set_alias <n> - make <n> the primary name in the alias listing for node <n>
  alias <n> - list aliases for <n>
  fanin <n> - list fanin for <n>
  fanin-get <n> - list fanin with values for <n>
  fanout <n> - list fanout for <n>

== Tracing ==
  dumptc <file> - dump transition counts for nodes to <file>
  cleartc - clear transition counts for nodes
  pairtc - turns on <input/output> pair transition counts
  trace <file> <time> - Create atrace file for <time> duration
  timescale <t> - set time scale to <t> picoseconds for tracing
*/
void production_rule_set::parse_command(const char *line)
{
	char vname[256];
	if (strncmp(line, "array", 5) == 0)
	{
		char dup[1024];
		strncpy(dup, line, 1024);
		char *token = strtok(dup, " ");
		char *name = strtok(NULL, " ");
		char *range = strtok(NULL, " ");
		char *cmd = strtok(NULL, "");

		int start = -1;
		int end = -1;
		int step = -1;

		start = atoi(range);
		char *cptr;
		for (cptr = range; *cptr != '\0'; cptr++)
		{
			if (*cptr == ':')
			{
				if (end < 0)
					end = atoi(cptr+1);
				else if (step < 0)
					step = atoi(cptr+1);
			}
		}

		if (end < 0)
			end = start+1;
		if (step < 0)
			step = 1;

		char new_cmd[1024];
		for (int i = start; i < end; i += step)
		{
			copy_replace(new_cmd, cmd, name, i);
			parse_command(new_cmd);
		}
	}
	else if (strncmp(line, "source", 6) == 0)
	{
		char cmd[1024];
		if (sscanf(line, "source %s", cmd) == 1)
		{
			preview_script(cmd);
		}
	}
	else if (strncmp(line, "cycle", 5) == 0)
	{
		if (sscanf(line, "cycle %s", vname) == 1)
			set_read(vname);
	}
	else if (strncmp(line, "set_reset", 9) == 0)
	{
		if (sscanf(line, "set_reset %s", vname) == 1)
		{
			reset = vname;
			set_read(reset);
		}
	}
	else if (strncmp(line, "set", 3) == 0)
	{
		if (sscanf(line, "set %s", vname) == 1)
			set_scripted(to_string(vname));
	}
	else if (strncmp(line, "get", 3) == 0)
	{
		if (sscanf(line, "get %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "assert", 6) == 0)
	{
		if (sscanf(line, "assert %s", vname) == 1)
			set_asserted(to_string(vname));
	}
	else if (strncmp(line, "seu", 3) == 0)
	{
		if (sscanf(line, "seu %s", vname) == 1)
			set_scripted(to_string(vname));
	}
	else if (strncmp(line, "uget", 4) == 0)
	{
		if (sscanf(line, "uget %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "watchall", 8) == 0)
	{
	}
	else if (strncmp(line, "watch", 5) == 0)
	{
		if (sscanf(line, "watch %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "unwatch", 7) == 0)
	{
		if (sscanf(line, "unwatch %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "bundle", 6) == 0)
	{
		if (sscanf(line, "bundle %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "vector", 6) == 0)
	{
		if (sscanf(line, "vector %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "channel", 7) == 0)
	{
		char vproto[256] = "";
		char vreq[256] = "";
		char vack[256] = "";
		set_read(reset);
		if (sscanf(line, "channel %s %s %s %s", vname, vproto, vreq, vack) >= 2)
		{
			channels.push_back(channel(vname, vproto, vreq, vack));
			set_read(channels.back().req);
			set_read(channels.back().ack);
		}
	}
	else if (strncmp(line, "clocked_bus", 11) == 0)
	{
		char vclk[256];
		if (sscanf(line, "clocked_bus %s %s", vname, vclk) == 2)
		{
			buses.push_back(bus(vname, vclk));
			set_read(buses.back().name);
			set_read(buses.back().clk);
		}
	}
	else if (strncmp(line, "clock_source", 12) == 0)
	{
		if (sscanf(line, "clock_source %s", vname) == 1)
			set_scripted(vname);
	}
	else if (strncmp(line, "inject", 6) == 0)
	{
		char vtype[256];
		if (sscanf(line, "inject %s %s", vname, vtype) == 2)
		{
			int idx = -1;
			for (int i = 0; i < (int)channels.size() && idx < 0; i++)
				if (channels[i].name == to_string(vname))
					idx = i;

			if (idx >= 0)
			{
				if (strncmp(vtype, "request", 7) == 0)
					set_scripted(channels[idx].req);
				else
					set_scripted(channels[idx].ack);
			}
			else
			{
				for (int i = 0; i < (int)buses.size() && idx < 0; i++)
					if (buses[i].name == to_string(vname))
						idx = i;
				
				if (idx >= 0)
					set_scripted(buses[idx].name);
			}
		}
	}
	else if (strncmp(line, "set_alias", 9) == 0)
	{
		if (sscanf(line, "set_alias %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "alias", 5) == 0)
	{
		if (sscanf(line, "alias %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "fanin", 5) == 0)
	{
		if (sscanf(line, "fanin %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "fanin-get", 9) == 0)
	{
		if (sscanf(line, "fanin-get %s", vname) == 1)
			set_read(to_string(vname));
	}
	else if (strncmp(line, "fanout", 6) == 0)
	{
		if (sscanf(line, "fanout %s", vname) == 1)
			set_read(to_string(vname));
	}
}

void production_rule_set::preview_script(string filename)
{
	FILE *fscr = fopen(filename.c_str(), "r");
	while (!feof(fscr))
	{
		string line = getline(fscr);
		parse_command(line.c_str());	
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

