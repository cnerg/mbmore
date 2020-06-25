
Mbmore!
==============
This repository is a collection of custom Cyclus facility archetypes that
utilize a random number generator (RNG) to create non-deterministic behaviors.
General methods controlling the behavior (including random number generation
and Gaussian distributions) are defined in the `behavior functions. <https://github.com/cnerg/mbmore/blob/main/src/behavior_functions.h>`_


Behavior Functions
------------------
These functions use the C++ 'srand' to create behaviors that change in time.
The RNG is seeded only once per simulation.  The seed value can be controlled
by the <rng_seed> tag in individual archetypes. (Although there are rng_seed
inputs for each archetype, it is only set once, so avoid defining it multiple
times in one input file). If set to -1, rng_seed is seeded on the system time at
simulation execution. Otherwise RNG is seeded on the value of rng_seed, for
reproducibility.

Available behavior functions are:

* *CalcYVal*: For various functions, returns Y given a set of relevant constants and X.
  
  - ``Constant`` [y0]:  a constant line with the form y = y0
  - ``Linear`` [y0, m]: line with the form y = y0 + m*x
  - ``Power`` [A, (B=1)]: powerlaw fn of the form y = B*(x^A). B defaults to 1 if not specified.
  - ``Step`` [y0, yf, (T)]: step function of the form y = y0  (x < T), y = y1 (x >= T).

* *EveryXTimestep* - Returns true every X interval
* *EveryRandomXTimestep* - Returns true with an approximate frequency defined by X, with individual instances randomly determined.
* *RNG_Integer* - Returns a randomnly choses discrete number between the defined min and max.
* *RNG_NormalDist* - Returns a randomnly generated number from a normal distribution defined by a mean and a sigma (full-width-half-max)
* *XLikely* - Returns true with an average likelihood defined by X [0-1], with individual instances randomly determined. 



Archetypes
----------

CascadeEnrich
+++++++++++++
Based on `cycamore:Enrich <http://fuelcycle.org/user/cycamoreagents.html#cycamore-enrichment>`_ , this facility designs a cascade based on the physical parameters of the individual counter-current centrifuges being used, the target assays and cascade feed flow, and the available number of centrifuges. The cascade is designed as an ideal one-up, one-down cascade in which the product/tails from one stage moves up/down to the next stage, respectively. Centrifuge machine performance is calculated using the Ratz equation and assuming an R2 (U-238) withdrawl radius of 0.975*a (radius of the centrifuge).  Only one physical centrifuge design maybe used in the cascade, it cannot mix centrifuge designs.  The cascade produced deviates from an ideal cascade only in that integer numbers of centrifuges must be used for each stage.  If the number of available centrifuges is insufficient to meet both the target assays and the target feed flow, then feedflow will be reduced to meet the other requirements.  The cascade will produce an enrichment level At Least as high as the ``design_product_assay``, again constrained by integer stage steps.   This archetype automatically designs the cascade at the beginning of the simulation, at which point the physical configuration of the centrifuges and cascade design are fixed for the remainder of the simulation.  The facility will still attempt to produce material of non-target product assay as requested, within the limits of integer number of stages, SWU and feed flow capacity constraints. When processing off-design material assays, separative capacity will be reduced. The facility assumes two-isotope enrichment (U-235 and U-238) such that molecutlar mass is 0.352kg/mol (UF6), a cut (ratio of product/feed quantity) of 0.5, an internal flow of 2.0 (in practice dependent on baffle/scoop design and can range from 2-4), and a pressure ratio of 1000 (Glaser, Science and Global Security, 2009).

Future work: Cut, efficiency (which can be significantly less than 1), pressure ratio, and internal flow should be user-defined with reasonable defaults. Blending capability to achieve the exact requested enrichment level. R2 withdrawl radius should be user defined as well (called in enrich_functions::CalcDelU). Time-based calculations (flow rates, SWU etc) should be changed to use arbitrary time base, currently timesteps of one month are assumed.

Enrichment and cascade design calculations are implemented in the accompanying enrich_functions.cc file.
  - ``design_feed_flow``: The amount of feed material the cascade is
    initially designed to process (kg/month).  Combined with
    ``design_feed_assay``, ``design_product_assay``, and ``design_waste_assay``,
    this sets the ideal
    number of centrifuges for the cascade. If there are not sufficient
    centrifuges available then actual production will not achieve this target.
  - ``max_centrifuges``: The total number of centrifuges available to design
    the cascade. If there are more available than necessary to achieve the
    ``design_feed_flow``, (given ``design_feed_assay``, ``design_product_assay``
    and ``design_waste_assay``)  then the cascade will use as many of the
    centrifuges as possible given the constraint that each stage must have an
    integer number of machines.
  - ``design_feed_assay``: Expected feed assay for which ideal cascade
    is designed.
  - ``design_product_assay``: Desired product assay for which ideal cascade
    is designed.
  - ``design_tails_assay``: Desired waste assay for which ideal cascade
    is designed.
  - ``temp``: Operating temperature of individual centrifuge machine, which
    is used to calculate machine separative capacity (Kelvin).
  -``centrifuge_velocity``: Desired operational velocity of the centrifuge,
    which is used to calculate machine separative capacity (m/s).
  - ``height``: height of the centrifuge, which is used to calculate machine
    separative capacity (m).
  - ``diameter``: diameter of the centrifuge, which is used to calculate machine
    separative capacity (m).
  - ``machine_feed``: maximum throughput for a single centrifuge, which is used
    to calculate machine separative capacity (m).

    



InteractRegion
++++++++++++++
This manager region is used to study the likelihood of a state pursuing and acquiring a nuclear weapon given relationships with a set of neighboring states, represented by StateInst.  The InteractRegion is a super-region that includes All States in the simulation and acts as a 'Simulation Context' for universal information such as the functional form and weighting of the Pursuit and Acquire decision-making equations. The Pursuit and Acquire Equations define the level of motivation for the country on a scale of 0-10, The Likely Equation (LikelyEqn) rescales this motivation into a likelihood of action at each timestep (0-1).
 - ``acquire_weights`` (not implemented): A map of (Factor Name, Weight). The equation that defines how motivated a state is to acquire a weapon on a given timestep; these are the relative weights of each of the factors considered in the determinination. The sum of all of the weights should be 1, and any factor whose weight is not defined here will be ignored in the calculation (weight = 0). Factor names are case sensitive and should match those defined in the StateInst.
 - ``pursuit_weights``: A map of (Factor Name, Weight). The equation that defines how motivated a state is pursue a weapon on a given timestep; these are the relative weights of each of the factors considered in the determinination. The sum of all of the weights should be 1, and any factor whose weight is not defined here will be ignored in the calculation (weight=0). Factor names are case sensitive and should match those defined in the StateInst.
 - ``likely_convert``: A map of (Equation, (Form, <Constants>)). After the motivational equation (Pursuit or Acquire) returns a value between 0 and 10, ``likely_convert`` rescales that into a likelihood of taking action (0-1).  The form of the equation can be defined to be any of those available in the behavior_function method *CalcYVal*.  For example, ('Pursuit', ('Power', [2])) means LikelyEqn = (Pursuit/10)^2. Then for Pursuit=2, LikelyEqn = 0.04, while for Pursuit = 9, LikelyEqn = 0.81.  The StateInst uses the result of LikelyEqn to convert to Yes or No decision for the timestep.
 - ``p_conflict_map``: A map of (Primary State, (Secondary State, Relation)) that defines the conflict between each pair of states at t=0.  Each state pairing must be defined (i.e. separate entries for StateA-StateB and StateB-StateA).  Options are +1 (friendly), 0 (neutral), -1 (antagonistic).  If states are in agreement about their mutual relationships, ``symmetric`` should be set to 1 (True). Otherwise states can have inconsistent perceptions of one another. Dynamic changes to  the conflict between two states are applied using the StateInst ``pursuit_factors`` variable.
 - ``symmetric`` (default 0): If 1 (True) then any changes in conflict between two states (StateA-StateB) will be mirrored also (StateB-StateA will have the same value). Otherwise if set to 0 (False) then states can have mutually inconsistent perceptions.  This flag affects only Changes to the relationships (defined StateInst), does not force initial conflict values to be symmetric.

A note on *Conflict*. Conflict is an interactive factor between states in the simulation. It is defined by a combination of relationship between states (enemy, ally or neutral) as well as the weapons status of each state. It updates in time as weapons status changes.  Each state-pair receives a conflict score between 0-10 based on `this table. <https://docs.google.com/document/d/1c9YeFngXm3RCbuyFCEDWJjUK9Ovn072SpmlZU6j1qhg/edit?usp=sharing>`_ . In a simulation with more than 2 states, the net conflict score for state A is the average of its individual pair conflict scores with B, C, D.. . .

StateInst
+++++++++
This manager institution is used along with InteractRegion to study whether a state will pursue or acquire a nuclear weapon given a set of political or economic internal Factors, as well as its relationships with a set of neighboring states.  At each timestep, the state decides whether or not to pursue a nuclear weapon by calculating the Pursuit Equation using these Factors (the relative weights of the factors are defined in the InteractRegion).  If the state decides to Pursue, then on the next timestep, a Secret Enrichment Facility and a Secret Receiver (sink) are deployed. The pursuit equation continues to be calculated at each timestep, and its value is used to determine whether the stae has succeeded in acquiring a weapon. If the state succeeds in Acquiring at time T, then HEU is produced at (T+1), and it is moved to the Receiver at (T+2), the quantity of HEU produced is defined in the input file as the requested quantity for the secret sink.
  - ``acquire_factors``: Not supported (see ``pursuit_factors`` for reference)
  - ``pursuit_factors``: Map of (Factor, (Function, Constants)). Each factor affecting decision to pursue weapons is defined with a name (case sensitive) and a function that describes its time dynamics.  Individual factors define the States independent perspective,: "Auth" (authoritarianism), "Enrich", "Mil_Sp" (military spending/GDP), "Reactors", "Sci_Net" (scientific network), "U_Reserve". Relational factors describe how the States interact with one another, and are: "Conflict","Mil_Iso" (military isolation).  Factor names may be a subset of all allowed factors and must have a correspondingly defined value in ``pursuit_weights``.  Factors must always have values between 0 and 10, where large values increase the likelihood of proliferation. For Individual Factors, functions can be chosen from the behavior_function method *CalcYVal*, and require the corresponding vector of constants. For example, ('Enrich', ('Step',[3,6,10])) means the Enrich Factor is defined by a step function so that its value is 3 from t = 0 to t = 10, and then it increases to 6 for the remainder of the simulation. For Relational Factors (eg Conflict), the t=0 values are defined in InteractRegion.  To change them during the simulation: P_f[\"Conflict\"]= (\"OtherState\", [Value, Time]). Then the relation between this state and OtherState changes at Time to be the new value (+1 = friendly, 0 = neutral, -1 = enemy. If InteractRegions' ``symmetric`` parameter is 1 (True), then the OtherState's record of the relationship will be correspondingly changed. If Time is omitted, then the timestep will be randomly chosen.
  - ``declared_protos``: Vector of prototype names. All declared facilities controlled by the state at the beginning of the simulation (mid-simulation deployment of declared facilities is not currently supported)
  - ``secret_protos``: Vector of prototype names. The names of any secret prototypes to be deployed when the state decides to proliferate.  All secret facilities are deployed the first timestep after Pursuit is True.
  - ``rng_seed``: (optional)  sets the RNG seed value for the simulation (should be defined only once in the input file). If set to -1, the system time at simulation runtime is used, otherwise the integer is passed directly as the seed.
  - ``weapon_status``: Defines whether each state begins the simulation as a non-weapon-state (0), pursuing weapons (2), or having acquired weapons (3).  If pursuing or acquired, then a Secret Sink and Secret Enrichment facility will be deployed by that state at the start of the simulation.  

RandomEnrich
+++++++++++++
Based on `cycamore:Enrich <http://fuelcycle.org/user/cycamoreagents.html#cycamore-enrichment>`_ , its additional features include variable tails assay, inspector swipe tests, and bidding behavior that can be set to occur at Every X timestep or at Random timesteps. All additional behaviors default back to the standard cycamore:Enrich.
  - ``social_behav``: Defines the character of time-varying behavior on offering
    bids. Options are 'None' (defaults to cycamore archetype), 'Every' (bid
    frequency is determined by ``behav_interval``, 'Random' (effective bid
    frequency is determined by ``behav_interval``.
  - ``behav_interval``: Defines the effective frequency with which bids are
    placed. During all other timesteps, no bids are made to offer out
    materials from the enrichment facility.
  - ``sigma_tails``: If set, it defines the standard deviation of a
    truncated Gaussian distribution that is used
    to vary the tails assay over time. The mean of the distribution is set
    with ``tails_assay``. The variation limited to be within the range
    [``tails_assay`` - ``sigma_tails``, ``tails_assay`` + ``sigma_tails``]
  - ``rng_seed``: sets the RNG seed value for the simulation (should be defined
    only once in the input file). If set to -1, the system time at simulation
    runtime is used, otherwise the integer is passed directly as the seed.
  - ``inspect_freq`` : defines an average frequency of inspections (implemented
    with EveryRandomX).  Creates an Inspections Table (if inspect_freq!=0)
    containing the columns: ``AgentID``, ``Time``, ``SampleLoc``,
    ``PosSwipeFrac``.  For each inspection and swipe location, ``n_swipes``
    are taken, and the fraction of these swipes that is positive for HEU (>20%
    enriched) is recorded in the table.  If the liklihood of a false positive (
    ``false_pos``) is non-zero, then XLikely is applied to every swipe that
    originally measures negative.  If the liklihood of a false negative
    (``false_neg``) is non-zero, then XLikely is applied to every swipe that
    originally measures positive for the remainder of the simulation. A swipe
    can measure inherently positive only if HEU has actually been produced.  If
    HEU has been produced and not previously detected, it's likelihood of
    detection increases approximately linearly across duration of the
    simulation.  If HEU is produced continuously, then it only registers as
    detectable when increments of 0.1kg have been accumulated (imagining that it
    is removed from the cascades in this increment and therefore there are
    discrete opportunities for contamination).
  - ``n_swipes`` : number of swipes for a single sample during inspection.
    (default 10)
  - ``false_pos`` : likelihood that an inherently negative swipe will falsely
    record as positive (default 0)
  - ``false_neg`` : likelihood that an inherently positive swipe will falsely
    record as negative (default 0)

RandomSink
+++++++++++
Based on `cycamore:Sink <http://fuelcycle.org/user/cycamoreagents.html#cycamore-sink>`_ , its additional features include ability to accept multiple recipes,  modifiable material preference, material request behavior can be set, trading can be suppressed before a specified timestep, material requests can occur at Every X timestep or at Random timesteps, and quantity requested can be varied using a Gaussian distribution function.
  - ``avg_qty``: Quantity of material requested. If ``sigma`` is also set then
    this is the mean value of time-varying material request defined by a
    Gaussian distribution.
  - ``sigma``: The standard deviation (FWHM) of the gaussian distribution used
    to generate the quantity of material requested.
  - ``social_behav``: Defines the character of time-varying behavior in
    requesting materials. Options are 'None' (defaults to cycamore archetype),
    'Every' (bid frequency is determined by ``behav_interval``, 'Random'
    (effective bid frequency is determined by ``behav_interval``, 'Reference'
    (queries the RNG to preserve order but requests a zero quantity, preserving
    the RNG querying of other archetypes)
  - ``behav_interval``: Defines the effective frequency with which request for
    material are placed. During all other timesteps, no bids are made to offer
    out materials from the enrichment facility.
  - ``rng_seed``: sets the RNG seed value for the simulation (should be defined
    only once in the input file). If set to -1, the system time at simulation
    runtime is used, otherwise the integer is passed directly as the seed.
  - ``t_trade``: At all timesteps before this value, the facility does not make
    material requests. At times at or beyond this value, requests are made,
    subject to the other behavior features available in this arcehtype.