
# general notes
say number of chunks for our algorithm is very small for our algorithm, UNLIKE The strided algorithm 
in theoretical computer science we use the word "smoothed" to mean that we are reandomly perturbing the internal structure of the algorithm. The U_i's are randomly perturbed from P_i's.

say fixed stride not constant stride  (if you say constant ppl will think you mean O(1) ;} ) 
whenever you talk about our span verbally say with high probability in n (beaus it s a randomize dalgorhtm)

Tp slide: if squares represent a constant amount of work
note: serial partition is trivial

instead of requiring the input to be random we show that you can use randomization inside the algorithm to get theoretical guarantees for all inputs. 



# talk outline
Dividing objects into separate groups is a common task.  For example, every
halowen I have to partition my candy into 2 groups: the good candy, and the
candy to give to my sisters The partition problem is ubiquitous in computer
science. It plays a crucial role in Quicksort and filtering in addition to
being interesting on its own.
The partition problem takes as iput an array and a pivot value I epresent
values in the aray with heights of rectangles and the pivote value with the
horiontal blue line The goal is to rearange the elemnts int hte array so that
elements less thatn the pivot value occur before elements grreater than the
pivot value.
So that's the partition problem, and when you have a single processor, this
solution is trivial.  The optimal solution is obvious, and has linear run-time,
and is in-place. 


My research focussed on the much more interesting problem of creating a fast
parallel algorithm for the partition problem.  What is a parallel algorith,?
The fundamental primitive of a parallel algoritm is the parallel for loop.  A
parallel for loop allows you to perform several tasks at the same time, as you
can see here. 

More complicated parallle algorotms can be made y combining paralle for loops
and recursion. As illustrated here

Note however that our model of parallelism does nto allow locsk or atomic
variable. This is because it is hard to guarantee that programs using these
will scale well, and implementations of these features are often hardware
specific

The natural metric to analyze a parallel algorithm with is it’s running time on
p processors.  This is called Tp and there are 2 very important special cases
of Tp

Work is the running time on a single processor. This is the sum of the running
times of each step.  Span is the running time on infinitely many processors.
This is the height of the graph.

Brent’s theorem tells us that we can bound Tp with work and span. This means
that if we determine an algorithms’ performance on a single processor and on
infinitely many processors we know its performance on p processors for all p.
Thus, when desinging a parallel algoroithm the goal is to get provable guarantees on work and span.

One of hte most classic parallel algorthms is what is sometimes just called the
standard parallel partition algorithm.  The Standard Parallel partition
algorithm has excellent theoretical performance guarantees.  Due to the steps
outlined here the algorithm incurs work O(n) and span O(log n) This is really
neat. First of all, the work is optimal, being the same as the work of the
serial algorithm. Second, the span is really low. Even adding n numbers takes
span \log n. Furthermore, span log n means that if you have enough processors you can
partition an array of size n in time log n, It also means if you have enough
processors you can quicksort an array in time O(log ^2 n)

However, the standard algorithm is slow in practice.  The reason is bad cache
behavior . This has a technical meaning, but for the purposees of this talk an
algorhtm has bad cache behavior if it uses extra memory, or if it performs
multiple passes over the array Doing this will make an algorithm slow

Unfortunately, the empirically fastest algorithms are theoretically
unsatisfactory Some algorithms use locks or atomics, but it’s hard to guarantee
that these will scale well.  The Strided Algorhtm is good

Our question:
so our quetion was: does there exist an algorithm with theoreitical guarantees that is fast in practice?
And ideally, we want not only theoretically optimal work and span, but also theoretically optimal cache behavior

Our result:

as you might have guessed the smoothed striding algrorhm was inspired by the stided algorthm
I'll now compare and contrast the 2 algorothms
the strided algorthm has good cache behavior in practice on most inptus, which makes it fast in ractice
however, it's worst case span is n. _That means that for some inputs, no matter how many processors you have the algorithm will run as slow as the serial algorithm_
but on random inputs it does slightyl better achieveign span n^2/3. Now, this
still isn't polylogarithmic like the standard algorthm, but this was the
insight htat inspired me to develop the smoothed-striding algorithm.

On the other hand we have the smoothed-stirding algorthm:
it has provably optimal cache bheavior,
   and span log n log log n 
   this means that if you have lots of processors then the running time will be about log n

instead of requiring the input to be random we show that you can use randomization inside the algorithm to get theoretical guarantees for all inputs. 

So the smooothed striing algorthm is theoretically satisfactory 

And my simulations show that my algorithm is also fast in practice.
This graph compares the speedup of the 3 parallel partiiton algorithms I've discussed.
I ran my implementations on an 18-core machine. 
On the x-axis we have the number of threads that the implementations of the
algorithms were run on, and on the y-axis we have the implementations speed up
over a serial baseline implementation of the partition algorithm.
So, in particular, higher speedup is better, and linear speedup in the number of processors is optimal.
As you can see, the standard algorithm scales very poorly due to its bad cache behavior.
On the other hand we hav ehte strided algoritm and the smoothed striding algorithm which both scale very well
On 18 cores the smoothed-striding algorithm performs within 15% of the strided algorithm.

So the smoothed-striding algorithm has empirically good performance in addition to theroetically good performance.

Now, I'd like to descibre my algorithm.
First it is useful to describe the strided algorithm.


...
We perform a serial partition on each P_i. And we are performing the serial partitition on all P_is at the same time.
...


Smoothed striding algorithm


