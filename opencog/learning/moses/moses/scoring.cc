/*
 * opencog/learning/moses/moses/scoring.cc
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Moshe Looks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "scoring.h"
#include <opencog/util/numeric.h>
#include <cmath>

namespace opencog { namespace moses {

// helper to log a combo_tree and its behavioral score
inline void log_candidate_bscore(const combo_tree& tr,
                                 const behavioral_score& bs) {
    if(logger().getLevel() >= Logger::FINE) {
        stringstream ss_tr;
        ss_tr << "Evaluate candidate: " << tr;
        logger().fine(ss_tr.str());
        stringstream ss_bsc;
        ostream_behavioral_score(ss_bsc, bs);
        logger().fine(ss_bsc.str());
    }
}


int logical_score::operator()(const combo_tree& tr) const
{
    // std::cout << "scoring " << tr << " " << arity << " "
    //   << target << " " << combo::complete_truth_table(tr,arity) << " "
    //   << (target.hamming_distance(combo::complete_truth_table(tr,arity))) << std::endl; // PJ

    return -int(target.hamming_distance(combo::complete_truth_table(tr, arity)));
}
behavioral_score logical_bscore::operator()(const combo_tree& tr) const
{
    combo::complete_truth_table tt(tr, arity);
    behavioral_score bs(target.size());

    std::transform(tt.begin(), tt.end(), target.begin(), bs.begin(),
                   std::not_equal_to<bool>()); //not_equal because lower is better

    return bs;
}

contin_t contin_score::operator()(const combo_tree& tr) const
{
    try {
        std::cout << "scoring " << tr << std::endl;
        contin_t sc = -target.abs_distance(combo::contin_output_table(tr, cti, rng));
        std::cout << sc << " X " << tr << std::endl;
        return sc;
    } catch (...) {
        std::cout << "threw" << std::endl;
        return get_score(worst_composite_score);
    }
}

contin_t contin_score_sq::operator()(const combo_tree& tr) const
{
    try {
        return -target.sum_squared_error(combo::contin_output_table(tr, cti, rng));
    } catch (...) {
        stringstream ss;
        ss << "The following candidate has failed to be evaluated: " << tr;
        logger().warn(ss.str());
        return get_score(worst_composite_score);
    }
}

behavioral_score contin_bscore::operator()(const combo_tree& tr) const
{
    combo::contin_output_table ct(tr, cti, rng);
    behavioral_score bs(target.size());

    behavioral_score::iterator dst = bs.begin();
    for (combo::contin_output_table::const_iterator it1 = ct.begin(), it2 = target.begin();
         it1 != ct.end();)
        *dst++ = fabs((*it1++) - (*it2++));

    return bs;
}

behavioral_score occam_contin_bscore::operator()(const combo_tree& tr) const
{
    cti.set_consider_args(argument_set(tr)); // to speed up binding
    combo::contin_output_table ct(tr, cti, rng);
    behavioral_score bs(target.size() + (occam?1:0));
        
    behavioral_score::iterator dst = bs.begin();
    for(combo::contin_output_table::const_iterator it1 = ct.begin(), 
            it2 = target.begin(); it1 != ct.end(); ++it1, ++it2) {
        *dst++ = sq(*it1 - *it2);
    }
    // add the Occam's razor feature
    if(occam)
        *dst = complexity(tr) * complexity_coef;

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger

    return bs;
}

void occam_contin_bscore::set_complexity_coef(double variance,
                                              double alphabet_size) {
    if(occam)
        complexity_coef = log((double)alphabet_size) * 2 * variance;
}

behavioral_score occam_truth_table_bscore::operator()(const combo_tree& tr) const
{
    truth_output_table ptt(tr, tt.input, rng);
    behavioral_score bs(tt.output.size() + (occam?1:0));
        
    behavioral_score::iterator dst = bs.begin();
    for(truth_output_table::const_iterator it1 = ptt.begin(), 
            it2 = tt.output.begin(); it1 != ptt.end(); ++it1, ++it2) {
        *dst++ = (*it1 == *it2 ? 0.0 : 1.0);
    }
    // add the Occam's razor feature
    if(occam)
        *dst = complexity(tr) * complexity_coef;

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger
    return bs;
}

} // ~namespace moses
} // ~namespace opencog
