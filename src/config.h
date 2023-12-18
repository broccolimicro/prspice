#pragma once

#include "common.h"

struct config
{
	config();
	~config();

	double Vdd;
	double Vtn;
	double Vtp;
	string mangle_chars;
	string mangle_letter;

	string get_path(string tech);
	string resolve_include(string include);
	void load(string tech);

	string mangle_process(string name);	
	string mangle_name(string name);
	string demangle_name(string name);
};

