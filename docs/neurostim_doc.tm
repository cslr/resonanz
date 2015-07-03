<TeXmacs|1.99.2>

<style|<tuple|generic|finnish>>

<\body>
  <strong|Neuromancer Neurostim [resonanz] stimulation selection method:
  documentation, new theory and future plans>

  Tomas Ukkonen, tomas.ukkonen@iki.fi, 2015

  \;

  <with|font-series|bold|Introduction>

  While maybe the best way to handle mental health problems is talking with
  somebody, getting wise advices, spiritual and emotional empathic support
  together with psychology lead therapy. The sad fact in modern western
  countries (and to some extent in developing countries too) is that many
  problems people have in life are treated using psychiatric drugs which just
  often reduce ability to feel and solve one's problems efficiently.
  <with|font-shape|italic|Additionally, because the treatments are not based
  on measurements and science - they can be arbitrary and ill-chosen.>

  Neuromancer Neurostim is a program that cannot solve most of the problems
  but tries to replace need of drugs which have side-effects and don't work
  that well - with measurements based brain stimulation (instead of
  medication) which aims to:\ 

  <\enumerate-numeric>
    <item>classify and recognize the state of the patient (anxiety, low mood,
    overactivity, confusion, afraid..) by measuring brainwaves

    <item>stimulate patient by using pictures, words and sounds which are
    carefully chosen to remedy patients <em|measured problem> or to improve
    overall stability or cognitive abilities of the brain. The idea is to
    stimulate similarly how music, horror films, art and talking can change
    mood of the listener/watcher.

    <item>measure what happens <strong|after> stimulation (after withdrawal)
    - find a stimulation techniques that are long-lasting

    <item>do not try to control the situation - the user of the software can
    diagnose him/herself and set whatever targets for the ``therapy''

    <item>is based on scientific measurements that guide the stimulation
    method - user can get hard numbers that tell how much his or her brain
    moved towards target state because of the stimulation - or was the
    stimulation just placebo that didn't have any measurable effects
  </enumerate-numeric>

  The software uses Emotiv Insight as the brain-computer-interface (BCI)
  measurements device to measure user's brain state.
  [<with|font-shape|italic|Insight won't be available to the market until the
  end of the summer 2015 so before it development must happen ``blindly''
  before the developed methods can be tested with real data - it might be
  good idea to try to use another brainwave measurement device like Muse in
  order to get more users and make the software more widely used]>

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

  <with|font-series|bold|Shortcomings of this method (Future Work)>\ 

  <\itemize-dot>
    <item>the method assumes state <math|\<b-t\>> is a well-chosen (emotiv
    meta-signals), in practice better classification and description of users
    state could be possible and would lead to better outcomes. Because
    <math|\<b-t\><around*|(|t|)>> is a time-serie, ICA preprocessing might be
    able to find independent components that are more useful. Additionally,
    calculating features directly from EEG data could interesting and allow
    software be more general than using meta signals from a single device

    <item>the chosen stimulation element does not depend on the previous
    stimulation element, in practice keywords can ``form a story'' and
    pictures are related to each other so we would like to use information
    about the current picture too when chosing the next picture and calculate
    <math|p<rsub|j*i><around*|(|\<b-t\><rsub|0>|)>> where <math|j> is the
    next stimulation element and <math|i> is the current stimulation element.
    Unfortunately, with large number of different stimulation elements
    (pictures) this grows as <math|O<around*|(|N<rsup|2>|)>> and the number
    of combinations to estimate prediction function for each picture pair
    becomes too large

    <item>the prediction model is calculated for each picture and keyword
    separatedly. Instead, we would like to first calculate feature vector
    <math|\<b-f\>> for each picture and keyword and then calculate global
    prediction function <math|\<b-t\><around*|(|\<b-f\>,\<b-t\><rsub|0>|)>>.
    It could be then applied to new pictures and do not require separated
    training phase. This would also make it quite easily to take into account
    the currently shown picture with its own feature vector
    <math|\<b-f\><rsub|0>>. GB-RBM+BB-RBM (in development) could be used to
    automatically generate feature vectors

    <item>the method doesn't try to analyze ``what happens after
    withdrawal''. instead of predicting the next step and going towards it, a
    better model could be a differential equation model like a simple linear
    one: <math|\<b-x\><rprime|'><around*|(|t|)>=\<b-A\>*\<b-x\><around*|(|t|)>+\<b-g\><around*|(|t|)>>,
    where <math|\<b-x\>> is a state vector containing measured state
    <math|\<b-t\><around*|(|t|)>> and special hidden states
    <math|\<b-h\><around*|(|t|)>>. Function <math|\<b-g\><around*|(|t|)>> is
    ``stimulation control'' (pictures and keywords). Learning this kind of
    model (bayesian probabilistic modelling to handle inaccurities) would
    allow us to learn what will happen when the stimulation stops and
    \ <math|\<b-g\><around*|(|t|)>> becomes zero and how to solve for optimal
    control <math|\<b-g\><around*|(|t|)>>. [Note: the linear model is way too
    simplistic but a good starting point to start analyzing the situation]

    <item>the method uses pre-selected pictures and keywords. It could be
    possible to calculate feature vectors <math|\<b-f\>> from pictures and
    pictures from feature vectors (autoencoder). This could be done by using
    GB-RBM deep learning methods. If this works one can then collect examples
    <math|<around*|{|<around*|(|\<b-t\><rsub|current>,\<b-f\>,\<b-t\><rsub|after>|)>|}>>
    and use traditional neural network to learn
    <math|\<b-f\><around*|(|\<b-t\><rsub|current>,\<b-t\><rsub|after>|)>=\<b-f\>>.
    In other words we could generate pictures (and keywords) dynamically
    based on the current and target state of the user

    <item>user's don't necessarily have money or want to use BCI device. In
    this case we don't have measurements at all (blind mode without brainwave
    measurement device). If we have precomputed model we would like to
    predict the outcomes using distributions instead of single values and
    always calculate mean values. In practice (blind mode) we would start
    from a uniform distribution of initial states: 1000 randomly chosen
    samples of <math|\<b-t\>> values and then calculate predicted value for
    each sample with every picture prediction function and calculate mean
    predicted error for each stimulation. We would then select the best
    stimulation (according to mean values or maybe according to worst case?)
    and get the new 1000 samples generated (+ model some error noise?). This
    would then take into accound the uncertaintly and allow to generate
    stimulation sequences that <with|font-shape|italic|are most likely> to
    push anybody with any starting state towards the target state.
  </itemize-dot>

  <with|font-series|bold|Difference equations model (initial simple)>

  Because diff.eq. models can give solutions how the system reacts
  <with|font-shape|italic|after stimulation has been stopped (after
  withdrawal)>. It is good idea to try to learn that model in order to try to
  find relatively short sequences that cause (semi-permanent) changes in the
  system which will then move to the right state after the stimulation (note
  that linear models just diverge into infinity, or converge into zero, or
  (complex numbers) oscillate around zero - but this convergence/divergence
  accuracy might be enough for us [we might be able to change variables so
  that the target is always at origo and then just solve for a control that
  makes it most likely for <math|\<b-x\><around*|(|t|)>>+noise to always
  converge to origo and not diverge away]).

  <center|<math|<frac|d\<b-x\><around*|(|t|)>|d*t>=\<b-A\>*\<b-x\><around*|(|t|)>+\<b-g\><around*|(|t|)>>>

  Initially, a simple linear discrete difference model will be attempted
  (fixed length time steps). This will lead into equations:

  <\center>
    <\math>
      \<Delta\>\<b-x\><around*|[|N|]>=\<b-A\>*\<b-x\><around*|[|N|]>+\<b-g\><around*|[|N|]>

      \<b-x\><around*|[|N+1|]>-\<b-x\><around*|[|N|]>=\<b-A\>*\<b-x\><around*|[|N|]>+\<b-g\><around*|[|N|]>

      \<b-x\><around*|[|N|]>=<around*|(|\<b-A\>+\<b-I\>|)><rsup|N>*\<b-x\><around*|[|0|]>+<big|sum><rsup|N><rsub|k=0><around*|(|\<b-A\>+\<b-I\>|)><rsup|k>*\<b-g\><around*|[|N-k|]>
    </math>
  </center>

  Parameters of the model are <math|\<b-A\>> and <math|\<b-g\>> which must be
  estimated from the data. However, because <math|\<b-g\>> is external
  stimulus <math|i> we don't have separate values for each
  <math|\<b-g\><around*|[|n|]>> but <math|\<b-g\><rsub|i>>-value depending on
  the stimuli shown during given time-step.

  Solving <math|\<b-A\>> is relatively simple because we can make
  measurements without any stimulus, we just record we flow of EEG-metasignal
  values and from the training data calculate pairs
  <math|<around*|(|\<Delta\>\<b-x\><around*|[|n|]>,\<b-x\><around*|[|n|]>|)>>.
  After this we can just take a least squares solution that minimizes error
  (initial solution, in practice we should probably make predictions <math|N>
  steps forward and minimize that error instead (MCMC sampling of the error
  probability function):

  <center|<math|min<rsub|\<b-A\>>E<around*|[|<frac|1|2><around*|\<\|\|\>|\<Delta\>\<b-x\>-\<b-A\>*\<b-x\>|\<\|\|\>><rsup|2>|]>>>

  After we have solved for <math|\<b-A\>> it is easy to solve for
  <math|\<b-g\><rsub|i>>:s, for each time step we just calculate
  <math|\<b-g\><around*|[|N|]>=\<Delta\>\<b-x\><around*|[|N|]>-\<b-A\>*\<b-x\><around*|[|N|]>>
  from the data and calculate mean value for different stimuli
  <math|\<b-g\><rsub|i>>. However, in practice we can have multiple different
  stimuli at the same (keywods, pictures) and we therefore have an equation
  (we again model external stimuli as additive to the linear equations):\ 

  <center|<math|\<Delta\>\<b-x\><around*|[|n|]>=\<b-A\>*\<b-x\><around*|[|n|]>+<around*|(|\<b-f\><around*|[|n|]>+\<b-g\><around*|[|n|]>|)>>>

  For the sum of control terms we have <math|\<b-s\><rsub|i*j>=E<around*|[|\<b-f\><rsub|i>|]>+E<around*|[|\<b-g\><rsub|j>|]>>,
  we can write this as a linear equation

  <center|<math|\<b-B\>*<matrix|<tformat|<table|<row|<cell|\<b-f\>>>|<row|<cell|\<b-g\>>>>>>=\<b-s\>><strong|>>

  where matrix <math|\<b-B\>> is <math|D<rsup|2>\<times\>2*D> sized matrix
  and a <math|\<b-s\>=vec<around*|(|\<b-S\>|)>> is a vectorized form of
  <math|\<b-S\><around*|(|i,j|)>=s<rsub|i*j>> matrix. And it is likely that
  we can calculate least squares solution for this using SVD. Notice here
  that the number of measurements needed grow still as
  <math|O<around*|(|N<rsup|2>|)>> so the solution doesn't scale to a large
  number of different keyword/pictures pairs

  Now that we have a initial solution <math|\<b-q\><rsub|0>=<around*|(|\<b-A\><rsub|0>,\<b-f\><rsub|0>,\<b-g\><rsub|0>|)>>
  solved part-wise, the parameters must be plugged to the prediction
  equation:

  <center|<math|\<b-x\><rsup|\<ast\>><around*|[|N|]>=<around*|(|\<b-A\>+\<b-I\>|)><rsup|N>*\<b-x\><around*|[|0|]>+<big|sum><rsup|N><rsub|k=0><around*|(|\<b-A\>+\<b-I\>|)><rsup|k>*\<b-g\><around*|[|N-k|]>>+<math|<big|sum><rsup|N><rsub|k=0><around*|(|\<b-A\>+\<b-I\>|)><rsup|k>*\<b-f\><around*|[|N-k|]>>>

  In real-world we are interested in predicting future outcomes, so we use
  use training data and parameters to always predict <math|N> steps forward
  and calculate <math|N> step prediction error
  <math|\<xi\><rsub|N><around*|(|\<b-q\>|)>>. We model error as gaussian
  noise and use MCMC sampling to ``sample'' parameters <math|\<b-q\>>
  starting from <math|\<b-q\><rsub|0>> which should be some kind of ``maximum
  likelihood'' solution to the problem. We then use Monte Carlo approch and
  use the samples <math|<around*|(|\<b-A\><rsub|i>,\<b-f\><rsub|i>,\<b-g\><rsub|i>|)>>
  to calculate expected prediction value <math|E<around*|[|\<b-x\><rsup|\<star\>><rsup|>|]>>
  and its covariance <math|Cov<around*|[|\<b-x\><rsup|\<star\>>|]>>.

  If this approach seems to work, extend the method to be pseudo non-linear
  by using matrix <math|\<b-A\><around*|(|\<b-x\>|)>> that is dependent on
  the current value of <math|\<b-x\><around*|[|N|]>>.

  \;

  <with|font-series|bold|Spinoff - MaxImpact>

  MaxImpact product idea is a spinoff from developing Neuromancer. This
  product is targeted for marketing people and designers. User takes pictures
  of the product or have a picture of an ad and then the software estimates
  how user is likely to react to it. What colors cause marketing message to
  stay in customer's head the longest etc. The idea is to calculate
  automatically generated feature vector <math|\<b-f\>> from pictures using
  deep learning (GB-RBM) and measure users response. The learnt model is then
  part of the program and can be used to any picture to predict how much the
  customer will react to it. The user can then try different product/ads
  designs/shapes/colors and select the one with the best response.

  Pricing: for a high-quality product made carefully 500 EUR for a software
  license can make sense as it can really improve efficiency of the marketing
  messages and large companies use lots of money in marketing.\ 

  Customers: all companies have products that have appearance, and they want
  to do marketing too. Also marketing companies.

  \;
</body>