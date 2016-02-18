#include "common.h"

string exec(string cmd, bool debug)
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
		else if (proc[i] == ':' && i+1 < (int)proc.size() && proc[i+1] == ':')
		{
			if (i != 0)
				result += "_";
			i++;
		}
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
        config_path = string(getenv("CAD_HOME")) + "/lib/netgen/" + config_path;

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


