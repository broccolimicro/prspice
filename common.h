#pragma once

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

string exec(string cmd, bool debug=true);
string act_to_spice(string proc);
vector<string> split(string str, string delim);
string join(vector<string> str, string delim);
string tolower(string str);
bool file_exists(string name);
string find_config(string config);
string mangle_name(string name, string mangle);
string demangle_name(string name, string mangle);
string trim(string name, string discard);

