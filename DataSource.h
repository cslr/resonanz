/*
 * DataSource.h
 *
 *  Created on: 1.3.2013
 *      Author: omistaja
 */

#ifndef DATASOURCE_H_
#define DATASOURCE_H_

#include <vector>
#include <string>

class DataSource {
public:
  virtual ~DataSource(){ }
  
  /**
   * returns current value
   */
  virtual bool data(std::vector<float>& x) const = 0;
  
  virtual bool getSignalNames(std::vector<std::string>& names) const = 0;

  virtual unsigned int getNumberOfSignals() const = 0;
};

#endif /* DATASOURCE_H_ */
