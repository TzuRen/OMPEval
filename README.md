# OMPEval for python

This is a fork of the OMPEval C++ hand evaluator repository. The regular readme can be found below.

This implementation is for using OMPEval's equity calculator in python. It also adds the functionality to calculate equities for smaller boards. For example the equity if the showdown was at the flop, instead of the river. The contribution is one library libhandequity.so which can be imported in python through ctypes. The motivation is that no python library can be nearly as fast [[1](https://github.com/worldveil/deuces)].

## Usage
Here is a simple example of how to use it in python:
```python
# Import ctypes, it is native to python
import numpy.ctypeslib as ctl
import ctypes
libname = 'libhandequity.so'
# The path may have to be changed
libdir = '../OMPEval-fork/lib/'
lib = ctl.load_library(libname, libdir)
# Defining the python function from the library
omp_hand_equity = lib.hand_equity
# Determining its arguments and return types
omp_hand_equity.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_bool]
omp_hand_equity.restype = ctypes.c_double
# Defining the arguments
hole_card = "9sKs"  # 9 of spades and ten of spades
community_card = "8dAhKh" # Number of cards defined here may very between 0 and 5
nb_players = 6 # Number of players may vary between 2 and 6
nb_board_cards = 5 # Default is 5. If = 3, showdown is at flop
std_err_tol = 10**-5 # Default is 10**-5. This is the std in % at which the hand equity will be returned
verbose = True # Default is False
time_1 = time.time()
# Calling the function (goes from python to C++ and back)
hand_equity = omp_hand_equity(hole_card.encode(), community_card.encode(), nb_players, nb_board_cards, std_err_tol, verbose)
print ("The equity is: " + str(hand_equity*100)+"%")
```

### Rebuilding
If a user wants to expand the code, for example to include opponent hand ranges, everything can be rebuild with [rebuild_libs](./rebuild_libs).
This will rebuild both libompeval.a and libhandequity.so.

### Performance and comparison
OMPEval is tested against two python-native libraries. The measure is the amount of evaluations per second:
- [pypokerengine](https://github.com/ishikota/PyPokerEngine) : 14'200 [evals/s]
- [Deuces](https://github.com/worldveil/deuces) : 47'700 [evals/s]
- [OMPEval](https://github.com/zekyll/OMPEval) : 21'100'000 [evals/s]

These measures are not performed in a robust setup, however we can get the order of magnitude. The code used to obtain these measures and example usage of these libraries can be found in [hand_evaluators.py](./hand_evaluators.py)

The overhead of calling the C++ function from python is not an issue as it consistently takes less than 0.5 ms.

###Note on overhead and CombineRange scheme

Another overhead was spotted. As can be seen below, the majority of the total computation time did not come from evaluating hands. This being especially true when there are more players.
```   
               Total time         Time on evals
2 players :    9 [ms]             1.9 [ms]           
4 players :    105 [ms]           2.5 [ms]           
6 players :    335 [ms]           3.3 [ms]           

```
This overhead came from the scheme used to combine hand ranges. This scheme speeds up the distribution of cards. It avoids distributing the same card to different players, which would result in restarting the distribution. It is relevant for highly overlapping hand ranges, but causes overhead for very broad ranges. It was thus removed.
```   
               Total time         Time on evals
2 players :    9.7 [ms]           2.2 [ms]           
4 players :    14.0 [ms]          2.9 [ms]           
6 players :    17.5 [ms]          3.4 [ms]           

```
# OMPEval (regular)

OMPEval is a fast C++ hand evaluator and equity calculator for Texas Holdem poker.

## Hand Evaluator
- Evaluates hands with any number of cards from 0 to 7 (with less than 5 cards any missing cards are considered the worst kicker).
- Multiple cards are combined in Hand objects which makes the actual evaluation fast and allows caching of partial hand data.
- Evaluator gives each hand 16-bit integer ranking, which can be used for comparing hands (bigger is better). The quotient when dividing with 4096 also gives the hand category.
- Has relatively low memory usage (200kB lookup tables) and initialization time (~10ms).
- Can be compiled for both 32- and 64-bit platforms but has better performance on 64bit.
- Uses SSE2/SSE4 when available. On x64 the impact is small, but in 32-bit mode SSE2 is required for decent performance.

Below is a performance comparison with three other hand evaluators ([SKPokerEval](https://github.com/kennethshackleton/SKPokerEval), [2+2 Evaluator](https://github.com/tangentforks/TwoPlusTwoHandEvaluator) and [ACE Evaluator](https://github.com/ashelly/ACE_eval)). Benchmarks were done on Intel 3770k using a single thread. Results are in millions of evaluations per second. **Seq**: sequential evaluation performance. **Rand1**: evaluation from a pregenerated array of random hands (7 x uint8). **Rand2**: evaluation from an array of random Hand objects.
```
        TDMGCC5.1 64bit        TDMGCC5.1 32bit        VC2013 64bit
        OMP  SKPE  2+2   ACE   OMP  SKPE  2+2   ACE   OMP  SKPE  2+2   ACE
Seq:    775  223   1588  80    716  134   1122  75    691  204   1544  69
Rand1:  272  146   19    43    233  99    19    38    262  148   19    39      (Meval/s)
Rand2:  520              87    466              62    529              72
```
###Usage
```c++
#include <omp/HandEvaluator.h>
#include <iostream>
using namespace omp;
int main()
{
    HandEvaluator eval;
    Hand h = Hand::empty(); // Final hand must include empty() exactly once!
    h += Hand(51) + Hand(48) + Hand(0) + Hand(1) + Hand(2); // AdAs2s2h2c
    std::cout << eval.evaluate(h) << std::endl; // 28684 = 7 * 4096 + 12
}
```

## Equity Calculator
- Supports Monte Carlo simulation and full enumeration.
- Hand ranges can be defined using syntax similar to EquiLab.
- Board cards and dead cards can be customized.
- Max 6 players.
- Uses multithreading automatically (number of threads can be chosen).
- Allows periodic callbacks with intermediate results.

In x64 mode both Monte carlo and enumeration are roughly 2-10x faster (per thread) than the free version of Equilab (except headsup enumeration where EquiLab uses precalculated results).

###Basic usage
```c++
#include <omp/EquityCalculator.h>
#include <iostream>
int main()
{
    omp::EquityCalculator eq;
    eq.start({"AK", "QQ"});
    eq.wait();
    auto r = eq.getResults();
    std::cout << r.equity[0] << " " << r.equity[1] << std::endl;
}
```
###Advanced usage
```c++
#include <omp/EquityCalculator.h>
#include <iostream>
using namespace omp;
using namespace std;
int main()
{
    EquityCalculator eq;
    vector<CardRange> ranges{"QQ+,AKs,AcQc", "A2s+", "random"};
    uint64_t board = CardRange::getCardMask("2c4c5h");
    uint64_t dead = CardRange::getCardMask("Jc");
    double stdErrMargin = 2e-5; // stop when standard error below 0.002%
    auto callback = [&eq](const EquityCalculator::Results& results) {
        cout << results.equity[0] << " " << 100 * results.progress
                << " " << 1e-6 * results.intervalSpeed << endl;
        if (results.time > 5) // Stop after 5s
            eq.stop();
    };
    double updateInterval = 0.25; // Callback called every 0.25s.
    unsigned threads = 0; // max hardware parallelism (default)
    eq.start(ranges, board, dead, false, stdErrMargin, callback, updateInterval, threads);
    eq.wait();
    auto r = eq.getResults();
    cout << endl << r.equity[0] << " " << r.equity[1] << " " << r.equity[2] << endl;
    cout << r.wins[0] << " " << r.wins[1] << " " << r.wins[2] << endl;
    cout << r.hands << " " << r.time << " " << 1e-6 * r.speed << " " << r.stdev << endl;
}
```

## Building
To build a static library (./lib/ompeval.a) on Unix systems, use `make`. To enable -msse4.1 switch, use `make SSE4=1`. Run tests with `./test`. For Windows there's currently no build files, so you will have to compile everything manually. The code has been tested with MSVC2013, TDM-GCC 5.1.0 and MinGW64 6.1, Clang 3.8.1 on Cygwin, and g++ 4.8 on Debian.

## About the algorithms used

The hand evaluator was originally based on SKPokerEval, but everything was written from scratch and there are now major differences. OMPEval uses perfect hashing to reduce the size of the main lookup table from 36MB to 200kB. Card rank multipliers were adjusted so that the evaluator allows any number of cards between 0 and 7 in hand. OMPEval also uses a structure for keeping track of the hand data so that the evaluator doesn't need access to the original cards, which allows partial hand data to be cached when enumerating etc. Flush checking is also done more efficiently using bit operations.

In equity calculator the Monte carlo simulation uses a random walk algorithm that avoids the problem of having to do a full resampling of all players' hands after holecard collision. The algorithm also combines players with narrow ranges and eliminates some of the conflicting combos, so it works well even with overlapping ranges where the naive rejection sampling would fail 99.9% of time.

Full enumeration utilizes preflop suit and player isomorphism by caching results in a table and looking for identical preflops. Performance degrades significicantly when the lookup table gets full, but this mostly happens when the situation is infeasible for enumeration to begin with. In postflop the algorithm recognizes some suit isomorphism for roughly 3x speedup, but there is still a lot of improvements to be done in this area.

## 3rd party libraries
OMPEval uses libdivide which has its own license. See http://libdivide.com/ for more info and LICENSE-libdivide.txt for license details.
