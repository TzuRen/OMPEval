
/**
 * Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 */
#include <stdio.h>
#include <stdlib.h>
#include "./omp/HandEvaluator.h"
#include "./omp/EquityCalculator.h"
#include <iostream>
using namespace std;
using namespace omp;
#include <chrono>
using namespace std::chrono;

extern "C" double hand_equity(const char *hole_card, const char *community_card, const int nb_players,
		const int nb_board_cards_ = 5, const double std_tol = 1e-5, const bool verbose = false)
{
	//string hole_card = "9sTs";
	//string community_card = "";
	//int nb_players = 6;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	unsigned nb_board_cards = nb_board_cards_;
	EquityCalculator eq;
	vector<CardRange> ranges (nb_players);
	ranges[0] = hole_card;
	for( int a = 1; a < nb_players; a = a + 1 ) {
		ranges[a] = "random";
	}


	uint64_t board = CardRange::getCardMask(community_card); //
	uint64_t dead = CardRange::getCardMask(""); //
	// stop when standard error below std_tol%

	auto callback = [&eq](const EquityCalculator::Results& results) {
			//cout << results.equity[0] << " " << 100 * results.progress
			//	<< " " << 1e-6 * results.intervalSpeed << endl;

		if (results.time > 1) // Stop after 1s
			eq.stop();
	};
	double updateInterval = 0.0001; // Callback called every 0.0001s.
	unsigned threads = 0; // max hardware parallelism (default)
	eq.start(ranges, board, dead, false, std_tol, callback, updateInterval, threads, nb_board_cards);
	eq.wait();
	auto r = eq.getResults();

	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>( t2 - t1 ).count();
	if (verbose){
		cout << "[From C++] Total time taken: "<< 1e-3*duration << " [ms], Time taken for evals: " << 1e3*r.time << " [ms]"<<endl;
		cout << "[From C++] Speed: " << 1e-6 * r.speed << " [Meval/s], std: " << r.stdev << endl;
	}


	return r.equity[0];


}
