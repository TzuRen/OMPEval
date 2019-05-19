/*
 * main.cpp
 *
 *  Created on: May 17, 2019
 *      Author: cyril
 */

#include "./omp/HandEvaluator.h"
using namespace omp;

int main()
{
    HandEvaluator eval;
    Hand h = Hand::empty(); // Final hand must include empty() exactly once!
    h += Hand(51) + Hand(48) + Hand(0) + Hand(1) + Hand(2); // AdAs2s2h2c
    std::cout << eval.evaluate(h) << std::endl; // 28684 = 7 * 4096 + 12
}


