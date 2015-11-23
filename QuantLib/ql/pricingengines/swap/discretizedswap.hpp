/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2001, 2002, 2003 Sadruddin Rejeb
 Copyright (C) 2004, 2007 StatPro Italia srl

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

/*! \file discretizedswap.hpp
    \brief Discretized swap class
*/

#ifndef quantlib_discretized_swap_hpp
#define quantlib_discretized_swap_hpp

#include <ql/instruments/vanillaswap.hpp>
#include <ql/discretizedasset.hpp>

namespace QuantLib {

    class DiscretizedFloatingCashflowStructure : public DiscretizedAsset {
        friend class DiscretizedSwap;
        DiscretizedFloatingCashflowStructure(const VanillaSwap::arguments&,
            const Date& referenceDate,
            const DayCounter& dayCounter, 
            const Date entryDate = Date());

        void reset(Size size);
        std::vector<Time> mandatoryTimes() const;
    protected:
        void preAdjustValuesImpl(Time entryTime = 0);
        void postAdjustValuesImpl(Time = 0);
    private:
        VanillaSwap::arguments arguments_;
        std::vector<Time> floatingResetTimes_;
        std::vector<Time> floatingPayTimes_;
    };

    class DiscretizedFixedCashflowStructure : public DiscretizedAsset {
        friend class DiscretizedSwap;
        DiscretizedFixedCashflowStructure(const VanillaSwap::arguments&,
            const Date& referenceDate,
            const DayCounter& dayCounter, 
            const Date entryDate = Date());

        void reset(Size size);
        std::vector<Time> mandatoryTimes() const;
    protected:
        void preAdjustValuesImpl(Time entryTime = 0);
        void postAdjustValuesImpl(Time = 0);
    private:
        VanillaSwap::arguments arguments_;
        std::vector<Time> fixedResetTimes_;
        std::vector<Time> fixedPayTimes_;
    };

    class DiscretizedSwap : public DiscretizedAsset {
        friend class DiscretizedCoterminalSwapStrip;
      public:
        DiscretizedSwap(const VanillaSwap::arguments&,
                        const Date& referenceDate,
                        const DayCounter& dayCounter, const Date entryDate = Date());
        Real impliedSwapRate(Time t, Integer stateId) const;
        void reset(Size size);
        std::vector<Time> mandatoryTimes() const;
      protected:
        void preAdjustValuesImpl(Time entryTime  = 0);
        void postAdjustValuesImpl(Time = 0);
      private:
        VanillaSwap::arguments arguments_;
        DiscretizedFloatingCashflowStructure floatingStructure_;
        DiscretizedFixedCashflowStructure fixedStructure_;
    };

}


#endif

