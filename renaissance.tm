<TeXmacs|1.0.7.15>

<style|generic>

<\body>
  <strong|Renaissance> (PLAN/TODO)

  Tomas Ukkonen 2016

  \;

  The aim of the program (initially command-line tool) is to learn how to
  create visual stimulation that pushes brain to target state <math|\<b-s\>>.
  This will be done in two-phases, initially learning DBN autoencoder that
  creates feature vectors <math|\<b-f\>> from small square pictures
  (downsampled to 128x128 or 256x256) and then using these features
  <math|\<b-f\>> to learn feedforward neural network
  <math|\<b-f\><rsub|new>=NN<around*|(|\<b-f\><rsub|current>,\<b-s\><rsub|current>,\<b-s\><rsub|target>|)>>
  where the states <math|\<b-s\>> are EEG-responses to the pictures. The
  target feature vector is transformed to a picture
  <math|invDBN<around*|(|\<b-f\><rsub|new>|)>> which is shown to the user.
  Finally, the watchers responses <math|\<b-s\>> are collected to a database
  during initial training and during the use of the program allowing better
  and better optimization.\ 

  <with|font-shape|italic|NOTE: a better approach could be deep convolutional
  neural networks which could be developed in dinrhiw2 (just sparse matrices
  in DBN with heuristics to keep responses local).>

  <with|font-series|bold|Deep belief network (DBN) autoencoder>

  The first step of the program is to train autoencoder. Pictures are
  transformed initially to <math|128\<times\>128= 2<rsup|14>> sized RGB
  pictures. The target DBN size is chosen to be
  <math|<around*|(|3\<times\>2<rsup|14>|)>\<times\>2<rsup|14>\<times\>1000\<times\>10>.
  This means that the first GB-RBM weight matrix <math|\<b-W\>> takes
  approximately <math|3> GB of memory which should be possible to learn with
  my laptop having 4 GB RAM. The learning method will be greedy
  layer-by-layer L-BFGS optimizer.

  After learning DBN, it might make sense to create autoencoder and further
  optimize the results and get non-symmetric encoding and decoding neural
  networks processing pictures. However, the total amount of memory required
  by autoencoder is now <math|\<sim\>> 6 GB RAM which is maybe too much.

  \;
</body>