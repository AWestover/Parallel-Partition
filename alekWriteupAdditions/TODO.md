# TODO now:
  take Take Related Algorithms section from Bill's paper
  this is the base
  describe the history
  and then add the Grouped Partition algorithm (only version 3)
    concisely explain why it's cool  ********this is the biggest part I need to add

  look at tense agreement. Especially for stuff where I say P_js U_ys etc. It is probably wrong to say "for all indices v_y", instead say "for all indices v_0,v_1,...,v_n-1"
  read paper for block, collection, group etc consistency  -- bill said to keep reminders of what these are in the paper (ppl prob wont catch on to the fact that we consistently refer to Pj as a part / block etc because its kinda confusing)

  make sure that I never say "compute min(v_y)" in serial because we don't!!! (I thought there were less v_ys then there really were, g is pretty big)

#### Questions for Bill:
  * Improve global definitions and usage
  * how much redundancy is good, how much is too much? work on the math / english balance

### To Do Later:
    * we probably need to mess with delta some more to get a more optimal value. Changing the number of groups on the top level <might> have a huge effect on parallelism, just like the effect of changing `b`. 
    we could go from span O(s) to span O(log s) on this step
    It's not like it reduces the span, it just lowers the constants attached a tiny bit
  * do Brent's theorem: compute *work* for algorithms (not just span)
  * say more about Bill's algorithm in my paper (look over his paper again, describe his algorithm in my paper?)
  * Add "systems stuff" and graphs
  * Add cache misses analysis for grouped partition algorithm
  * at some point (probably systems stuff) we need to talk about the fact that we only consider the case where the input size is a multiple of b\cdot s, and how we deal with this (how we dea with it is we just insert the stuff that doesn't fit into the middle)

#### Good Practices:
  spell check your document!!
    * `:setlocal spell` underlies this
    * but <leader> p is what you really want
  don't forget <leader> l to compile (reading stuff is easier in a pdf)

  Use this comand to compile it with correct citation stuff (only necessary if you are going to go print it)
    pdflatex -synctex=1 -interaction=nonstopmode "writeup".tex
	
