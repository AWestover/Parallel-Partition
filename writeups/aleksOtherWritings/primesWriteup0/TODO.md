# TODO:

## English
  *  look at tense agreement. Especially for stuff where I say P_js U_ys etc. It is probably wrong to say "for all indices v_y", instead say "for all indices v_0,v_1,...,v_g-1"
    * read paper for block, collection, group etc consistency  -- bill said to keep reminders of what these are in the paper (ppl prob wont catch on to the fact that we consistently refer to Pj as a part / block etc because its kinda confusing)
  * how much redundancy is good, how much is too much? work on the math / english balance
  * Improve global definitions and usage

## writeup completeness
  * compute work for algorithms (not just span)
  * say more about Bill's algorithm in my paper (look over his paper again, describe his algorithm in my paper?)
  * Add "systems stuff"--e.g. graphs

## writeup correctness
  * make sure I explicitly say every time g is defined to be n / (bs) (g = n/ (bs) is not good enough, need to say it is by definition)
  * "Cache-Efficient Partial-Partition Algorithm" refers to ONLY ONE LEVEL OF RECURSION in an algorithm that we made for solving parallel partition
    --- you can solve parallel partition by applying the Cache-Efficient Partial-Partition Algorithm and then solving a recursive subproblem with more itterations of the Cache-Efficient Partial-Partition Algorithm 
    -- or you can solve parallel partition by applying the Cache-Efficient Partial-Partition Algorithm and then solving the remaining subproblem with Bill's O(logn log logn) algorithm

  * When analyzing the cache misses, we need to be more explicit about why there are so few cache misses in the first level of recursion. In particular, you should say what parts of memory you're treating as being pinned in cache for all the processors (I think there is some array you're assuming is pinned). And then you should be explicit that, besides that, each processor actually only needs to keep its most recent $O(1)$ I/Os in cache at any given moment. (I think that's true?)

