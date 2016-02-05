#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

template<class type>
string to_string(type n)
{
	std::ostringstream stm;
	stm << n;
	return stm.str();
}

string exec(string cmd, bool debug=true)
{
	if (debug)
		cout << cmd << endl;

	FILE *pipe = popen(cmd.c_str(), "r");
	if (pipe == NULL)
		return "ERROR";

	char buffer[128];
	string result = "";
	while (!feof(pipe))
	{
		if (fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}

	pclose(pipe);
	return result;
}


string act_to_spice(string proc)
{
	string result;
	for (int i = 0; i < (int)proc.size(); i++)
	{
		if (proc[i] == '<' || proc[i] == '>' || proc[i] == ',')
			result += "_";
		else if (proc[i] != ' ')
			result += proc[i];
	}

	if (result.substr(result.size()-2, 2) == "__")
		return result.substr(0, result.size()-2);
	else
		return result;
}

vector<string> split(string str, string delim)
{
	vector<string> result;
	int i = -1;
	int j = str.find_first_of(delim);
	while (j != -1)
	{
		result.push_back(str.substr(i+1, j-i-1));
		if (result.back().size() == 0)
			result.pop_back();

		i = j;
		j = str.find_first_of(delim, i+1);
	}

	result.push_back(str.substr(i+1));
	if (result.back().size() == 0)
		result.pop_back();

	return result;
}

string join(vector<string> str, string delim)
{
	string result;
	for (int i = 0; i < (int)str.size(); i++)
	{
		if (i != 0)
			result += delim;
		result += str[i];
	}
	return result;
}

string tolower(string str)
{
	for (int i = 0; i < (int)str.size(); i++)
		str[i] = tolower((int)str[i]);
	return str;
}

bool file_exists(string name)
{
    if (FILE *file = fopen(name.c_str(), "r"))
	{
        fclose(file);
        return true;
    }
	else
        return false;
}

string find_config(string config)
{
	string config_path = config;
    if (!file_exists(config_path) && config_path[0] != '/')
    {
        config_path = string(getenv("VLSI_INSTALL")) + "/lib/netgen/" + config_path;

        if (!file_exists(config_path) && config_path.find(".conf") == -1)
            config_path += ".conf";
    }

    if (!file_exists(config_path))
    {
        printf("Netgen config file not found\n");
        exit(1);
    }

	return config_path;
}

string mangle_name(string name, string mangle)
{
	string result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == '_')
			result += "__";
		else
		{
			int loc = mangle.find(name[i]);
			if (loc == -1)	
				result += name[i];
			else
				result += "_" + to_string(loc);
		}
	}
	return result;
}

string demangle_name(string name, string mangle)
{
	string result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == '_' && ++i < (int)name.size())
		{
			if (name[i] == '_')
				result += "_";
			else
			{
				int loc = atoi(name.data() + i);
				result += mangle[loc];
			}
		}
		else
			result += name[i];
	}
	return result;
}

string trim(string name, string discard)
{
	int left = 0, right = (int)name.size()-1;
	for (left = 0; left < (int)name.size() && discard.find(name[left]) != -1; left++);
	for (right = (int)name.size()-1; right >= 0 && discard.find(name[right]) != -1; right--);
	return name.substr(left, right-left+1);
}

struct pr_variable
{
	pr_variable()
	{
		written = false;
		read = false;
		scripted = false;
	}
	
	~pr_variable() {}

	vector<string> names;
	bool written;
	bool read;
	bool scripted;
	bool aliased;

	bool is(string name)
	{
		return find(names.begin(), names.end(), name) != names.end();
	}

	void combine(pr_variable v)
	{
		names.insert(names.end(), v.names.begin(), v.names.end());
		sort(names.begin(), names.end());
		names.resize(unique(names.begin(), names.end()) - names.begin());
		written = written || v.written;
		read = read || v.read;
		scripted = scripted || v.scripted;
		aliased = aliased || v.aliased;
	}

	void add_name(string name)
	{
		names.push_back(name);
		sort(names.begin(), names.end());
        names.resize(unique(names.begin(), names.end()) - names.begin());
	}

	void add_names(vector<string> name)
	{
		names.insert(names.end(), name.begin(), name.end());
		sort(names.begin(), names.end());
		names.resize(unique(names.begin(), names.end()) - names.begin());
	}

	string name()
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
};

struct production_rule_set
{
	production_rule_set() {}
	production_rule_set(string prsfile, string scriptfile, string mangle)
	{
		load_prs(prsfile);
		load_script(scriptfile, mangle);
	}
	~production_rule_set() {}

	vector<pr_variable> variables;
	string script;

	int indexof(string name)
	{
		for (int i = 0; i < variables.size(); i++)
			if (variables[i].is(name))
				return i;
		return -1;
	}

	int set(string name)
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

	void set_written(string name)
	{
		variables[set(name)].written = true;
	}

	bool is_written(string name)
	{
		for (int j = 0; j < (int)variables.size(); j++)
			if (variables[j].is(name))
				return variables[j].written;
		return false;
	}

	void set_read(string name)
	{
		variables[set(name)].read = true;
	}

	bool is_read(string name)
	{
		for (int j = 0; j < (int)variables.size(); j++)
			if (variables[j].is(name))
				return variables[j].read;
		return false;
	}

	void set_scripted(string name)
	{
		variables[set(name)].scripted = true;
	}

	bool is_scripted(string name)
	{
		for (int j = 0; j < (int)variables.size(); j++)
			if (variables[j].is(name))
				return variables[j].scripted;
		return false;
	}

	void set_aliased(string name)
	{
		variables[set(name)].aliased = true;
	}

	bool is_aliased(string name)
	{
		for (int j = 0; j < (int)variables.size(); j++)
			if (variables[j].is(name))
				return variables[j].aliased;
		return false;
	}



	void load_prs(string filename)
	{
		// parse the environment prs file and figure out what is driven by the environment
		FILE *fenv = fopen(filename.c_str(), "r");
		char buffer[1024];
		while (!feof(fenv))
		{
			fgets(buffer, 1023, fenv);
			string line = buffer;
			
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

		fclose(fenv);
	}

	void load_script(string filename, string mangle)
	{
		FILE *fscr = fopen(filename.c_str(), "r");
		char buffer[256];
		int delay = 0;
		while (!feof(fscr))
		{
			memset(buffer, 0, 256);
			fgets(buffer, 256, fscr);
	
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
};

int main(int argc, char **argv)
{
	bool debug = true;

	string process = "";
	string instance = "dut";
	string config = "";
	string test_file = "";
	string script_file = "";

	for (int i = 1; i < argc; i++)
	{
		if (string(argv[i]) == "-C" && ++i < argc)
			config = argv[i];
		else if (string(argv[i]) == "-p" && ++i < argc)
			process = argv[i];
		else if (string(argv[i]) == "-i" && ++i < argc)
			instance = argv[i];
		else if (test_file == "" && string(argv[i]).find(".act") != -1)
			test_file = argv[i];
		else if (script_file == "")
			script_file = argv[i];
		else
		{
			printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"netgen config\" \"act_file\" \"script_file\"\n");
			exit(1);
		}
	}

	if (process == "" || config == "" || test_file == "")
	{
		printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"netgen config\" \"act_file\" \"script_file\"\n");
		exit(1);
	}

	string dir = test_file.substr(0, test_file.find_last_of("."));

	// Create the run directory
	exec("mkdir " + dir);
	if (to_string(getenv("PRSPICE_INSTALL")) == "")
	{
		printf("please define the PRSPICE_INSTALL path");
		exit(1);
	}
	exec("cp " + to_string(getenv("PRSPICE_INSTALL")) + "/* " + dir);

	string config_path = find_config(config);
	
	// get the mangle string
	string mangle = exec("grep mangle_chars \"" + config_path + "\" | grep -o \"\\\".*\\\"\"", debug);
	mangle = mangle.substr(1, mangle.find_last_of("\"")-1);

	// Generate the production rules for the environment
	exec("cflat -DLAYOUT=false " + test_file + " | grep -v \"" + instance + "\" > " + dir + "/env.prs", debug);

	// Get the spice netlist for the device under test
	exec("netgen -p \"" + process + "\" -C " + config + " " + test_file + " > " + dir + "/dut.spi", debug);
	
	// Generate a wrapper spice subcircuit to connect up power and ground
	string spice_process = act_to_spice(process);

	vector<string> subckt = split(exec("grep \".subckt " + spice_process + "\" " + dir + "/dut.spi", debug), " \t\n\r");
	vector<string> wrapper_subckt;

	for (int i = 2; i < (int)subckt.size(); i++)
		if (tolower(subckt[i]).find("vdd") == -1 && tolower(subckt[i]).find("gnd") == -1)
			wrapper_subckt.push_back(subckt[i]);
	
	string wrapper = "\n.subckt wrapper " + join(wrapper_subckt, " ") + "\n";
	wrapper += "x" + instance;
	for (int i = 2; i < (int)subckt.size(); i++)
	{
		if (tolower(subckt[i]).find("vdd") != -1)
			wrapper += " vdd";
		else if (tolower(subckt[i]).find("gnd") != -1)
			wrapper += " gnd";
		else
			wrapper += " " + subckt[i];
	}
	wrapper += " " + spice_process + "\n.ends\n";

	// add it to the spice netlist for the device under test
	FILE *fdut = fopen((dir + "/dut.spi").c_str(), "a");
	if (fdut == NULL)
	{
		printf("unable to open dut file\n");
		exit(1);
	}

	fprintf(fdut, "%s", wrapper.c_str());

	fclose(fdut);

	// Generate the verilog glue
	production_rule_set prset(dir + "/env.prs", script_file, mangle);
	vector<string> demangled_wrapper_subckt;
	for (int i = 0; i < (int)wrapper_subckt.size(); i++)
		demangled_wrapper_subckt.push_back(demangle_name(wrapper_subckt[i], mangle));

	string verilog = "";
	verilog += "`timescale 1ns / 1ps;\n\n";

	verilog += "module wrapper(" + join(wrapper_subckt, ", ") + ");\n";
	for (int i = 0; i < (int)wrapper_subckt.size(); i++)
	{
		string direction = "output";
		if (tolower(wrapper_subckt[i]).find("reset") != -1 || prset.is_written(demangled_wrapper_subckt[i]))
			direction = "input";

		verilog += "\t" + direction + " " + wrapper_subckt[i] + ";\n";
	}
	verilog += "\tinitial $nsda_module();\n";
	verilog += "endmodule\n\n";
	verilog += "module top;\n";
	for (int i = 0; i < (int)prset.variables.size(); i++)
	{
		string mname = mangle_name(prset.variables[i].name(), mangle);
		if ((prset.variables[i].written && !prset.variables[i].read) || 
			(prset.variables[i].read && !prset.variables[i].written && !prset.variables[i].scripted))
			verilog += "\twire " + mname + ";\n";
		else if (prset.variables[i].scripted)
			verilog += "\treg " + mname + ";\n";
		
	}
	verilog += "\n";

	verilog += "\tinitial begin\n";
	verilog += "\t\t#10 $prsim(\"env.prs\");\n";
	for (int i = 0; i < (int)prset.variables.size(); i++)
	{
		string name = prset.variables[i].name();
		string mname = mangle_name(name, mangle);

		if (prset.variables[i].written && find(subckt.begin(), subckt.end(), mname) != subckt.end())
			verilog += "\t\t$from_prsim(\"" + name + "\", \"" + mname + "\");\n";
		else if ((prset.variables[i].read || prset.variables[i].aliased) && !prset.variables[i].written)
			verilog += "\t\t$to_prsim(\"" + mname + "\", \"" + name + "\");\n";
	}

	verilog += "\n";
	verilog += prset.script;
	verilog += "\tend\n";
	verilog += "\n";
	verilog += "\twrapper dut(" + join(wrapper_subckt, ", ") + ");\n";
	verilog += "endmodule\n";
	
	FILE *fver = fopen((dir + "/test.v").c_str(), "w");
	fprintf(fver, "%s", verilog.c_str());
	fclose(fver);
}
