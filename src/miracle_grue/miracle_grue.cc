/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/



#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdint.h>

#include "mgl/abstractable.h"
#include "mgl/configuration.h"
#include "mgl/miracle.h"

#include "libthing/Vector2.h"
#include "optionparser.h"

#include "mgl/log.h"



using namespace std;
using namespace mgl;


/// Extends options::Arg to specifiy limitations on arguments
struct Arg: public option::Arg
{
  static void printError(const char* msg1, const option::Option& opt, const char* msg2)
  {
    fprintf(stderr, "%s", msg1);
    fwrite(opt.name, opt.namelen, 1, stderr);
    fprintf(stderr, "%s", msg2);
  }

  static option::ArgStatus Unknown(const option::Option& option, bool msg)
  {
    if (msg) printError("Unknown option '", option, "'\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Required(const option::Option& option, bool msg)
  {
    if (option.arg != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires an argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
  {
    if (option.arg != 0 && option.arg[0] != 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a non-empty argument\n");
    return option::ARG_ILLEGAL;
  }

  static option::ArgStatus Numeric(const option::Option& option, bool msg)
  {
    char* endptr = 0;
    if (option.arg != 0 && strtod(option.arg, &endptr)){};
    if (endptr != option.arg && *endptr == 0)
      return option::ARG_OK;

    if (msg) printError("Option '", option, "' requires a numeric argument\n");
    return option::ARG_ILLEGAL;
  }
};

// all ID's of the options we expect
enum optionIndex {UNKNOWN, HELP, CONFIG, FIRST_Z,LAYER_H,LAYER_W, FILL_ANGLE, FILL_DENSITY,
				 N_SHELLS, BOTTOM_SLICE_IDX, TOP_SLICE_IDX, DEBUG_ME, START_GCODE,
				END_GCODE, OUT_FILENAME};
// options descriptor table
const option::Descriptor usageDescriptor[] =
{
 {UNKNOWN, 0, "", "",Arg::None, "miracle-grue [OPTIONS] FILE.STL \n\n"
                                        "Options:" },
 {HELP, 0,"", "help",Arg::None, "  --help  \tPrint usage and exit." },
{CONFIG, 1,"c", "config", Arg::NonEmpty, "-c  \tconfig data in a config.json file."
		 "(default is local miracle.config)" },
{FIRST_Z, 2,"f", "firstLayerZ", Arg::Numeric,
		"-f \tfirst layer height (mm)" },
{LAYER_H, 3,"h", "layerH", Arg::Numeric,
		"  -h \tgeneral layer height(mm)" },
{LAYER_W, 4,"w", "layerW", Arg::Numeric,
		"  -w \tlayer width(mm)" },
{ FILL_ANGLE, 5, "a","angle", Arg::Numeric,
		"  -a \tinfill grid inter slice angle(radians)" },
{ FILL_DENSITY, 6, "p", "density", Arg::Numeric,
		"  -p \tapprox infill density(percent), aka rho aka p" },
{ N_SHELLS, 	7, "n", "nShells", Arg::Numeric,
		"  -n \tnumber of shells per layer" },
{ BOTTOM_SLICE_IDX, 8, "b", "bottomIdx", Arg::Numeric,
		"  -b \tbottom slice index" },
{ TOP_SLICE_IDX, 	9, "t", "topIdx", Arg::Numeric,
		"  -t \ttop slice index" },
{ DEBUG_ME, 	10, "d", "debug", Arg::Numeric,
		"  -d \tdebug level, 0 to 99. 60 is 'info'" },
{ START_GCODE, 	11, "s", "header", Arg::NonEmpty,
		"  -s \tstart gcode file" },
{ END_GCODE, 	12, "e", "footer", Arg::NonEmpty,
		"  -e \tend gcode file" },
{ OUT_FILENAME, 	13, "o", "outFilename", Arg::NonEmpty,
		"  -o \twrite gcode to specific filename (defaults to <model>.gcode" },
{0,0,0,0,0,0},
};

void usage() {


	cout << endl;
	cout << "It is pitch black. You are likely to be eaten by a grue." << endl;
	cout << "You are using " << GRUE_PROGRAM_NAME << " version " << GRUE_VERSION  << endl;
	cout << endl;
	cout << "This program translates a 3d model file in STL format to GCODE toolpath for a " << endl;
	cout << "3D printer." << " Another fine MakerBot Industries product!"<< endl;
	cout << endl;
    option::printUsage(std::cout, usageDescriptor);
	Log::severe() <<" Log level::severe ";
	Log::info()<<"::info";
	Log::fine() <<"::fine";
	Log::finer() <<"::finer";
	Log::finest() <<"::finest";
	cout << endl;
}


int newParseArgs( Configuration &config,
		int argc, char *argv[],
		string &modelFile,
		int &firstSliceIdx,
		int &lastSliceIdx) {

	string configFilename = "";

	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats  stats(usageDescriptor, argc, argv);
	option::Option* options = new option::Option[stats.options_max];
	option::Option* buffer  = new option::Option[stats.buffer_max];
	option::Parser parse(usageDescriptor, argc, argv, options, buffer);


	if (parse.error())
		return -20;

	if (options[HELP] || argc == 0) {
		usage();
		exit(0);
	}

	///read config file and/or help option first
	if ( options[CONFIG]) {
		configFilename = string(options[CONFIG].arg);
	}

	// fallback to default config
    if (configFilename.compare(string("")) == 0)
		config.readFromDefault();
	else
		config.readFromFile(configFilename);

	for (int i = 0; i < parse.optionsCount(); ++i)
	{
		option::Option& opt = buffer[i];
		fprintf(stdout, "Argument #%d name %s is #%s\n", i, opt.desc->longopt, opt.arg );
		switch (opt.index())
		{
			case  LAYER_H:
			case  LAYER_W:
			case  FILL_ANGLE:
			case  FILL_DENSITY:
			case  N_SHELLS:
			case  BOTTOM_SLICE_IDX:
			case  TOP_SLICE_IDX:
			case  FIRST_Z:
				config["slicer"][opt.desc->longopt] = atof(opt.arg);
				break;
			case  DEBUG_ME:
				config["meta"][opt.desc->longopt] = atof(opt.arg);
				break;
			case  START_GCODE:
			case  END_GCODE:
			case  OUT_FILENAME:
				config["gcoder"][opt.desc->longopt] = opt.arg;
				break;
			case CONFIG:
				// handled above before other config values
				break;
			case HELP:
			// not possible, because handled further above and exits the program
			default:
				break;
		}
	}

	/// handle parameters (not options!)
	if ( parse.nonOptionsCount() == 0) {
		usage();
	}
	else if ( parse.nonOptionsCount() != 1) {
		Log::severe() << "too many parameters" << endl;
		for (int i = 0; i < parse.nonOptionsCount(); ++i)
			Log::severe() << "Parameter #" << i << ": " << parse.nonOption(i) << "\n";
			exit(-10);
	}
	else {
		//handle the unnamed parameter separately
		modelFile = parse.nonOption(0);
		Log::finer() << "filename " << modelFile << endl;
		ifstream testmodel(modelFile.c_str(), ifstream::in);
		if (testmodel.fail()) {
			usage();
			throw mgl::Exception(("Invalid model file [" + modelFile + "]").c_str());
			exit(-10);
		}
	}

	firstSliceIdx = -1;
	lastSliceIdx = -1;

	// [programName] and [versionStr] are always hard-code overwritten
	config["programName"] = GRUE_PROGRAM_NAME;
	config["versionStr"] = GRUE_VERSION;
	config["firmware"] = "unknown";

    if(false == config.isMember("machineName")) {
        config["machineName"] = "Machine Name Unknown";
    }

	/// convert debug data to a module/level specific setting
	g_debugVerbosity = log_verbosity_unset;
	if ( config["meta"].isMember("debug") ) {
		try {
			uint32_t debugLvl = config["meta"]["debug"].asUInt();
			if ( debugLvl < 90 ) g_debugVerbosity = log_finest;
			else if ( debugLvl < 80 ) g_debugVerbosity = log_finer;
			else if ( debugLvl < 70 ) g_debugVerbosity = log_fine;
			else if ( debugLvl < 60 ) g_debugVerbosity = log_info;
			else if ( debugLvl < 10 ) g_debugVerbosity = log_severe;
			else g_debugVerbosity = log_verbosity_unset;
		}
		catch (...){
			cout << "fail sauce on debug level" << endl;
			// passed -d sans option. Assume default dbg level
			//g_debugVerbosity = log_default_level;
		}
	}

	return 0;
}




int main(int argc, char *argv[], char *[]) // envp
{

	string modelFile;

    Configuration config;
    try
    {
		int firstSliceIdx, lastSliceIdx;

		int ret = newParseArgs(config, argc, argv, modelFile, firstSliceIdx, lastSliceIdx);

		if(ret != 0){
			usage();
			exit(ret);
		}

		// cout << config.asJson() << endl;

		MyComputer computer;
		Log::fine() << endl << endl;
		Log::fine() << "behold!" << endl;
		Log::fine() << "Materialization of \"" << modelFile << "\" has begun at " << computer.clock.now() << endl;

		std::string scadFile = "."; // outDir
		scadFile += computer.fileSystem.getPathSeparatorCharacter();
		scadFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile.c_str()).c_str(), ".scad" );

		
		std::string gcodeFile = config["gcoder"]["outFilename"].asString();

		if (gcodeFile.empty()) {
			gcodeFile = ".";
			gcodeFile += computer.fileSystem.getPathSeparatorCharacter();
			gcodeFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile.c_str()).c_str(), ".gcode" );
		}

		Log::fine() << endl << endl;
		Log::fine() << modelFile << " to \"" << gcodeFile << "\" and \"" << scadFile << "\"" << endl;

		GCoderConfig gcoderCfg;
		loadGCoderConfigFromFile(config, gcoderCfg);

		SlicerConfig slicerCfg;
		loadSlicerConfigFromFile(config, slicerCfg);

		const char* scad = NULL;

		if (scadFile.size() > 0 )
			scad = scadFile.c_str();

		Tomograph tomograph;
		Regions regions;
		std::vector<mgl::SliceData> slices;

		ProgressLog log;

		miracleGrue(gcoderCfg, slicerCfg, modelFile.c_str(),
					scad,
					gcodeFile.c_str(),
					firstSliceIdx,
					lastSliceIdx,
					tomograph,
					regions,
					slices,
					&log);

    }
    catch(mgl::Exception &mixup)
    {
    	Log::severe() << "ERROR: "<< mixup.error << endl;
    	return -1;
    }

	exit(EXIT_SUCCESS);
}
