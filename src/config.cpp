#include "config.h"
#include "common.h"

config::config()
{
	Vdd = 1.0;
	Vtn = 0.2;
	Vtp = 0.8;
	mangle_chars = "";
	mangle_letter = "_";
}

config::~config()
{
}

string config::get_path(string tech)
{
	string config_path = "/conf/" + tech + "/";
	char *cad = getenv("ACT_HOME");
	if (cad != NULL) {
		config_path = string(cad) + config_path;
	} else {
		printf("Please set the ACT_HOME environment variable.\n");
	}

	return config_path;
}

string config::resolve_include(string include)
{
	string target = include;
	char *cad = getenv("ACT_HOME");
	if (cad != NULL) {
		target = replace(include, "${ACT_HOME}", string(cad));
	} else {
		printf("Please set the ACT_HOME environment variable.\n");
	}

	if (!file_exists(target)) {
		printf("Process technology config file not found\n");
		exit(1);
	}

	return target;
}

void config::load(string tech)
{
	vector<string> stack;
	stack.push_back(get_path(tech) + "actsim.conf");
	stack.push_back(get_path(tech) + "lint.conf");
	stack.push_back(get_path(tech) + "global.conf");

	while ((int)stack.size() > 0) {
		FILE *fptr = fopen(stack.back().c_str(), "r");
		if (fptr == NULL) {
			printf("error: unable to open process technology config file\n");
		}
		stack.pop_back();

		while (!feof(fptr)) {
			vector<string> line = split(trim(getline(fptr), " \n\r\t"), " ");

			if (line.size() == 2) {
				if (line[0] == "include") {
					stack.push_back(resolve_include(trim(line[1], "\"\'")));
				}
			} else if (line.size() >= 3) {
				if (line[1] == "mangle_chars") {
					mangle_chars = line[2].substr(1, line[2].size()-2);
				} else if (line[1] == "mangle_letter") {
					mangle_letter = line[2].substr(1, line[2].size()-2);
				} else if (line[0] == "real" and line[1] == "Vdd") {
					Vdd = atof(line[2].c_str());
				} else if (line[0] == "real" and line[1] == "V_high") {
					Vtp = atof(line[2].c_str());
				} else if (line[0] == "real" and line[1] == "V_low") {
					Vtn = atof(line[2].c_str());
				}
			}
		}

		fclose(fptr);
	}
}

string config::mangle_process(string name)
{
	int start = (int)name.find_first_of("<");
	int end = (int)name.find_last_of(">");
	
	if (start == -1 || end == -1 || end == start+1) {
		return name.substr(0, start);
	}

	return mangle_name(name);
}

string config::mangle_name(string name)
{
	std::ostringstream result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == mangle_letter[0])
			result << mangle_letter << mangle_letter;
		else
		{
			int loc = mangle_chars.find(name[i]);
			if (loc == -1)	
				result << name[i];
			else if (loc <= 9)
				result << mangle_letter << loc;
			else if (loc > 9)
				result << mangle_letter << (char)('a' + loc-10);
		}
	}
	return result.str();
}

string config::demangle_name(string name)
{
	std::ostringstream result;
	for (int i = 0; i < (int)name.size(); i++)
	{
		if (name[i] == mangle_letter[0] && ++i < (int)name.size())
		{
			if (name[i] == mangle_letter[0])
				result << mangle_letter;
			else
			{
				int loc = name[i] - '0';
				if (loc > 9)
					loc = (name[i] - 'a') + 10;
				result << mangle_chars[loc];
			}
		}
		else
			result << name[i];
	}
	return result.str();
}


