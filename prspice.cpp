#include "common.h"
#include "dbase.h"

int main(int argc, char **argv)
{
	bool debug = true;

	string process = "";
	string instance = "dut";
	string config = "";
	string test_file = "";
	string script_file = "";
	bool pack = false;

	for (int i = 1; i < argc; i++)
	{
		if (string(argv[i]) == "-C" && ++i < argc)
			config = argv[i];
		else if (string(argv[i]) == "-p" && ++i < argc)
			process = argv[i];
		else if (string(argv[i]) == "-i" && ++i < argc)
			instance = argv[i];
		else if (string(argv[i]) == "-pack")
			pack = true;
		else if (test_file == "" && string(argv[i]).find(".act") != -1)
			test_file = argv[i];
		else if (script_file == "")
			script_file = argv[i];
		else
		{
			printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"netgen config\" \"act_file\" \"script_file\"\n");
			exit(1);
		}
	}

	if (process == "" || config == "" || test_file == "")
	{
		printf("usage: prspice -p \"process_name\" -i \"instance_name\" -C \"netgen config\" \"act_file\" \"script_file\"\n");
		exit(1);
	}

	string dir = test_file.substr(0, test_file.find_last_of("."));

	// Create the run directory
	exec("mkdir " + dir);
	if (to_string(getenv("PRSPICE_INSTALL")) == "")
	{
		printf("please define the PRSPICE_INSTALL path\n");
		exit(1);
	}
	exec("cp " + to_string(getenv("PRSPICE_INSTALL")) + "/* " + dir);

	string config_path = find_config(config);
	
	// get the mangle string
	string mangle = exec("grep mangle_chars \"" + config_path + "\" | grep -o \"\\\".*\\\"\"", debug);
	mangle = mangle.substr(1, mangle.find_last_of("\"")-1);

	// Generate the production rules for the environment
	string flat = "cflat -DLAYOUT=false " + test_file + " | grep -v \"" + instance + "\"";
	if (pack)
		flat += " | prspack " + dir + "/names";
	flat += " > " + dir + "/env.prs";
	exec(flat, debug);

	// Get the spice netlist for the device under test
	exec("netgen -p \"" + process + "\" -C " + config + " " + test_file + " > " + dir + "/dut.spi", debug);
	
	// Generate a wrapper spice subcircuit to connect up power and ground
	string spice_process = act_to_spice(process);

	vector<string> subckt = split(exec("grep \".subckt " + spice_process + "\" " + dir + "/dut.spi", debug), " \t\n\r");
	vector<string> wrapper_subckt;

	for (int i = 2; i < (int)subckt.size(); i++)
		if (tolower(subckt[i]).find("vdd") == -1 && tolower(subckt[i]).find("gnd") == -1)
			wrapper_subckt.push_back(subckt[i]);
	
	string wrapper = "\n.subckt wrapper " + join(wrapper_subckt, " ") + "\n";
	wrapper += "x" + instance;
	for (int i = 2; i < (int)subckt.size(); i++)
	{
		if (tolower(subckt[i]).find("vdd") != -1)
			wrapper += " vdd";
		else if (tolower(subckt[i]).find("gnd") != -1)
			wrapper += " gnd";
		else
			wrapper += " " + subckt[i];
	}
	wrapper += " " + spice_process + "\n.ends\n";

	// add it to the spice netlist for the device under test
	FILE *fdut = fopen((dir + "/dut.spi").c_str(), "a");
	if (fdut == NULL)
	{
		printf("unable to open dut file\n");
		exit(1);
	}

	fprintf(fdut, "%s", wrapper.c_str());

	fclose(fdut);

	// Generate the verilog glue
	production_rule_set prset(dir + "/env.prs", script_file, mangle);
	vector<string> demangled_wrapper_subckt;
	for (int i = 0; i < (int)wrapper_subckt.size(); i++)
		demangled_wrapper_subckt.push_back(demangle_name(wrapper_subckt[i], mangle));

	string verilog = "";
	verilog += "`timescale 1ns / 1ps;\n\n";

	// First generate the hsim wrapper
	verilog += "module wrapper(" + join(wrapper_subckt, ", ") + ");\n";
	for (int i = 0; i < (int)wrapper_subckt.size(); i++)
	{
		string direction = "output";
		if (tolower(wrapper_subckt[i]).find("reset") != -1 || prset.is_written(demangled_wrapper_subckt[i]))
			direction = "input";

		verilog += "\t" + direction + " " + wrapper_subckt[i] + ";\n";
	}
	verilog += "\tinitial $nsda_module();\n";
	verilog += "endmodule\n\n";

	// Now the prsim environment
	verilog += "module top;\n";
	// define all of the signals
	for (int i = 0; i < (int)prset.variables.size(); i++)
	{
		// If the signal is driven in the prsim script, then its a register. Otherwise, its a wire
		string mname = mangle_name(prset.variables[i].name(), mangle);
		if ((prset.variables[i].written && !prset.variables[i].read) || 
			(prset.variables[i].read && !prset.variables[i].written && !prset.variables[i].scripted))
			verilog += "\twire " + mname + ";\n";
		else if (prset.variables[i].scripted)
			verilog += "\treg " + mname + ";\n";
		
	}
	verilog += "\n";

	// connect the prsim environment
	verilog += "\tinitial begin\n";
	// hack to prevent initial dc-initialization from messing with prsim
	verilog += "\t\t#10 $prsim(\"env.prs\");\n";
	for (int i = 0; i < (int)prset.variables.size(); i++)
	{
		string name = prset.variables[i].name();
		string mname = mangle_name(name, mangle);

		// If the signal is driven by the production rules then it comes from prsim. Otherwise it goes to prsim
		if (prset.variables[i].written && find(subckt.begin(), subckt.end(), mname) != subckt.end())
			verilog += "\t\t$from_prsim(\"" + name + "\", \"" + mname + "\");\n";
		else if ((prset.variables[i].read || prset.variables[i].aliased) && !prset.variables[i].written)
			verilog += "\t\t$to_prsim(\"" + mname + "\", \"" + name + "\");\n";
	}
	verilog += "\n";

	// copy in the loaded script
	verilog += prset.script;
	verilog += "\tend\n";
	verilog += "\n";
	// define an instance of the wrapper
	verilog += "\twrapper dut(" + join(wrapper_subckt, ", ") + ");\n";
	verilog += "endmodule\n";
	
	FILE *fver = fopen((dir + "/test.v").c_str(), "w");
	fprintf(fver, "%s", verilog.c_str());
	fclose(fver);
}
