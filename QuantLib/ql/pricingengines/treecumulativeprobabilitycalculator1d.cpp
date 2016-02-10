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

#include <stack>
#include <boost/shared_ptr.hpp>
#include <ql/pricingengines/treecumulativeprobabilitycalculator1d.hpp> 
#include <ql/pricingengines/swaption/discretizedswaption.hpp>

namespace QuantLib{

	void TreeCumulativeProbabilityCalculator1D::setupDiscretizedAsset(const boost::shared_ptr<DiscretizedAsset>& asset) {
		const boost::shared_ptr<DiscretizedSwaption> swaption = boost::dynamic_pointer_cast<DiscretizedSwaption>(asset);
		if (swaption) {
			exerciseTimes_ = swaption->positiveExerciseTimes();
			setExerciseIndex(swaption->exerciseIndex());
			std::vector<Date> tempExercised = swaption->exerciseDates();
			int negativeDateCount = static_cast<int>(tempExercised.size()) - exerciseTimes_.size();

			QL_REQUIRE(negativeDateCount >= 0, "Exercise dates definition error.  The number of past exercise days should be positive by construction.");
			exerciseDates_.resize(tempExercised.size());
			for (int i = tempExercised.size() - 1; i >= negativeDateCount; --i) {
				exerciseDates_[i] = tempExercised[i - negativeDateCount];
			}
		}
	}

    void TreeCumulativeProbabilityCalculator1D::setExerciseIndex(const boost::shared_ptr<std::vector<std::pair<bool, std::pair<Size, Real> > > >& exerciseIndex) {
        exerciseIndex_ = exerciseIndex;
    }

	void TreeCumulativeProbabilityCalculator1D::calculateAdditionalResults() {
		calculating_ = true;
        computeCumulativeProbabilities();
		boost::shared_ptr<std::map<Date, std::pair<double, double> > > result(new std::map<Date, std::pair<double, double> >);
		
		for (size_t exerciseTimeId = 0; exerciseTimeId < exerciseTimes_.size(); ++exerciseTimeId) {
			(*result)[exerciseDates_[exerciseTimeId]] = exerciseProbability(exerciseTimeId);
		}

		additionalResults_["ExerciseProbabilityAndSwapBoundary"] = result;
		calculating_ = false;
	}

	void TreeCumulativeProbabilityCalculator1D::setTree(const boost::shared_ptr<OneFactorModel::ShortRateTree>& tree) {
		tree_ = tree;
	}

	boost::shared_ptr<OneFactorModel::ShortRateTree> TreeCumulativeProbabilityCalculator1D::tree() const {
		return tree_;
	}
	//This tree constructs the total probability, for each exercise date, that the option remains unexercised.
	void TreeCumulativeProbabilityCalculator1D::computeCumulativeProbabilities() {
        //exercise events were collected back to front.
        std::reverse(exerciseIndex_->begin(), exerciseIndex_->end());

		const TimeGrid& times = tree_->timeGrid();
		std::stack<std::pair<Integer, Size> > exerciseTimesAndLevels;

        //FIXME - refactor to go the right way and pick a list instead of a stack to accumulate exercise event data
        size_t lastExerciseId = exerciseIndex_->size() - 1;
        for (Integer exerciseId = static_cast<int>(exerciseTimes_.size()) - 1; exerciseId >= 0; --exerciseId) {
            std::pair<bool, std::pair<Size, Real> > exerciseLevel = (*exerciseIndex_)[exerciseId];
			if (exerciseLevel.first == true) {
                exerciseTimesAndLevels.push(
                    std::make_pair(static_cast<int>(times.index(exerciseTimes_[exerciseId])), exerciseLevel.second.first));
			}
		}
		
        std::pair<Integer, Size> exerciseTimeAndLevel = std::make_pair(-1, 0);
        if (!exerciseTimesAndLevels.empty()) {
            exerciseTimeAndLevel = exerciseTimesAndLevels.top(); exerciseTimesAndLevels.pop();
        }

        cumulativeProbs_.clear();
		cumulativeProbs_.push_back(std::vector<std::pair<Real, Real> >(1, std::make_pair(0.0, 1.0)));
		std::vector<Size> exerciseLimits(times.size());

        exerciseLimits[0] = 1;
		for (Integer timePt = 1; timePt < static_cast<Integer>(times.size()); ++timePt) {
			Size priceIdCount = tree_->size(timePt);
			if (timePt == exerciseTimeAndLevel.first) {
			priceIdCount = exerciseTimeAndLevel.second + 1;
				if (!exerciseTimesAndLevels.empty()) {
                    exerciseTimeAndLevel = exerciseTimesAndLevels.top(); exerciseTimesAndLevels.pop();
				}
			}
			exerciseLimits[timePt] = priceIdCount;
			cumulativeProbs_.push_back(std::vector<std::pair<Real, Real> >(priceIdCount, std::make_pair(0.0, 0.0)));
		}
		for (Size timePt = 0; timePt < times.size() - 1; ++timePt) {
			Size priceIdCount = exerciseLimits[timePt];
            //On each timestep, we are aggregating calculated probabilities over the vector of states
            //for that timestep and calculating marginal probabilities for the next timestep.
            cumulativeProbs_[timePt][0].first = cumulativeProbs_[timePt][0].second;
            std::pair<Real, Real> parentProb = cumulativeProbs_[timePt][0];
            for (Size nodeId = 0; nodeId < tree_->size(1); ++nodeId) {
                if (tree_->descendant(timePt, 0, nodeId) < exerciseLimits[timePt + 1]) {
                    double increment = tree_->probability(timePt, 0, nodeId)*parentProb.second;
                    std::pair<Real, Real>& probabilityNode = cumulativeProbs_[timePt + 1][tree_->descendant(timePt, 0, nodeId)];
                    probabilityNode.second += increment;
                }
            }
			for (Size priceId = 1; priceId < priceIdCount; ++priceId) {
				cumulativeProbs_[timePt][priceId].first = cumulativeProbs_[timePt][priceId].second + cumulativeProbs_[timePt][priceId-1].first;
				std::pair<Real, Real> parentProb = cumulativeProbs_[timePt][priceId];
				for (Size nodeId = 0; nodeId < tree_->size(1); ++nodeId) {
                    if (tree_->descendant(timePt, priceId, nodeId) < exerciseLimits[timePt + 1]) {
                        double increment = tree_->probability(timePt, priceId, nodeId)*parentProb.second;
                        std::pair<Real, Real>& probabilityNode = cumulativeProbs_[timePt + 1][tree_->descendant(timePt, priceId, nodeId)];
                        probabilityNode.second += increment;
                    }
				}
			}
		}
        //accumulate cumulative probabilities on the last timestep.
        Size priceIdCount = exerciseLimits[times.size() - 1];
        cumulativeProbs_[times.size() - 1][0].first = cumulativeProbs_[times.size() - 1][0].second;
        for (Size priceId = 1; priceId < priceIdCount; ++priceId) {
            cumulativeProbs_[times.size() - 1][priceId].first = cumulativeProbs_[times.size() - 1][priceId].second + cumulativeProbs_[times.size() - 1][priceId -1].first;
        }
	}

	std::pair<Real, Real> TreeCumulativeProbabilityCalculator1D::exerciseProbability(Size exerciseTimeId) {
		QL_REQUIRE(!exerciseIndex_->empty(), "Index empty.  Must run pricing call before exerciseProbability().");
		
        const std::pair<bool, std::pair<Size, Real> >& exercise = (*exerciseIndex_)[exerciseTimeId];
		if (exercise.first == true) {
            Size exerciseDateId = tree_->timeGrid().index(exerciseTimes_[exerciseTimeId]);
            return std::make_pair(1.0 - cumulativeProbs_[exerciseDateId][exercise.second.first].first, exercise.second.second);
		}
		else {
			return std::make_pair(0.0, std::numeric_limits<double>::quiet_NaN());
		}
	}
}
