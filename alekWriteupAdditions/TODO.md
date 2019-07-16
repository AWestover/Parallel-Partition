# TODO:

## TOP PRIORITY:
  * check correctness on all the math

## Code
  * see todo list in the code
  * __make sure parameters chosen align with writeup's advertised parameters__

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

# Bill's Suggestions:
## introduction of cache modeling (work on together at mathcamp)
## should try to get experimental data by the end of Mathcamp

(1) I'd like to see a theorem statement that very explicitly states the full most general result. i.e., it should include (a) the running time, (b) the span, (c) the number of cache misses, and (d) the probability bound. Ideally, all of these should be explicit. (So, for example, the $o(1)$ in the cache misses should be made something explicit.) Probably, the approach that makes the most step is to make this theorem be only about the top level of recursion, and to also include a bound on $v_max - v_min$. To really make the theorem as generally applicable as possible, we may also want to parameterize it by $b, \delta, s,\ldots$. We can then do as corollaries (maybe they should be theorems and not corollaries) the important special cases (i.e., which are currently Theorem 4.3 and Proposition 4.1). Does that make sense / what do you think?
(2) When analyzing the cache misses, we need to be more explicit about why there are so few cache misses in the first level of recursion. In particular, you should say what parts of memory you're treating as being pinned in cache for all the processors (I think there is some array you're assuming is pinned). And then you should be explicit that, besides that, each processor actually only needs to keep its most recent $O(1)$ I/Os in cache at any given moment. (I think that's true?)
(3) When stating theorems, it's good to define (or even redefine) all of the variables in the theorem statement within the body of the theorem statement (rather than, for example, right before the theorem statement). 
(4) Make sure that when you prove a theorem, you put everything that you're proving in the theorem statement. (Theorem 4.2's statement, for example, doesn't mention anything about cache misses)
(5) More math symbols. Every major mathematical step should have a centered equation that you reference later, and there should be paragraph breaks between major mathematical steps. 

Priority: 521( 34 )

