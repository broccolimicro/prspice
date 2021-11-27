#pragma once

#include "common.h"
#include "hash.h"
#include "config.h"

struct pr_variable
{
	pr_variable();
	~pr_variable();

	hashtable<string, 100> names;

	string name;
	int depth;
	bool filtered;

	bool written;
	bool read;
	bool scripted;
	bool asserted;
	bool aliased;

	bool is(string name);
	void combine(pr_variable v);
	void add_name(string name, bool is_filtered);
	void add_names(vector<string> name, vector<bool> is_filtered);
};

struct channel
{
	channel() {}
	channel(string name, string protocol, string req, string ack)
	{
		this->name = name;
		this->protocol = protocol;
		if (req.size() > 0)
			this->req = name + "." + req;
		else
			this->req = name + ".d";

		if (ack.size() > 0)
			this->ack = name + "." + ack;
		else
			this->ack = name + "." + this->protocol[0];
	}
	~channel() {}

	string name;
	string protocol;
	string req;
	string ack;
};

struct bus
{
	bus() {}
	bus(string name, string wires, string clk)
	{
		this->name = name;
		this->wires = wires;
		this->clk = clk;
	}
	~bus() {}

	string name;
	string wires;
	string clk;
};


typedef list<pr_variable>::iterator pr_index;

struct production_rule_set
{
	production_rule_set();
	production_rule_set(string prsfile, string scriptfile, config conf);
	~production_rule_set();

	vector<string> filter;
	list<pr_variable> variables;
	hashmap<string, pr_index, 10000> variable_map;

	//bool init;
	string init;
	string script;
	string reset;

	vector<channel> channels;
	vector<bus> buses;

	bool filter_name(string name);
	vector<bool> filter_names(vector<string> names);

	pr_index indexof(string name);
	pr_index set(string name);

	vector<pr_index> set_all_recurse(char *pre, int pre_length, char *post, int post_length);
	vector<pr_index> set_all(string name);
	
	vector<pr_index> set_written(string name);
	bool is_written(string name);

	vector<pr_index> set_read(string name);
	bool is_read(string name);

	vector<pr_index> set_scripted(string name);
	bool is_scripted(string name);

	vector<pr_index> set_asserted(string name);
	bool is_asserted(string name);

	vector<pr_index> set_aliased(string name);
	bool is_aliased(string name);

	void add_pr(string filename);
	void load_prs(string filename);
	void load_script(string filename, config conf);
	void parse_command(const char *line);
	void preview_script(string filename);
	void write_dbase(string filename);
	void load_dbase(string filename);
	pr_index new_var();
	void delete_var(pr_index index);
	void replace_var(pr_index from, pr_index to);

	bool has_prs();
};


