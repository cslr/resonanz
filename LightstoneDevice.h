/*
 * LightStoneDevice.h
 *
 *  Created on: 4.7.2015
 *      Author: Tomas
 */

#ifndef LIGHTSTONEDEVICE_H_
#define LIGHTSTONEDEVICE_H_

#include "DataSource.h"

#include <thread>
#include <mutex>


namespace whiteice {
namespace resonanz {

/**
 * Implements data source/device for WildDrive Lightstone USB meditation device
 */
class LightstoneDevice: public DataSource {
public:
	LightstoneDevice();
	virtual ~LightstoneDevice();

	  /*
	   * Returns unique DataSource name
	   */
	  virtual std::string getDataSourceName() const;

	  /**
	  	 * Returns true if connection and data collection to device is currently working.
	  	 */
	  virtual bool connectionOk() const;

	  /**
	   * returns current value
	   */
	  virtual bool data(std::vector<float>& x) const;

	  virtual bool getSignalNames(std::vector<std::string>& names) const;

	  virtual unsigned int getNumberOfSignals() const;

private:
	  void lightstone_loop(); // worker thread loop

	  // helper functions
	  float estimate_heart_rate(const std::vector<float>& data,
			  const std::vector<float>& t, const float sampling_hz) const;

	  void calculate_mean_var(const std::vector<float>& data,
	  		const unsigned int start, const unsigned int end, float& mean, float& var) const;

	  std::thread* worker;
	  bool running;
	  bool has_lightstone;
	  mutable std::mutex start_mutex; // for handling threading variables

	  std::vector<float> value; // current sensor value
	  mutable std::mutex data_mutex;
	  long long latest_data_point_added; // milliseconds since epoch
};

} /* namespace resonanz */
} /* namespace whiteice */

#endif /* LIGHTSTONEDEVICE_H_ */
