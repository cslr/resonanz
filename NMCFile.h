/*
 * NMCLoader.h
 *
 *  Created on: 15.6.2015
 *      Author: Tomas
 */

#ifndef NMCFILE_H_
#define NMCFILE_H_

#include <string>
#include <vector>
#include <memory>

namespace whiteice {
namespace resonanz {

/**
 * Implements .NMC file loader from the NeuromancerUI
 */
class NMCFile {
public:
	NMCFile();
	virtual ~NMCFile();

	bool loadFile(const std::string& filename);
	bool saveFile(const std::string& filename) const;

	unsigned int getNumberOfPrograms() const;

	bool getProgramSignalName(unsigned int index, std::string& name) const;

	bool getRawProgram(unsigned int index, std::vector<float>& program) const;
	bool getInterpolatedProgram(unsigned int index, std::vector<float>& program) const;

	static bool interpolateProgram(std::vector<float> program);

private:
	static const unsigned int NUMBER_OF_PROGRAMS = 2;

	std::string signalName[NUMBER_OF_PROGRAMS];
	std::vector<float> program[NUMBER_OF_PROGRAMS];
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* NMCFILE_H_ */
