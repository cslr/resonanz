<TeXmacs|1.0.7.18>

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
    <item>the method only calculates error of reaching the next time step but
    in order to avoid dead-ends a better approach (assuming we can make good
    enough predictions) would be to try all possible stimulation sequences
    <math|N> step forward <math|O<around*|(|M<rsup|N>|)>> and then select
    next step stimulation which minimizes <math|N>-step total error (in
    practice use of <math|N=2> probably makes sense and longer sequences can
    be learnt/generated by using Hidden Markov Models).

    <item>method tries to predict <math|\<b-t\><rsub|after>>, in practice
    changes will be small and often zero, it is probably easier to to try to
    learn function <math|\<b-f\><rsub|1><around*|(|\<b-t\>|)>=\<Delta\>\<b-t\>>
    instead of <math|\<b-f\><rsub|2><around*|(|\<b-t\><rsub|before>|)>=\<b-t\><rsub|after>>
    because now resources of the network are not wasted on trying to learn
    identity function but a bit simpler constant output (zero) function and
    then learn (smaller) areas where <math|\<Delta\>\<b-t\>> is non--zero.\ 

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

  <with|font-series|bold|Difference equations model (initial simple model)>

  Because diff.eq. models can give solutions how the system reacts
  <with|font-shape|italic|after stimulation has been stopped (after
  withdrawal)>. It is good idea to try to learn a model to find relatively
  short sequences that cause (semi-permanent) changes in the system which
  will then <with|font-series|bold|move to the right state after the
  stimulation has stopped> (note that linear models just diverge into
  infinity, or converge into zero, or (complex numbers) oscillate around zero
  - but this convergence/divergence accuracy might be enough for us [we might
  be able to change variables so that the target is always at origo and then
  just solve for a control that makes it most likely for
  <math|\<b-x\><around*|(|t|)>>+noise to always converge to origo and not
  diverge away]).

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
  measurements without any stimulus, we just record free flow of
  EEG-metasignal values and from the training data calculate pairs
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
  number of different keyword/pictures pairs - <with|font-series|bold|unless>
  <math|\<b-B\>> becames full rank earlier which is possible and the minimum
  number of measurements is now theoretically <math|2*D>. (<math|\<b-B\>> has
  a special structure - analyze this in more detail?).

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
  data likelihood'' solution to the problem. We then use Monte Carlo approch
  and use the samples <math|<around*|(|\<b-A\><rsub|i>,\<b-f\><rsub|i>,\<b-g\><rsub|i>|)>>
  to calculate expected prediction value <math|E<around*|[|\<b-x\><rsup|\<star\>><rsup|>|]>>
  and its covariance <math|Cov<around*|[|\<b-x\><rsup|\<star\>>|]>>.

  If this approach seems to work, extend the method to be pseudo non-linear
  by using matrix <math|\<b-A\><around*|(|\<b-x\>|)>> that is dependent on
  the current value of <math|\<b-x\><around*|[|N|]>>.

  \;

  <with|font-series|bold|Spinoff - MaxImpact>

  MaxImpact product idea is a spinoff from developing Neuromancer. This
  product is targeted for marketing people and designers. User takes pictures
  of a product or have a picture of an ad and then the software estimates how
  user is likely to react to it. What colors cause marketing message to stay
  in customer's head the longest etc. The idea is to calculate automatically
  generated feature vector <math|\<b-f\>> from pictures using deep learning
  autoencoder (GB-RBM) and measure users response. The learnt model is then
  part of the program and can be used to any picture to predict how much the
  customer will react to it. The user can then try different product/ads
  designs/shapes/colors and select the one with the best response.

  Pricing: for a high-quality product made carefully 500 EUR for a software
  license can make sense as it can really improve efficiency of the marketing
  messages and large companies use lots of money in marketing.\ 

  Customers: all companies have products that have appearance, and they want
  to do marketing too. Also marketing companies.

  \;

  <strong|Reinforcement Learning>

  Reinforcement learning can be useful because in practice the brain do not
  change immediately but with <with|font-series|bold|delay> and the changes
  are related to stimulus seen just before and after the current stimulation.
  So instead of minimizing for the next step, you look forward for <math|T>
  seconds and calculate <with|font-series|bold|average weighted error>
  (however using average error doesn't fit into reinforcement learning
  theory). Additionally, dynamic programming assumes
  <with|font-shape|italic|discrete number \ states through which we move>.
  There is also a cost function that needs to be minimized which can be just
  be some kind of error function of the target state (difference between the
  current value and the target value of the stimulation program).

  The discretization of the current state with <math|D> dimensions can be
  done just by dividing each variable into <math|K> discrete values which
  leads to exponentially scaling <math|K<rsup|D>> different discrete state
  variables. Now, for a typical problem the number of variables is small
  <math|D\<less\>10> and the because the accuracy of different states can be
  maybe only <math|0.1> for each variable the number of states is
  <math|10<rsup|D>=10<rsup|10>>. This a very large number of states. In
  practice the maximum number of variables is smaller, maybe <math|6>. This
  will then lead to <math|<around*|(|2<rsup|3>|)><rsup|6>=2<rsup|18>
  \<thickapprox\>256.000> different states (8 different discrete value per
  variable and 6 variables). This is somehow manageable but the number of
  variables clearly needs to be smaller, just <math|3> so if we now divide to
  10 different discrete states per variable we get a manageable
  <math|10<rsup|3>=1000> different states.\ 

  Another interesting property of our problem is that we cannot always move
  between different states. This can be modelled as a cost function which
  takes very large values if neural networks (simulation actions) predicting
  the next value say it cannot be done.

  \;

  <\strong>
    Learning Language of Stimulation
  </strong>

  After we have programmed a machine which looks <math|N>-steps forward and
  minimizes stimulation targets we get stimulation sequences
  <math|<around*|{|s<rsub|1>,s<rsub|2>\<ldots\>s<rsub|N>|}>> which we can
  think of as forming a ``language''. ``Characters''/''Words'' that occur
  often together can be useful at causing changes to brain. So instead of
  saving just <math|<around*|{|\<b-t\><rsub|n>,\<b-s\>,\<b-t\><rsub|n+1>|}>>
  one should:

  <\enumerate-numeric>
    <item>save ``corpus'' of stimulation sequences occuring when looking for
    optimal stimulation to push brain into target states according to the
    given program (one can generate ``random'' zigzag programs to generate
    optimized output sequences which try to reach into different randomly
    chosen states in zigzag like fashion)

    <item>find most probable <math|N>-grams and then collect data ad learn
    commonly occuring joint state transitions
    <math|<around*|{|\<b-t\><rsub|n>,\<b-s\><rsub|1>\<b-s\><rsub|2>\<ldots\>\<b-s\><rsub|N>,\<b-t\><rsub|n+N>|}>>
    and attemp to find and use multistep stimulations if they seem to give
    better results (defined how? we still need to calculate most likely
    in-between states <math|\<b-t\><rsub|n+1>\<ldots\>\<b-t\><rsub|N-1>> in
    order to calculate minimum error). But if we use something like Monte
    Carlo simulation to estimate multi-step error given inaccurities in
    prediction, then joint-model should give smaller output variance at
    <math|\<b-t\><rsub|n+N>> and maybe also smaller average error [we can
    maybe learn <with|font-shape|italic|all> intermediate stimulation result
    models <math|<around*|(|\<b-s\><rsub|1>,\<b-s\><rsub|1>\<b-s\><rsub|2>,\<b-s\><rsub|1>\<b-s\><rsub|2>\<b-s\><rsub|3>,\<ldots\>|)>>
    of popular sequences so we don't have to do single step transitions when
    predicting multistep stimulations]

    <item>learn from the optimized stimulation sequences Hidden Markov Model
    (HMM) and then generate/sample multistep likely future steps that are
    tried as likely sequences that exist in data

    <item>another way to use HMMs could use tagging of stimulation elements
    into stimulation classes? (idea: stacked HMMs? hidden states out of
    hidden states?)

    <item>can you merge error rate (of stimulation element <math|\<b-s\>>
    when used with state <math|\<b-t\>>) into HMMs somehow? Large error means
    stimulation element <math|\<b-s\>> is improbable and therefore we know
    that probability of such sequence is small.. (optimization from random
    search generates sequences which probabilities are ``high''er than
    random)
  </enumerate-numeric>

  By using these mathematical ideas, first generating sequences through pure
  optimization (one step forward trying to get into target state) and then
  learning probability model of sequential data to find and generate common
  sequences we should be able to efficiently find stimulation sequences can
  be used to reach target states as well as possible.

  <with|font-shape|italic|The key insight in HMM is that the amount of
  resources in HMM only grows as <math|O<around*|(|N<rsup|2>*D|)>> where
  <math|N> is number of hidden states generating visible observations and
  <math|D<rsup|>> is number of observations. If we can model our data to be
  generated by small number of hidden states then the number of states in
  observations doesn't matter as it grows only linearly
  <math|O<around*|(|D|)>>!> (Now this property is very interesting and it
  would be interesting to try to modify HMM idea to ``stationary'' problems
  without timbecause almost all algorithms that try to analyze <math|D>
  dimensional problems grow at least <math|O<around*|(|D<rsup|2>|)>> because
  you ``have to'' compute how each element affects to each other element
  <with|font-series|bold|but not so in HMM model!>. This property is so good
  that it might be event possible to try to give HMM multiobservational
  picture data because the problem grows linearly to the number of pixels in
  image.

  \;

  <strong|Prediction model clustering>

  Calculating prediction model for each stimulation element separatedly
  requires lots of measurements and restricts use of the computational
  methods to be simple in order to scale to large number of different
  stimulation elements. One approach could be to calculate feature vector
  <math|\<b-x\>> from stimulation elements and then try to calculate joint
  prediction model which is parameterized by <math|\<b-x\>>:
  \ <math|\<b-t\><rsub|next>=\<b-f\><around*|(|\<b-t\><rsub|prev>,\<b-x\>|)>>.
  However, calculating such feature vector from large pictures and large
  amount of keywords is a complicated task.

  Another, somewhat simple approach would be ``model clustering'' approach
  (my own idea). Here we use EM-method (expectation maximization). We assume
  that elements belong to <math|K> different clusters which each has it's own
  distinct prediction model <math|\<b-f\><rsub|k><around*|(|\<b-t\>|)>>. We
  initially assing each stimulation element (and its measurement data)
  randomly to one of the clusters <math|k>. All data belonging to such
  prediction model cluster is then used to optimize/calculate updated
  prediction model <math|\<b-f\><rsub|k><around*|(|\<b-t\>|)>>. After this
  step stimulation elements are re-assigned to prediction models (clusters)
  according to their measurement data. Prediction model <math|k> with the
  smallest prediction error is the chosen model for a stimulation element.
  Now the method follows typicaly EM-clustering approach, prediction models
  are updated and stimulation elements re-assigned as long as there are
  changes in stimulation elements.

  This method is ``discretized version'' and don't use any indermediate
  parameters like feature vectors but concentrates directly to minimize
  prediction error with <math|K> different clusters. After computation of
  such models, one requires very little new data when using prediction
  methods for new stimulation elements. New stimulation elements are always
  assigned to such prediction model which gives smallest prediction error to
  available data. This means that one can assign prediction model to
  stimulation element even when there is just a single measurement available.
  It is also likely that because prediction models are clustered, that they
  are quite different and only a small amount of data is required to
  accuratedly select a good prediction model for a new stimulation element.

  Only problem is that now different stimulation elements has exactly
  <with|font-series|bold|same> predicted response to stimuli. Basically one
  could pick the stimulation element randomly, but for words, for example,
  some kind of corpus based model could pick most likely element. Similarly,
  for pictures it might be good idea to try to minimize difference between
  pictures in order to create ``smooth'' simulation.

  \;

  Possible methods to use:

  <\itemize-dot>
    <item>bayesian neural networks

    <item>neurodynamic programming (reinforcement learning)

    <item>hamiltonian monte carlo, monte carlo sampling

    <item>deep learning: GB-RBMs, RBMs

    <item>differential equations

    <item>parallel tempering

    <item>hidden markov models (HMMs)
  </itemize-dot>

  \;

  <with|font-series|bold|Different approach>

  Because using advanced computational methods, the burden of using lots of
  different images is great. A bit different approach to the problem would be
  to show black screen most of the time and then only quickly show a single
  picture for a while in order to push system back on the right track.

  For example, a group of words (with similar and ``good'' overall response)
  could be shown quickly and system would compute a single <math|T>
  milliseconds long stimulation picture that will push system towards target
  state.

  \;

  \;

  <with|font-series|bold|Brain Source Localization>

  Localization of the signal sources from the brain using measurements from
  measurement points with known location on the surface of head
  <math|\<b-l\><rsub|i>> is a tricky problem. Here is my simple approach to
  the problem that I devided by myself. There are probably better methods
  which one can find from a literature but this is a good starting point.

  Assume we measure power of the signal (FFT) in different frequencies
  (frequency ranges) which only changes slowly unlike actual signal values.
  This mean that for a short time period we can ignore time delays and we
  have additive mixture of power based solely on distance between signal
  source and the measurement location.

  <\center>
    <\math>
      s<rsub|i>=<big|sum>q<rsub|i*j>*<around*|(|\<b-l\><rsub|j>-\<b-l\><rsub|i>|)>p<rsub|j>\<nocomma\>\<nocomma\>\<nocomma\>
    </math>

    <math|\<b-s\>=\<b-Q\>*\<b-p\>>
  </center>

  If we assume that power spectras of the sources (at the chosen frequency
  band, in practice we have a linear equation for each frequency band) are
  independent variables from each other. Then we can use ICA to solve for
  <math|\<b-p\>> values and we have following equation for the sources (each
  assumed to have equal output power).\ 

  <\center>
    <\math>
      <frac|1|4*\<pi\>*\|<around*|\||\<b-l\><rsub|j>-\<b-l\><rsub|i><rsub|>|\<\|\|\>><rsup|2>>=q<rsub|i*j>
    </math>
  </center>

  This equation shows also that miximing matrix coefficients should be above
  zero which constraint should be introduced into ICA somehow. We know the
  measurement point coordinates <math|\<b-l\><rsub|i>> and can then use
  gradient descent to find approximative solution for the problem. Let's
  choose random coordinates with the convex area setup by measurement points
  (we are measuring signals that originate inside from the brain.. hopefully)
  and define error function:

  <center|<math|E=<big|sum><rsub|f>w<rsub|f><big|sum><rsub|i.j><frac|1|2><around*|(|<frac|1|4*\<pi\>*q<rsup|<around*|(|f|)>><rsub|i*j>*>-<around*|\<\|\|\>|\<b-l\><rsub|j>-\<b-l\><rsub|i>|\<\|\|\>><rsup|2>|)><rsup|2>>,
  where>

  <math|p> is index for solutions at different frequency bands and
  <math|w<rsub|f>> (<math|<big|sum>w<rsub|f>=1>) is a positive weight for
  each frequency band when computing solution. It is then easy to calculate
  gradient for <math|\<b-l\><rsub|j>>:

  <center|<math|\<nabla\><rsub|\<b-l\><rsub|j>>E=2*<big|sum><rsub|f>w<rsub|f><big|sum><rsub|i><around*|(|<frac|1|4*\<pi\>*q<rsup|<around*|(|f|)>><rsub|i*j>*>-<around*|\<\|\|\>|\<b-l\><rsub|j>-\<b-l\><rsub|i>|\<\|\|\>><rsup|2>|)>**<around*|(|\<b-l\><rsub|j>-\<b-l\><rsub|i>|)>><strong|>>

  This will lead to a local minima of source locations <math|\<b-l\><rsub|i>>
  and from ICA solutions we also have each source <math|i>'s power spectrum:\ 

  <center|<math|P<rsub|i>=<around*|[|p<rsup|<around*|(|f<rsub|0>|)>><rsub|i>,\<ldots\>,p<rsup|<around*|(|f<rsub|k>|)>><rsub|i>,\<ldots\>p<rsup|<around*|(|f<rsub|N>|)>><rsub|i>|]>>>

  The number of sources to localize is equal to the number of measurement
  points. Now, for customer-grade EEG measurement devices (Interaxon Muse,
  Emotiv Insight) we have only something like 2-3 active measurement points
  at any given time (signal contacts may go up and down and we lose couple of
  signals). This means that we can hope to measure one or two major signal
  sources from the brain and their locations. These locations can be then
  used to estimate approximative power spectrum within brain areas.\ 

  <with|font-series|bold|Measurement results>

  Some generic notes about measurement results.\ 

  Time interval: 333-1000ms. Brain has a reaction time and too short
  stimulation time periods mean brain don't have time to properly process
  stimulation and reactions are based to shapes, colors and forms and not
  content.

  Number of measurements needed is 200-250 or more per element. Lower numbers
  lead to problems.

  Neural network complexity <math|C> is 10-50. (N-C*N-N single layer neural
  network). Output of neural network should be approximated gradient of EEG,
  dEEG/dt.

  Measurement delta output values of EEG are order of 0.34 (EEG mean delta
  during measurement period). Model expection error (with incorrectly
  measured data) was 0.20 with sounds (FM synthesis) and 0.30-0.40 with
  pictures and keywords. The stimulation time used was 333ms. FM sound
  response prediction uses also previous synthesis state as a input (previous
  fm params, next fm params, current eeg state) meaning that

  <\enumerate-numeric>
    <item>either sound is much better way to alter eeg than visual stimuli
    or..

    <item>models which take a previously chosen stimulation element into
    account are much better predicting response. (50% decrese in model error)

    <item>large number of measuremens (thousands) are needed to for
    meaningful prediction instead of aprox. 100 which is used now for
    prediction stimulation element response.

    <item>Model calculation code is working incorrectly.
  </enumerate-numeric>

  Cases 2 and 3 mean that picture stimulation must use picture synthesis code
  (in development) through parameters instead of showing individual pictures.

  <with|font-series|bold|Current Sound Stimulation Implementation>

  Sound is synthesized by forcing fundamental frequency to notes between A-4
  to A-5. After this FM synthesis parameters are constantly glided between
  previous and current configuration so that full glide happens in one
  seconds. Values of FM synthesizer are changed every 1 second meaning that
  FM synthesis parameters are in constant glide.

  Neural network model uses previous and current synthesizer parameters and
  current eeg state as input parameters and tries to predict the next eeg
  state. The neural network is 2-layer feedforward neural network which uses
  complexity parameter 25. This means the network architecture is
  13-325-325-6. The learning code is multistart L-BFGS with 150 iterations
  after which maximum data likelihood distribution
  <math|p<around*|(|data<around*|\||neural network params|\<nobracket\>>|)>>
  is sampled using HMC sampler for 500 samples which are used to predict
  uncertainty of the output using monte carlo integration.

  <with|font-shape|italic|Selection of stimulation sound.> Currently, the
  method generates random gaussian noise around current synthesis parameters
  (80%-90%) and completely random synthesis parameters (10-20%). [1000 tries]
  Neural network model is then used to predict the response and stimulation
  with the smallest risk-adjusted distance\ 

  <center|<math|d<around*|(|\<b-x\>|)>=E<around*|[|\<b-t\>-NN<around*|(|\<b-x\>|)>|]>+0.5<sqrt|Var<around*|[|\<b-t\>-NN<around*|(|\<b-x\>|)>|]>>*>>

  to the target state <math|\<b-t\>> is selected.

  <with|font-shape|italic|Improved selection of sound parameters (?).>
  Instead of using directly generated random parameters <math|\<b-x\>>, a
  gradient descent to the nearest local minima should be used. This can be
  done by simply minimizing error function

  <center|<math|e<around*|(|\<b-x\>|)>=<frac|1|2><around*|\<\|\|\>|\<b-t\>-NN<around*|(|\<b-x\>|)>|\<\|\|\>><rsup|2>>,
  <math|\<nabla\><rsub|\<b-x\>>*e<around*|(|\<b-x\>|)>=<around*|(|NN<around*|(|\<b-x\>|)>-\<b-t\>|)><rsup|T>\<nabla\><rsub|\<b-x\>>NN<around*|(|\<b-x\>|)>>>

  \ by doing iterative line search into decreasing gradient direction until
  convergence. The gradient of <math|NN<around*|(|\<b-x\>|)>*>is easy to
  calculate using chain rule. The only problem is increasing computational
  burden because instead of one function we now have
  <math|p<around*|(|\<b-omega\>|)>> distribution of 100 samples of neural
  network parameters which must be used to estimate gradient and error
  function value. <with|font-series|bold|Only problem with gradient descent
  to local minima is that this method cannot be easily parallelized. While it
  is easy to generate and evaluate random samples in parallel, gradient
  descent algorithm is sequential and requires sophisticated computation.>

  However, this removes the need to generate 600+ samples with gaussian noise
  around current synth parameters in order to find best local next step. If
  the method converges in 20 steps requiring 30 evaluations of neural network
  formula the method has about same speed. Additionally, unlike random
  search, gradient descent scales to large number of sound synthesizer input
  parameters. The only problem with gradient descent is that it cannot find
  global minimum and if neural network model is overfitting, gradient descent
  goes to its local optima which may very well be artificial noise of the
  neural network model.

  Optimization result with multistart L-BFGS 500 iterations, 13-325-325-6
  architecture and 5327 examples in training set (optimization divides in
  <math|\<sim\>2650> samples training set for L-BFGS). UHMC sampler, on the
  other hand, uses all 5327 samples when sampling maximum data likelihood
  parameters.

  13-325-6 (2600 examples) =\<gtr\> LBFGS <math|0.06> error

  13-325-325-6 (5327 examples) =\<gtr\> LBFGS <math|\<sim\>0.07> error, bayes
  error: 0.30.

  <with|font-series|bold|There is some bug in UHMC code. Error values for
  prediction are too high.>

  PCA 13-325-325-6 (5327 examples) =\<gtr\> LBFGS x error, bayes error:\ 
</body>