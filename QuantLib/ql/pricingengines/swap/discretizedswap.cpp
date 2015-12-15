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
#include <iostream>
#include <ql/pricingengines/swap/discretizedswap.hpp>

namespace QuantLib {

    DiscretizedFloatingCashflowStructure::DiscretizedFloatingCashflowStructure(const VanillaSwap::arguments& args,
        const Date& referenceDate,
        const DayCounter& dayCounter,
        const Date entryDate)
        : arguments_(args) {

        floatingResetTimes_.resize(args.floatingResetDates.size());
        for (Size i = 0; i < floatingResetTimes_.size(); ++i) {
            if (entryDate <= args.floatingResetDates[i]) {
                floatingResetTimes_[i] =
                    dayCounter.yearFraction(referenceDate,
                        args.floatingResetDates[i]);
            }
        }

        floatingPayTimes_.resize(args.floatingPayDates.size());
        for (Size i = 0; i < floatingPayTimes_.size(); ++i) {
            if (entryDate <= args.floatingResetDates[i]) {
                floatingPayTimes_[i] =
                    dayCounter.yearFraction(referenceDate,
                        args.floatingPayDates[i]);
            }
        }
    }

    void DiscretizedFloatingCashflowStructure::reset(Size size) {
        values_ = Array(size, 0.0);
        adjustValues();
    }

    std::vector<Time> DiscretizedFloatingCashflowStructure::mandatoryTimes() const {
        std::vector<Time> times;
        
        for (Size i = 0; i<floatingResetTimes_.size(); i++) {
            Time t = floatingResetTimes_[i];
            if (t >= 0.0)
                times.push_back(t);
        }
        for (Size i = 0; i<floatingPayTimes_.size(); i++) {
            Time t = floatingPayTimes_[i];
            if (t >= 0.0)
                times.push_back(t);
        }
        return times;
    }

    void DiscretizedFloatingCashflowStructure::preAdjustValuesImpl() {
        
        for (Size i = 0; i<floatingResetTimes_.size(); i++) {
            Time t = floatingResetTimes_[i];
            if (t >= 0.0 && isOnTime(t)) {
                DiscretizedDiscountBond bond;
                bond.initialize(method(), floatingPayTimes_[i]);
                bond.rollback(time_);

                Real nominal = arguments_.nominal;
                Time T = arguments_.floatingAccrualTimes[i];
                Spread spread = arguments_.floatingSpreads[i];
                Real accruedSpread = nominal*T*spread;
                for (Size j = 0; j<values_.size(); j++) {
                    Real coupon = nominal * (1.0 - bond.values()[j])
                        + accruedSpread * bond.values()[j];
                    values_[j] += coupon;
                }
            }
        }
        
    }

    void DiscretizedFloatingCashflowStructure::postAdjustValuesImpl() {
        for (Size i = 0; i<floatingPayTimes_.size(); i++) {
            Time t = floatingPayTimes_[i];
            Time reset = floatingResetTimes_[i];
            if (t >= 0.0 && isOnTime(t) && reset < 0.0) {
                Real currentFloatingCoupon = arguments_.floatingCoupons[i];
                QL_REQUIRE(currentFloatingCoupon != Null<Real>(),
                    "current floating coupon not given");
                values_ += currentFloatingCoupon;
            }
        }
    }

    DiscretizedFixedCashflowStructure::DiscretizedFixedCashflowStructure(const VanillaSwap::arguments& args,
        const Date& referenceDate,
        const DayCounter& dayCounter,
        const Date entryDate)
        : arguments_(args) {
        fixedRate_ = args.fixedRate;
        fixedResetTimes_.resize(args.fixedResetDates.size());
        for (Size i = 0; i < fixedResetTimes_.size(); ++i) {
            if (entryDate <= args.fixedResetDates[i]) {
                fixedResetTimes_[i] =
                    dayCounter.yearFraction(referenceDate,
                        args.fixedResetDates[i]);
            }
        }

        fixedPayTimes_.resize(args.fixedPayDates.size());
        for (Size i = 0; i < fixedPayTimes_.size(); ++i) {
            if (entryDate <= args.fixedResetDates[i]) {
                fixedPayTimes_[i] =
                    dayCounter.yearFraction(referenceDate,
                        args.fixedPayDates[i]);
            }
        }
    }

    void DiscretizedFixedCashflowStructure::reset(Size size) {
        values_= Array(size, 0.0);
        adjustValues();
    }

    std::vector<Time> DiscretizedFixedCashflowStructure::mandatoryTimes() const {
        std::vector<Time> times;
        for (Size i = 0; i<fixedResetTimes_.size(); i++) {
            Time t = fixedResetTimes_[i];
            if (t >= 0.0)
                times.push_back(t);
        }
        for (Size i = 0; i<fixedPayTimes_.size(); i++) {
            Time t = fixedPayTimes_[i];
            if (t >= 0.0)
                times.push_back(t);
        }
        return times;
    }

    void DiscretizedFixedCashflowStructure::preAdjustValuesImpl() {

        for (Size i = 0; i<fixedResetTimes_.size(); i++) {
            Time t = fixedResetTimes_[i];
            if (t >= 0.0 && isOnTime(t)) {
                DiscretizedDiscountBond bond;
                bond.initialize(method(), fixedPayTimes_[i]);
                bond.rollback(time_);

                Real fixedCoupon = arguments_.fixedCoupons[i];
                for (Size j = 0; j<values_.size(); j++) {
                    Real coupon = fixedCoupon*bond.values()[j];
                    values_[j] += coupon;
                }
            }
        }
    }

    void DiscretizedFixedCashflowStructure::postAdjustValuesImpl() {
        // fixed coupons whose reset time is in the past won't be managed
        // in preAdjustValues()
        for (Size i = 0; i<fixedPayTimes_.size(); i++) {
            Time t = fixedPayTimes_[i];
            Time reset = fixedResetTimes_[i];
            if (t >= 0.0 && isOnTime(t) && reset < 0.0) {
                Real fixedCoupon = arguments_.fixedCoupons[i];
                values_ += fixedCoupon;
            }
        }
    }

    Real DiscretizedFixedCashflowStructure::fixedRate() const {
        return fixedRate_;
    }

    Real DiscretizedSwap::impliedSwapRate(Time t, Integer stateId) const {
        return floatingStructure_.values()[stateId] * fixedStructure_.fixedRate()/ fixedStructure_.values()[stateId];
    }

    DiscretizedSwap::DiscretizedSwap(const VanillaSwap::arguments& args,
                                     const Date& referenceDate,
                                     const DayCounter& dayCounter,
                                     const Date entryDate)
    : arguments_(args), floatingStructure_(args, referenceDate, dayCounter, entryDate), fixedStructure_(args, referenceDate, dayCounter, entryDate) {

    }

    void DiscretizedSwap::reset(Size size) {
        values_ = Array(size, 0.0);
        fixedStructure_.initialize(method(), time());
        floatingStructure_.initialize(method(), time());
        adjustValues();
    }

    std::vector<Time> DiscretizedSwap::mandatoryTimes() const {
        
        std::vector<Time> times = fixedStructure_.mandatoryTimes();
        std::vector<Time> floatingTimes = floatingStructure_.mandatoryTimes();
        times.insert(times.end(), floatingTimes.begin(), floatingTimes.end());

        return times;
    }

    void DiscretizedSwap::preAdjustValuesImpl() {
        // floating payments
        floatingStructure_.partialRollback(time());
        floatingStructure_.preAdjustValuesImpl();
        fixedStructure_.partialRollback(time());
        fixedStructure_.preAdjustValuesImpl();
        values_ = floatingStructure_.values() - fixedStructure_.values();
        
        if (arguments_.type == VanillaSwap::Receiver) {
            values_ *= -1.0;
        } 
    }

    void DiscretizedSwap::postAdjustValuesImpl() {
        floatingStructure_.postAdjustValuesImpl();
        fixedStructure_.postAdjustValuesImpl();
        values_ = floatingStructure_.values() - fixedStructure_.values();

        if (arguments_.type == VanillaSwap::Receiver) {
            values_ *= -1.0;
        }
    }

}
