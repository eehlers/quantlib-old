/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb
Copyright (C) 2005 StatPro Italia srl

This file is part of QuantLib, a free-software/open-source library
for financial quantitative analysts and developers - http://quantlib.org/

QuantLib is free software: you can redistribute it and/or modify it
under the terms of the QuantLib license.  You should have received a
copy of the license along with this program; if not, please email
<quantlib-dev@lists.sf.net>. The license is also available online at
<http://quantlib.org/license.shtml>.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file quantlibtreecumulativeprobabilitycalculator.hpp
\brief Abstract one-factor interest rate model class
*/
#ifndef quantlib_tree_cumulative_probability_calculator_hpp
#define quantlib_tree_cumulative_probability_calculator_hpp

#include <ql/pricingengines/additionalresultcalculators.hpp>
#include <ql/models/shortrate/onefactormodel.hpp>
#include <ql/instruments/vanillaswap.hpp>


namespace QuantLib {

	struct TreeCumulativeProbabilityCalculator1D : AdditionalResultCalculator {

        void setExerciseIndex(const boost::shared_ptr<std::vector<std::pair<bool, std::pair<Size, Real> > > >& exerciseIndex);

		void calculateAdditionalResults();

		void setTree(const boost::shared_ptr<OneFactorModel::ShortRateTree>& tree);

		boost::shared_ptr<OneFactorModel::ShortRateTree> tree() const;

		void setupDiscretizedAsset(const boost::shared_ptr<DiscretizedAsset>&);
	private:
		std::pair<Real, Real> exerciseProbability(Size exerciseTimeId);
		void computeCumulativeProbabilities();
		boost::shared_ptr<OneFactorModel::ShortRateTree> tree_;
		boost::shared_ptr<std::vector<std::pair<bool, std::pair<Size, Real> > > > exerciseIndex_;
		std::vector<Time> exerciseTimes_;   //future exercise dates
		std::vector<Date> exerciseDates_;   //all exercise dates, future and past.
		std::vector<std::vector<std::pair<Real, Real> > > cumulativeProbs_;
		Date referenceDate_;
		VanillaSwap::Type swapType_;
	};
}
#endif
