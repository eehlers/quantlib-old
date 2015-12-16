/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2007 Ferdinando Ametrano
 Copyright (C) 2005 Plamen Neykov
 Copyright (C) 2004, 2005, 2006 Eric Ehlers

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

//#if defined(HAVE_CONFIG_H)     // Dynamically created by configure
//    #include <qlo/config.hpp>
//#endif
//#include <qlo/utilities.hpp>
#include <qlo/objmanual_utilities.hpp>
//#include <qlo/qladdindefines.hpp>
#include <ql/version.hpp>
#include <rp/repository.hpp>
#include <qlo/serialization/serializationfactory.hpp>

#include <qlo/obj_vanillaswaps.hpp>//DELETEME
#include <ql/cashflows/coupon.hpp>//DELETEME

#include <map>
#include <boost/any.hpp>

//#if defined BOOST_MSVC       // Microsoft Visual C++
//#  define BOOST_LIB_DIAGNOSTIC
//#  include <ql/auto_link.hpp>
//#  undef BOOST_LIB_DIAGNOSTIC
//#  include <qlo/vcconfig.hpp>
//#else
//#  define COMPILER_STRING
//#endif

//#define VERSION_STRING "QuantLibXL " QLADDIN_VERSION
//#define VERSION_STRING_VERBOSE "QuantLibXL " QLADDIN_VERSION COMPILER_STRING " - " __DATE__ " " __TIME__

std::string QuantLibAddin::version() {
    return QL_VERSION;
}

//    std::string qlAddinVersion() {
//        return QLADDIN_VERSION;
//    }

//    std::string qlxlVersion(bool verbose) {
//        if (verbose)
//            return VERSION_STRING_VERBOSE;
//        else
//            return VERSION_STRING;
//    }

long QuantLibAddin::ObjectCount() {
    return reposit::Repository::instance().objectCount();
}

std::vector<QuantLib::Date> QuantLibAddin::swapFixedLegAccrualStartDates(const std::string &objectID) {
    RP_GET_REFERENCE(swap, objectID, QuantLibAddin::VanillaSwap, QuantLib::VanillaSwap);
    std::vector<QuantLib::Date> ret;
    const std::vector<boost::shared_ptr<QuantLib::CashFlow> >& leg =
        swap->fixedLeg();
    for (unsigned int i=0; i<leg.size(); i++) {
        boost::shared_ptr<QuantLib::Coupon> coupon =
            boost::dynamic_pointer_cast<QuantLib::Coupon>(leg[i]);
        ret.push_back(coupon->accrualStartDate());
    }
    return ret;
}

QuantLib::Date QuantLibAddin::swapFixedLegLastAccrualStartDate(const std::string &objectID) {
    RP_GET_REFERENCE(swap, objectID, QuantLibAddin::VanillaSwap, QuantLib::VanillaSwap);
    QuantLib::Date ret;
    const std::vector<boost::shared_ptr<QuantLib::CashFlow> >& leg =
        swap->fixedLeg();
    for (unsigned int i=0; i<leg.size(); i++) {
        boost::shared_ptr<QuantLib::Coupon> coupon =
            boost::dynamic_pointer_cast<QuantLib::Coupon>(leg[i]);
        if (coupon->accrualStartDate() > ret)
            ret = coupon->accrualStartDate();
    }
    return ret;
}

std::vector<QuantLib::Date> QuantLibAddin::swaptionExerciseDates(const std::string &objectID) {
    RP_GET_REFERENCE(swaption, objectID, QuantLibAddin::Swaption, QuantLib::Swaption);
    std::vector<QuantLib::Date> ret;

    typedef std::map<std::string, boost::any> result_map;
    typedef std::map<Date, std::pair<double, double> > dated_prob_boundaries;
    const result_map allResults = swaption->additionalResults();
    
    result_map::const_iterator result_ptr = allResults.find("ExerciseProbabilityAndSwapBoundary");
    if (result_ptr != allResults.end()) {
        const boost::shared_ptr<dated_prob_boundaries> datedProbBoundaries = 
            boost::any_cast<boost::shared_ptr<dated_prob_boundaries> > (result_ptr->second);
        for (
            dated_prob_boundaries::const_iterator ptr = datedProbBoundaries->begin();
            ptr != datedProbBoundaries->end();
            ++ptr
        ) {
            ret.push_back(ptr->first);
        }
    }
    return ret;
}

std::vector<double> QuantLibAddin::swaptionExerciseProbabilities(const std::string &objectID) {
    RP_GET_REFERENCE(swaption, objectID, QuantLibAddin::Swaption, QuantLib::Swaption);
    std::vector<double> ret;

    typedef std::map<std::string, boost::any> result_map;
    typedef std::map<Date, std::pair<double, double> > dated_prob_boundaries;
    const result_map allResults = swaption->additionalResults();
    
    result_map::const_iterator result_ptr = allResults.find("ExerciseProbabilityAndSwapBoundary");
    if (result_ptr != allResults.end()) {
        const boost::shared_ptr<dated_prob_boundaries> datedProbBoundaries = 
            boost::any_cast<boost::shared_ptr<dated_prob_boundaries> > (result_ptr->second);
        for (
            dated_prob_boundaries::const_iterator ptr = datedProbBoundaries->begin();
            ptr != datedProbBoundaries->end();
            ++ptr
        ) {
            std::pair<double, double> result = ptr->second;
            ret.push_back(result.first);
        }
    }
    return ret;
}

std::vector<double> QuantLibAddin::swaptionExerciseRates(const std::string &objectID) {
    RP_GET_REFERENCE(swaption, objectID, QuantLibAddin::Swaption, QuantLib::Swaption);
    std::vector<double> ret;

    typedef std::map<std::string, boost::any> result_map;
    typedef std::map<Date, std::pair<double, double> > dated_prob_boundaries;
    const result_map allResults = swaption->additionalResults();
    
    result_map::const_iterator result_ptr = allResults.find("ExerciseProbabilityAndSwapBoundary");
    if (result_ptr != allResults.end()) {
        const boost::shared_ptr<dated_prob_boundaries> datedProbBoundaries = 
            boost::any_cast<boost::shared_ptr<dated_prob_boundaries> > (result_ptr->second);
        for (
            dated_prob_boundaries::const_iterator ptr = datedProbBoundaries->begin();
            ptr != datedProbBoundaries->end();
            ++ptr
        ) {
            std::pair<double, double> result = ptr->second;
            ret.push_back(result.second);
        }
    }
    return ret;
}
