#pragma once

#include "common.h"
#include "hash.h"

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

typedef list<pr_variable>::iterator pr_index;

struct production_rule_set
{
	production_rule_set();
	production_rule_set(string prsfile, string scriptfile, string mangle);
	~production_rule_set();

	vector<string> filter;
	list<pr_variable> variables;
	hashmap<string, pr_index, 10000> variable_map;

	string init;
	string script;

	bool filter_name(string name);
	vector<bool> filter_names(vector<string> names);

	pr_index indexof(string name);
	pr_index set(string name);
	pr_index set_written(string name);
	bool is_written(string name);
	pr_index set_read(string name);
	bool is_read(string name);
	pr_index set_scripted(string name);
	bool is_scripted(string name);
	pr_index set_asserted(string name);
	bool is_asserted(string name);
	pr_index set_aliased(string name);
	bool is_aliased(string name);
	void add_pr(string filename);
	void load_prs(string filename);
	void load_script(string filename, string mangle);
	void preview_script(string filename);
	void write_dbase(string filename);
	void load_dbase(string filename);
	pr_index new_var();
	void delete_var(pr_index index);
	void replace_var(pr_index from, pr_index to);

	bool has_prs();
};


