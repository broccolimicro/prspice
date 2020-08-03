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
		if (j-i-1 > 0)
			result.push_back(str.substr(i+1, j-i-1));

		i = j;
		j = str.find_first_of(delim, i+1);
	}

	if (i+1 < str.size())
		result.push_back(str.substr(i+1));

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
		char *cad = getenv("ACT_HOME");
		if (cad != NULL)
		{
			config_path = string(cad) + "/conf/" + config_path + "/global";

			if (!file_exists(config_path) && config_path.find(".conf") == -1)
				config_path += ".conf";
		}
		else
			printf("Please set the ACT_HOME environment variable.\n");
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
	std::ostringstream result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == '_')
			result << "__";
		else
		{
			int loc = mangle.find(name[i]);
			if (loc == -1)	
				result << name[i];
			else if (loc <= 9)
				result << "_" << loc;
			else if (loc > 9)
				result << "_" << (char)('a' + loc-10);
		}
	}
	return result.str();
}

string demangle_name(string name, string mangle)
{
	std::ostringstream result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == '_' && ++i < (int)name.size())
		{
			if (name[i] == '_')
				result << "_";
			else
			{
				int loc = name[i] - '0';
				if (loc > 9)
					loc = (name[i] - 'a') + 10;
				result << mangle[loc];
			}
		}
		else
			result << name[i];
	}
	return result.str();
}

string trim(string name, string discard)
{
	int left = 0, right = (int)name.size()-1;
	for (left = 0; left < (int)name.size() && discard.find(name[left]) != -1; left++);
	for (right = (int)name.size()-1; right >= 0 && discard.find(name[right]) != -1; right--);
	return name.substr(left, right-left+1);
}

string getline(FILE *fptr)
{
	string result;
	char buffer[1024];
	bool done = false;
	while ((result.size() == 0 || (result[result.size()-1] != '\r' && result[result.size()-1] != '\n')) && fgets(buffer, 1023, fptr) != NULL)
		result += string(buffer);
	return result;
}

void copy_replace(char *target, const char *source, const char *search, int replace)
{
	int slen = strlen(search);
	const char *s = source;
	const char *p = NULL;
	char *t = target;

	while (1) {
		p = strstr(s, search);
		if (p == NULL)
		{
			strcpy(t, s);
			return;
		} else {
			memcpy(t, s, p-s);
			t += p-s;
			t += sprintf(t, "%d", replace);
			s = p + slen;
			p = NULL;
		}
	}
}

