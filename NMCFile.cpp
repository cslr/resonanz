/*
 * NMCLoader.cpp
 *
 *  Created on: 15.6.2015
 *      Author: Tomas
 */

#include "NMCFile.h"
#include <stdio.h>

#include <memory>

namespace whiteice {
namespace resonanz {

NMCFile::NMCFile() {  }

NMCFile::~NMCFile() {  }


/**
 * creates program (length_secs seconds longs) 
 * which given target values
 */
bool NMCFile::createProgram(const DataSource& ds, 
			    const std::vector<float>& target,
			    unsigned int length_secs)
{
        if(target.size() != ds.getNumberOfSignals())
	  return false;
	
	if(length_secs <= 0)
	  return false;
	
	// finds first positive target values
	int first_target = -1;
	int second_target = -1;
	
	for(int i=0;i<target.size();i++){
	  if(target[i] >= 0.0f){
	    if(first_target == -1)
	      first_target = i;
	    else if(second_target == -1)
	      second_target = i;
	  }	   
	}
	
	if(first_target == -1 && second_target == -1)
	  return false;
	
	std::string names[NUMBER_OF_PROGRAMS];
	std::vector<float> pvalues[NUMBER_OF_PROGRAMS];
	
	for(unsigned int i=0;i<NUMBER_OF_PROGRAMS;i++){
	  names[i] = "N/A";
	  pvalues[i].resize(length_secs);
	  for(unsigned int s=0;s<length_secs;s++)
	    pvalues[i][s] = -1.0f;
	}
	
	std::vector<std::string> snames;
	ds.getSignalNames(snames);
	
	if(first_target >= 0){
	  names[0] = snames[first_target];
	  pvalues[0].resize(length_secs);
	  for(unsigned int s=0;s<length_secs;s++)
	    pvalues[0][s] = target[first_target];
	}
	
	if(second_target >= 0){
	  names[1] = snames[second_target];
	  pvalues[1].resize(length_secs);
	  for(unsigned int s=0;s<length_secs;s++)
	    pvalues[1][s] = target[second_target];
	}
	
	// everything went ok
	
	for(unsigned int i=0;i<NUMBER_OF_PROGRAMS;i++){
	  signalName[i] = names[i];
	  program[i] = pvalues[i];
	}
	
	return true;
}
  

bool NMCFile::loadFile(const std::string& filename)
{
	FILE* handle = NULL;

	try{
		handle = fopen(filename.c_str(), "rb");
		if(handle == NULL) return false;

		std::unique_ptr<char> name1(new char[33]); // US-ASCII chars
		std::unique_ptr<char> name2(new char[33]);
		unsigned int len = 0;

		if(fread(name1.get(), sizeof(char), 32, handle) != 32){
			fclose(handle);
			return false;
		}

		if(fread(name2.get(), sizeof(char), 32, handle) != 32){
			fclose(handle);
			return false;
		}

		if(fread(&len, sizeof(unsigned int), 1, handle) != 1){ // assumes little endianess here...
			fclose(handle);
			return false;
		}

		if(len > 10000){ // sanity check [in practice values must be always "small"]
			fclose(handle);
			return false;
		}

		name1.get()[32] = '\0';
		name2.get()[32] = '\0';

		// next we read actual "program" data (floats)

		std::unique_ptr<float> program1(new float[len]); // 32bit floats [little endian]
		std::unique_ptr<float> program2(new float[len]);

		if(fread(program1.get(), sizeof(float), len, handle) != len){
			fclose(handle);
			return false;
		}

		if(fread(program2.get(), sizeof(float), len, handle) != len){
			fclose(handle);
			return false;
		}

		// all data has been successfully read, stores values to internal class variables
		signalName[0] = std::string(name1.get());
		signalName[1] = std::string(name2.get());

		// trimming
		signalName[0].erase(signalName[0].find_last_not_of(" ")+1);
		signalName[1].erase(signalName[1].find_last_not_of(" ")+1);

		program[0].resize(len);
		program[1].resize(len);

		for(unsigned int i=0;i<len;i++){
			program[0][i] = program1.get()[i];
			program[1][i] = program2.get()[i];
		}

		fclose(handle);
		return true;
	}
	catch(std::exception& e){
		return false;
	}



}


bool NMCFile::saveFile(const std::string& filename) const
{
	return false; // FIXME: NOT IMPLEMENTED
}

unsigned int NMCFile::getNumberOfPrograms() const
{
	return NUMBER_OF_PROGRAMS;
}

bool NMCFile::getProgramSignalName(unsigned int index, std::string& name) const
{
	if(index > NUMBER_OF_PROGRAMS) return false;
	name = signalName[index];
	return true;
}


bool NMCFile::getRawProgram(unsigned int index, std::vector<float>& program) const
{
	if(index > NUMBER_OF_PROGRAMS) return false;
	program = this->program[index];
	return true;
}


bool NMCFile::getInterpolatedProgram(unsigned int index, std::vector<float>& program) const
{
	if(index > NUMBER_OF_PROGRAMS) return false;
	program = this->program[index];

	return interpolateProgram(program);
}


bool NMCFile::interpolateProgram(std::vector<float>& program)
{
	// goes through the code program and finds the first positive point
	unsigned int prevPoint = program.size();
	for(unsigned int i=0;i<program.size();i++){
		if(program[i] >= 0.0f){ prevPoint = i; break; }
	}

	if(prevPoint == program.size()){ // all points are negative
		for(auto& p : program)
			p = 0.5f;

		return true;
	}

	for(unsigned int i=0;i<prevPoint;i++)
		program[i] = program[prevPoint];

	// once we have processed the first positive point, looks for the next one
	while(prevPoint+1 < program.size()){
		for(unsigned int p=prevPoint+1;p<program.size();p++){
			if(program[p] >= 0.0f){
				const float ratio = (program[p] - program[prevPoint])/(p - prevPoint);
				for(unsigned int i=prevPoint+1;i<p;i++)
					program[i] = program[prevPoint] + ratio*(i - prevPoint);
				prevPoint = p;
				break;
			}
			else if(p+1 >= program.size()){ // last iteration of the for loop
				// we couldn't find any more positive elements..
				for(unsigned int i=prevPoint+1;i<program.size();i++)
					program[i] = program[prevPoint];

				prevPoint = program.size();
			}
		}
	}

	return true;
}



} /* namespace resonanz */
} /* namespace whiteice */
