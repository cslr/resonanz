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
  
  /*
   * Returns unique DataSource name
   */
  virtual std::string getDataSourceName() const = 0;

  /**
   * Returns true if connection and data collection to device is currently working.
   */
  virtual bool connectionOk() const = 0;

  /**
   * returns current value
   */
  virtual bool data(std::vector<float>& x) const = 0;
  
  virtual bool getSignalNames(std::vector<std::string>& names) const = 0;

  virtual unsigned int getNumberOfSignals() const = 0;
};

#endif /* DATASOURCE_H_ */
