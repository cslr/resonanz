<TeXmacs|1.99.2>

<style|<tuple|generic|finnish>>

<\body>
  <strong|Neuromancer Neurostim [resonanz] stimulation selection method:
  documentation, new theory and future plans>

  Tomas Ukkonen, tomas.ukkonen@iki.fi, 2015

  \;

  <with|font-series|bold|Introduction>

  While maybe the best way to handle mental health problems is talking with
  somebody, getting advices, spiritual and emotional empathic support
  together with psychology lead therapy. The sad fact in modern western
  countries (and to some extent in developing countries too) is that many
  problems people have in life are treated using psychiatric drugs which just
  often reduce ability to feel and solve one's problems efficiently.
  <with|font-shape|italic|Additionally, because the treatments are not based
  on measurements and science - they can be arbitrary and ill-chosen.>

  Neuromancer Neurostim is a program that cannot solve most of the problems
  but tries to replace need of drugs which have side-effects and don't work
  that well - with measurements based brain stimulation which aims to:

  <\enumerate-numeric>
    <item>classify and recognize state of the patient (anxiety, low mood,
    overactivity, confusion, afraid..) by measuring brainwaves

    <item>stimulate patient by using pictures, words and sounds which are
    carefully chosen to remedy patients <em|measured problem> or to improve
    overall stability or cognitive abilities of the brain

    <item>try to measure what happens <strong|after> stimulation (after
    withdrawal) - find a stimulation techniques that are long-lasting

    <item>do not try to control the situation - the user of the software can
    diagnose him/herself and set whatever targets for the ``therapy''

    <item>is based on scientific measurements that guide the stimulation
    method - user can get hard numbers that tell how much his or her brain
    moved towards target state because of the stimulation - or was the
    stimulation just place that didn't have any effects
  </enumerate-numeric>

  The software uses Emotiv Insight as the brain-computer-interface (BCI)
  measurements device to measure user's brain state.
  [<with|font-shape|italic|Insight won't be available to the market until the
  end of the summer 2015 so before it development must happen ``blindly''
  before the developed methods can be tested with real data - it might be
  good idea to try to use another brainwave measurement device like Muse in
  order to get more users and make the software more general]>

  <with|font-series|bold|Current method>

  Currently, the software used rather simple method to generate picture and
  keyword sequences that hopefully pushes brain to the right direction. Using
  the interface the user first sets target state <math|\<b-t\>> (Emotiv
  Affectiv signal values) to which program should try to move the brain. The
  program also generates allowed errors/weights <math|\<sigma\><rsub|i>> for
  each component <math|t<rsub|i>>. \ After this the program goes through
  pre-selected keywords <math|<around*|{|k<rsub|i>|}>> and pictures
  <math|<around*|{|p<rsub|j>|}>>, computes prediceted response to each
  picture and keyword <math|\<b-t\><rsub|p<rsub|j>><around*|(|\<b-t\><rsub|0>|)>>
  and <math|\<b-t\><rsub|k<rsub|i>><around*|(|\<b-t\><rsub|0>|)>>
  [<math|\<b-t\><rsub|0>> is the current measured state of the patient] and
  selectes keyword and picture that minimizes distance to the target state:

  <center|<math|p<rsub|j><around*|(|\<b-t\><rsub|0>|)>=min<rsub|p><around*|(|\<b-t\>-\<b-t\><rsub|p>|)>\<Sigma\><rsup|-1><around*|(|\<b-t\>-\<b-t\><rsub|p>|)>>>

  The selected picture and keyword shown at each time frame to the user
  therefore depends only from the current state of the patient
  <math|\<b-t\><rsub|0>> and the target state <math|\<b-t\>>.

  The predicted response to pictures and keywords is learnt during model
  optimization phase. Before using pictures or keywords, they are shown to
  the user (time duration 500ms currently) many times in random order and
  combinations. Users responses to each picture and keyword is then measured
  separatedly so that each stimulation element will have ``training samples''
  <math|<around*|{|<around*|(|\<b-t\><rsub|before>,\<b-t\><rsub|after>|)>|}>>.
  Machine learning (basic feedforward neural network) is optimized using
  L-BFGS optimization method to predict the changes caused by each
  stimulation element.

  <with|font-series|bold|Shortcomings of this method>

  <\itemize-dot>
    <item>method assumes state <math|\<b-t\>> is a well-formed and chosen
    (emotiv meta-signals), in practice better classification and description
    of users state could be possible and would lead to better outcomes.
    Because <math|\<b-t\><around*|(|t|)>> is a time-serie, ICA preprocessing
    might be able to find independent components that are more useful

    <item>the chosen stimulation element does not depend on the previous
    stimulation element, in practice keywords can ``form a story'' and
    pictures are related to each other so in practice we would like to use
    the current picture stimulation too when chosing the next stimulation and
    calculate <math|p<rsub|j*i><around*|(|\<b-t\><rsub|0>|)>> where <math|j>
    is the next stimulation element and <math|i> is the current stimulation
    element. Unfortunately, with large number of different stimulation
    elements (pictures) this grows as <math|O<around*|(|N<rsup|2>|)>> and the
    number of combinations to estimate prediction function for each picture
    pair is too large

    <item>prediction model is calculated for each picture and keyword
    separatedly. Instead, we would like to first calculate feature vector
    <math|\<b-f\>> for each picture and keyword and then calculate global
    prediction function <math|\<b-t\><around*|(|\<b-f\>,\<b-t\><rsub|0>|)>>.
    It could be then applied to new pictures and do not require separated
    training phase. This would also make it quite easily to take into account
    the currently seen picture with its own feature vector
    <math|\<b-f\><rsub|0>>.

    <item>method doesn't try to analyze ``what happens after withdrawal''.
    instead of predicting the next step and going towards it, a better model
    could be a differential equation model like linear one:
    <math|\<b-x\><rprime|'><around*|(|t|)>=\<b-A\>*\<b-x\><around*|(|t|)>+\<b-g\><around*|(|t|)>>,
    where <math|\<b-x\>> is a state vector containing measured stated
    <math|\<b-t\><around*|(|t|)>> and special hidden states
    <math|\<b-h\><around*|(|t|)>> and <math|\<b-g\><around*|(|t|)>> is
    ``stimulation control''. Learning this kind of model (bayesian modelling
    to handle inaccurities) would allow us to learn what will happen when the
    stimulation stops and \ <math|\<b-g\><around*|(|t|)>> becomes zero and
    how to solve for optimal control <math|\<b-g\><around*|(|t|)>>

    <item>the method uses pre-selected pictures and keywords. It could be
    possible to calculate feature vectors <math|\<b-f\>> from pictures and
    pictures from feature vectors (autoencoder). This could be done by using
    GB-RBM deep learning methods. If this works one can then collect examples
    <math|<around*|{|<around*|(|\<b-t\><rsub|current>,\<b-f\>,\<b-t\><rsub|after>|)>|}>>
    and use traditional neural network to learn
    <math|\<b-f\><around*|(|\<b-t\><rsub|current>,\<b-t\><rsub|after>|)>=\<b-f\>>.
    In other words we could generate pictures (and keywords) dynamically
    based on the current and target state of the user

    <item>measurements have always error or variance or we don't have
    measurements at all (blind mode without brainwave measurement device). If
    we have precomputed model we would like to predict the outcomes using
    distributions instead of single values and always calculate mean values.
    In practice (blind mode) we would start from a uniform distribution of
    initial states: 1000 randomly chosen samples of <math|\<b-t\>> values and
    then calculate perdicted value for each sample with every picture
    prediction function and calculate mean predicted error for each
    stimulation. We would then select the best stimulation (according to mean
    values or maybe according to worst case?) and get the new 1000 samples
    generated by choice of this function + some error noise. This would then
    take into accound the uncertaintly and allow to generate stimulation
    sequences that <with|font-shape|italic|are most likely> to push anybody
    with any starting state towards the target state.
  </itemize-dot>

  \;
</body>