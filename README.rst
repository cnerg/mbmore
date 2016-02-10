
Mbmore!
==============
This repository is a collection of custom Cyclus facility archetypes that
utilize a random number generator (RNG) to create non-deterministic behaviors.
General methods controlling the behavior (including random number generation
and Gaussian distributions) are defined in the `behavior functions. <https://github.com/mbmcgarry/mbmore/blob/master/src/behavior_functions.h>`_


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

* EveryXTimestep - Returns true every X interval
* EveryRandomXTimestep - Returns true with an approximate frequency defined by X, with individual instances randomly determined.
* RNG_NormalDist - Returns a randomnly generated number from a normal distribution defined by a mean and a sigma (full-width-half-max)
* RNG_Integer - Returns a randomnly choses discrete number between the defined min and max.
* XLikely - Returns true with an average likelihood defined by X [0-1], with individual instances randomly determined. 



Archetypes
----------

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