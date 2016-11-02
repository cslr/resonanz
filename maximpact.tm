<TeXmacs|1.0.7.15>

<style|generic>

<\body>
  <strong|Theory for random search of maximum impact pictures>

  \;

  Assume initial picture <math|\<b-p\>> and we want to gradient descent
  picture towards picture that causes largest response to measured delta
  EEG-values <math|f<around*|(|\<b-p\>|)>>.

  We want to do L-BFGS search and therefore we need to be able to calculate
  gradient <math|<frac|\<partial\>f<around*|(|\<b-p\>|)>|\<partial\>\<b-p\>>>:

  <center|<center|<math|<frac|\<partial\>f<around*|(|\<b-p\>|)>|\<partial\>\<b-p\>><rsup|T><frac|\<partial\>\<b-p\>|\<partial\>t>=<frac|\<partial\>f<around*|(|\<b-p\>|)>|\<partial\>t>
  <rsup|>>>>

  The values of the function is easy to calculate by showing picture
  <math|\<b-p\>> for <math|\<Delta\>t> time and measuring changes <math|N>
  times. However, calculating the gradient
  <math|\<nabla\>f<around*|(|\<b-p\>|)>=<frac|\<partial\>f<around*|(|\<b-p\>|)>|\<partial\>\<b-p\>>>
  is more complicated, we can show picture again <math|D> times and we have
  equations: \ 

  <center|<math|\<nabla\>f<around*|(|\<b-p\>|)><rsup|T>\<b-u\><rsub|i>=r<rsub|i>>>

  <center|<center|<math|\<b-U\>\<b-g\>=\<b-r\>>>>

  This leads to linear problem for which we can solve for minimum length
  <math|\<b-g\>> solution by calculating pseudoinverse of <math|\<b-U\>>.

  Values of <math|\<b-u\><rsub|i>> are easy to calculate although the
  dimensions<math| \<b-u\><rsub|i>> are large here (picture) we just use data
  delta given the time interval. On the other hand, calculating
  <math|r<rsub|i>> means we calculate in fact the second derivate. This can
  be done by observing changes caused by showing the picture
  <math|2\<Delta\>t> time period and by calculating the curvature. \ 

  \;

  \;

  \;
</body>