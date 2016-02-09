#pragma once

#include "common.h"

struct pr_variable
{
	pr_variable();
	~pr_variable();

	vector<string> names;
	bool written;
	bool read;
	bool scripted;
	bool aliased;

	bool is(string name);
	void combine(pr_variable v);
	void add_name(string name);
	void add_names(vector<string> name);
	string name();
};

struct production_rule_set
{
	production_rule_set();
	production_rule_set(string prsfile, string scriptfile, string mangle);
	~production_rule_set();

	vector<pr_variable> variables;
	string script;

	int indexof(string name);
	int set(string name);
	void set_written(string name);
	bool is_written(string name);
	void set_read(string name);
	bool is_read(string name);
	void set_scripted(string name);
	bool is_scripted(string name);
	void set_aliased(string name);
	bool is_aliased(string name);
	void add_pr(string filename);
	void load_prs(string filename);
	void load_script(string filename, string mangle);
	void write_dbase(string filename);
	void load_dbase(string filename);
};


