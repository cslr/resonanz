
#ifndef __EmotivInsightStub_h
#define __EmotivInsightStub_h

#include "DataSource.h"

class EmotivInsightStub : public DataSource
{
 public:
  EmotivInsightStub();
  virtual ~EmotivInsightStub();
  
  /**
   * returns current value
   */
  bool data(std::vector<float>& x) const;
  
  unsigned int getNumberOfSignals() const;
  
  
};

#endif
