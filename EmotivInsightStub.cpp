
#include "EmotivInsightStub.h"
#include <stdlib.h>


EmotivInsightStub::EmotivInsightStub()
{
  
}

EmotivInsightStub::~EmotivInsightStub()
{
  
}


/**
 * returns current value
 */
bool EmotivInsightStub::data(std::vector<float>& x) const
{
  x.resize(5);
  
  x[0] = ((float)rand()) / ((float)RAND_MAX);
  x[1] = ((float)rand()) / ((float)RAND_MAX);
  x[2] = ((float)rand()) / ((float)RAND_MAX);
  x[3] = ((float)rand()) / ((float)RAND_MAX);
  x[4] = ((float)rand()) / ((float)RAND_MAX);
  
  return true;
}


unsigned int EmotivInsightStub::getNumberOfSignals() const
{
  return 5;
}


