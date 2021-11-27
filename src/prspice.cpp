#include "common.h"
#include "dbase.h"
#include "config.h"

vector<pair<string, string> > parse_sources(string sources)
{
	vector<pair<string, string> > result;
	vector<string> drivers = split(sources, ";");
	for (int i = 0; i < (int)drivers.size(); i++)
	{
		vector<string> eq = split(drivers[i], "=");
		result.push_back(pair<string, string>(trim(eq[0], " \t\n\r"), trim(eq[1], " \t\n\r")));
	}
	return result;
}

int main(int argc, char **argv)
{
	bool debug = true;

	string process = "";
	string instance = "dut";
	string tech = "";
	string test_file = "";
	string script_file = "";
	string dir = "";
	bool pack = false;
	string prs2net_flags = "";
	string sources = "g.Vdd=1.0;g.GND=0.0";

	for (int i = 1; i < argc; i++)
	{
		if (string(argv[i]) == "-C" && ++i < argc)
			tech = argv[i];
		else if (string(argv[i]) == "-p" && ++i < argc)
			process = argv[i];
		else if (string(argv[i]) == "-i" && ++i < argc)
			instance = argv[i];
		else if (string(argv[i]) == "-o" && ++i < argc)
			dir = argv[i];
		else if (string(argv[i]) == "-s" && ++i < argc)
			sources = argv[i];
		else if (string(argv[i]) == "-scale" && ++i < argc)
			prs2net_flags += "-s " + string(argv[i]);
		else if (string(argv[i]) == "-pack")
			pack = true;
		else if (string(argv[i]) == "-B")
			prs2net_flags += "-B";
		else if (test_file == "" && string(argv[i]).find(".act") != -1)
			test_file = argv[i];
		else if (script_file == "")
			script_file = argv[i];
		else
		{
			printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"prs2net config\" \"act_file\" \"script_file\"\n");
			exit(1);
		}
	}

	if (process == "" || tech == "" || test_file == "")
	{
		printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"prs2net config\" \"act_file\" \"script_file\"\n");
		exit(1);
	}

	if (dir == "")
		dir = test_file.substr(0, test_file.find_last_of("."));

	// Create the run directory
	exec("mkdir " + dir);

	config conf;
	conf.load(tech);	

	// Generate the production rules for the environment
	string flat = "aflat -DDUT=false -DLAYOUT=false -DPRSIM=false " + test_file + " | prdbase \"" + script_file + "\" \"" + dir + "/dbase.dat\" \"" + instance + "\"";
	if (pack)
		flat += " | prspack " + dir + "/names";
	flat += " > " + dir + "/env.prs";
	exec(flat, debug);

	// Get the spice netlist for the device under test
	string spice_str = "prs2net -T" + tech + " -DDUT=true -DLAYOUT=true -DPRSIM=false " + prs2net_flags + " -p \"" + process + "\" " + test_file + " > " + dir + "/dut.spi";
	exec(spice_str, debug);
	
	// Generate a wrapper spice subcircuit to connect up power and ground
	string spice_process = act_to_spice(process);

	vector<string> subckt = split(exec("grep \".subckt " + spice_process + "\" " + dir + "/dut.spi", debug), " \t\n\r");

	vector<pair<string, string> > drivers = parse_sources(sources);
	for (int i = 0; i < (int)drivers.size(); i++) {
		drivers[i].first = conf.mangle_name(drivers[i].first);
	}

	// Load dbase
	production_rule_set prset;
	prset.load_dbase(dir + "/dbase.dat");
	prset.load_script(script_file, conf);
	vector<pr_index> vlist;

	for (int i = 2; i < (int)subckt.size(); i++)
	{
		string name = instance + "." + conf.demangle_name(subckt[i]);
		pr_index index = prset.indexof(name);	

		if (index == prset.variables.end())
			vlist.push_back(prset.set(name));
		else
			vlist.push_back(index);
	}

	// Generate spice test file and prsim script file

	// Print header for spice test file
	FILE *ftest = fopen((dir + "/test.spi").c_str(), "a");
	if (ftest == NULL) {
		printf("unable to open test file\n");
		exit(1);
	}

	// instantiate local power sources	
	for (int i = 0; i < (int)drivers.size(); i++) {
		fprintf(ftest, "c%s %s 0 1.0\n", drivers[i].first.c_str(), drivers[i].first.c_str());
	}
	fprintf(ftest, "\n");
	for (int i = 0; i < (int)drivers.size(); i++)
		fprintf(ftest, "vpwr%d %s 0 dc %s\n", i, drivers[i].first.c_str(), drivers[i].second.c_str());
	fprintf(ftest, "\n");
	fprintf(ftest, ".inc %s/lib/spice/%s.spi\n", getenv("ACT_HOME"), tech.c_str());
	fprintf(ftest, ".inc dut.spi\n\n");
	
	// Print header for prsim script file
	FILE *fsim = fopen((dir + "/prsim.rc").c_str(), "w");
	if (fsim == NULL) {
		printf("unable to open prsim script file\n");
		exit(1);
	}
	fprintf(fsim, "netlist test.spi\n");

	// Connect signals
	for (pr_index i = prset.variables.begin(); i != prset.variables.end(); i++)
	{
		bool found = false;
		for (int j = 0; j < (int)drivers.size() and not found; j++) {
			found = i->is(conf.demangle_name(drivers[j].first));
		}
		if (not found) {
			string name = i->name;
			string mname = conf.mangle_name(name);

			// If the signal is driven by the production rules then it comes from prsim. Otherwise it goes to prsim
			if ((i->written || i->scripted) && find(vlist.begin(), vlist.end(), i) != vlist.end())
			{
				fprintf(fsim, "dac %s ydac!%s %g 143e-12 143e-12\n", name.c_str(), mname.c_str(), conf.Vdd);
				fprintf(ftest, "ydac %s %s 0 prsimDAC\n", mname.c_str(), mname.c_str());
			}
			else if (i->read && !i->written && !i->scripted)
			{
				fprintf(fsim, "adc yadc!%s %s %g %g\n", mname.c_str(), name.c_str(), conf.Vtp, conf.Vtn);
				fprintf(ftest, "yadc %s %s 0 prsimADC\n", mname.c_str(), mname.c_str());
			}
		}
	}

	
	// Print footer for prsim script file
	FILE *fscript = fopen(script_file.c_str(), "r");
	if (fscript == NULL) {
		printf("unable to read input prsim script\n");
		exit(1);
	}

	int buffer_len = 1024;
	char buffer[buffer_len];
	while (!feof(fscript)) {
		size_t n = fread(buffer, sizeof(char), buffer_len, fscript);
		fwrite(buffer, sizeof(char), n, fsim);
	}
	fclose(fscript);
	
	fclose(fsim);
	fsim = NULL;

	// Print footer for spice test file
	fprintf(ftest, "\n.model prsimADC ADC(settlingtime=143ps uppervoltagelimit=%g lowervoltagelimit=0)\n", conf.Vdd);
	fprintf(ftest, ".model prsimDAC DAC(tr=143e-12 tf=143e-12)\n\n");

	// Instantiate DUT
	fprintf(ftest, "x%s", instance.c_str());
	for (int i = 0; i < (int)vlist.size(); i++) {
		string mname = conf.mangle_name(vlist[i]->name);
		fprintf(ftest, " %s", mname.c_str());
	}
	fprintf(ftest, " %s\n\n", spice_process.c_str());

	fprintf(ftest, ".print tran format=gnuplot v(*)");
	for (int i = 0; i < (int)drivers.size(); i++)
		fprintf(ftest, " i(vpwr%d)", i);
	fprintf(ftest, "\n\n");
	fprintf(ftest, ".tran 0 20e-4\n\n");
	fprintf(ftest, ".end\n");

	fclose(ftest);
	ftest = NULL;
}
