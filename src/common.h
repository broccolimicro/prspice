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

template <typename type>
bool vector_intersects(const vector<type> &v1, const vector<type> &v2)
{
	typename vector<type>::const_iterator i, j;
	for (i = v1.begin(), j = v2.begin(); i != v1.end() && j != v2.end();)
	{
		if (*j > *i)
			i++;
		else if (*i > *j)
			j++;
		else
			return true;
	}

	return false;
}

string exec(string cmd, bool debug=true);
vector<string> split(string str, string delim);
string join(vector<string> str, string delim);
string tolower(string str);
bool file_exists(string name);
string trim(string name, string discard);
string getline(FILE *fptr);
void copy_replace(char *target, const char *source, const char *search, int replace);

