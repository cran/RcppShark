// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::depends(BH)]]
/*!
 * 
 *
 * \brief       Basic types and definitions of the Rng component.
 * 
 * 
 *
 * \author      T. Voss
 * \date        2011-07-02
 *
 *
 * \par Copyright 1995-2017 Shark Development Team
 * 
 * <BR><HR>
 * This file is part of Shark.
 * <http://shark-ml.org/>
 * 
 * Shark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Shark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Shark.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef SHARK_RNG_RNG_H
#define SHARK_RNG_RNG_H

#include <shark/Rng/Runif.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/linear_congruential.hpp>

namespace shark {

	/** \brief Default RNG of the shark library. */
	typedef boost::rand47	DefaultRngType;

	/** \brief Fast RNG type. */
	typedef boost::rand48	FastRngType;
}

#endif 

