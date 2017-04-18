<TeXmacs|1.99.5>

<style|<tuple|generic|finnish>>

<\body>
  <center|<\strong>
    How Tranquility Works

    tomas.ukkonen@iki.fi, 2017<center|>
  </strong>>

  \;

  Tranquility is experimental software that uses EEG-measurements and machine
  learning to show pictures and play sounds that aims to push and keep brain
  in a target EEG-state such as meditative (strong alpha frequencies) or
  relaxed state. The software uses new cheap customer-grade EEG-devices
  (currently only Interaxon Muse is supported) to measure brain states. The
  motivation to develop such software comes from the frustration of
  psychiatric treatments that has been stuck to medication based approach.
  Patients (and their brain functions) are seldom or if not-at-all measured,
  medications are static interventions that do not respond dynamically to
  what is happening inside brain and there is no aim to use gentler
  approaches which would have have less side effects (such as Tranquility's
  audiovisual stimulation).

  Tranquility works by first showing and playing semirandom stimulation to
  brain and by using machine learning (artificial neural networks) to learn
  how different sounds and pictures change brain state. Due to the limited
  number of measurement points in customer-grade EEG-devices, there is no
  attempt to model how responses differ in different parts of the brain and
  only average frequency power of delta, theta, alpha, beta, gamma bands etc.
  under the whole area of the brain is measured as a single brain state
  vector <strong|s>. The measurements <math|\<b-s\>> are, however, processed
  using Hidden Markov Models (HMM) by discretizing them (k-means clustering)
  and using HMM-algorithm to try to learn deeper brain states.

  After the initial learning phase, Tranquility software uses reinforcement
  learning extended to continuous actions and states. Mahalobinis distance to
  target brain state <math|\<b-t\>> is used as the reinforcement signal
  <math|r<around*|(|\<b-s\>|)>=<around*|(|\<b-s\>-\<b-t\>|)><rsup|T>\<b-D\><rsup|-1><around*|(|\<b-s\>-\<b-t\>|)>>
  that aims to keep distance to <math|\<b-s\>> as small as possible. For
  pictures <math|\<b-a\><rsub|p>> (continuous picture vector), the
  reinforcement learning uses Q-learning neural network
  <math|Q<around*|(|\<b-s\>,\<b-a\><rsub|p>|)>> to find the best picture
  <math|>(from discrete set of user suplied pictures) to minimizes expected
  future distances to the target state <math|\<b-s\>>. For sounds, continuous
  FM sound synthesizer's parameters <math|\<b-a\><rsub|f>> are optimized
  (Continuous Control with Deep Reinforcement Learning, Google Deepmind,
  2016) by learning policy function <math|\<b-mu\>> (artificial neural
  network) that maps response to current state <math|\<b-s\>> to continous FM
  sound synthesizer action <math|\<b-a\><rsub|f>>. The policy is optimized by
  using policy gradient <math|\<nabla\><rsub|\<b-w\>>J=E<rsub|\<b-s\><rsub|t>><around*|[|\<nabla\><rsub|\<b-w\>>Q<around*|(|\<b-s\>=\<b-s\><rsub|t>,\<b-a\><rsub|f>=\<b-mu\><around*|(|\<b-s\><rsub|t><around*|\||\<b-w\>|\<nobracket\>>|)>|)>|]>>.

  <strong|Future work>

  There was attempt to extend sound synthesis based stimulation to
  computer-based composition of music using RNN-RBM and MIDI notes database
  but I ran out of computer power to compute sufficiently detailed model that
  would give good enough results (results that would sound like music).

  Currently, the picture based stimulation uses discrete set of preselected
  pictures and normal feedforward neural network operating on thumbnails of
  the full pictures. Furtherwork, would modify traditional feedforward neural
  network to convolution neural network (ConvNet) with weight sharing so that
  full-sized pictures could be processed and neural network could learn to
  process fine details of the pictures. Additionally, if there would be
  <with|font-shape|italic|much more> RAM, policy-based method could be
  extended to pictures and would allow using continuous picture stimulation
  instead of preselected pictures.

  \;
</body>