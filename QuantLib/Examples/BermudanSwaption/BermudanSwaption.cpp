/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*!
Copyright (C) 2002, 2003 Sadruddin Rejeb
Copyright (C) 2004 Ferdinando Ametrano
Copyright (C) 2005, 2006, 2007 StatPro Italia srl

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

#include <ql/quantlib.hpp>

#ifdef BOOST_MSVC
/* Uncomment the following lines to unmask floating-point
exceptions. Warning: unpredictable results can arise...

See http://www.wilmott.com/messageview.cfm?catid=10&threadid=9481
Is there anyone with a definitive word about this?
*/
// #include <float.h>
// namespace { unsigned int u = _controlfp(_EM_INEXACT, _MCW_EM); }
#endif

#include <boost/timer.hpp>
#include <iostream>
#include <iomanip>
#include <ql/pricingengines/treecumulativeprobabilitycalculator1d.hpp>
#include <ql/pricingengines/greekcalculator.hpp>

using namespace QuantLib;

#ifdef BOOST_MSVC
#  ifdef QL_ENABLE_THREAD_SAFE_OBSERVER_PATTERN
#    include <ql/auto_link.hpp>
#    define BOOST_LIB_NAME boost_system
#    include <boost/config/auto_link.hpp>
#    undef BOOST_LIB_NAME
#    define BOOST_LIB_NAME boost_thread
#    include <boost/config/auto_link.hpp>
#    undef BOOST_LIB_NAME
#  endif
#endif

#if defined(QL_ENABLE_SESSIONS)
namespace QuantLib {

    Integer sessionId() { return 0; }

}
#endif


//Number of swaptions to be calibrated to...

Size numRows = 5;
Size numCols = 5;

Size treeTimeStepCount = 50;

std::vector<Integer> swapLengths = {5,     4,     3,     2,     1 };
Volatility swaptionVols[] = {
    0.1490, 0.1340, 0.1228, 0.1189, 0.1148,
    0.1290, 0.1201, 0.1146, 0.1108, 0.1040,
    0.1149, 0.1112, 0.1070, 0.1010, 0.0957,
    0.1047, 0.1021, 0.0980, 0.0951, 0.1270,
    0.1000, 0.0950, 0.0900, 0.1230, 0.1160 };

namespace {

    void calibrateModel(
        const boost::shared_ptr<ShortRateModel>& model,
        const std::vector<boost::shared_ptr<CalibrationHelper> >& helpers,
        const boost::shared_ptr<OptimizationMethod>& optimizer,
        const EndCriteria& criteria
    ) {

        model->calibrate(helpers, *optimizer, criteria);

        // Output the implied Black volatilities
        for (Size i = 0; i<numRows; i++) {
            Size k = (i+1)*(numCols - 1);
            Real npv = helpers[i]->modelValue();
            Volatility implied = helpers[i]->impliedVolatility(npv, 1e-4,
                                                               1000, 0.05, 0.50);
            Volatility diff = implied - swaptionVols[k];

            std::cout << i + 1 << "x" << swapLengths[i]
            << std::setprecision(5) << std::noshowpos
            << ": model " << std::setw(7) << io::volatility(implied)
            << ", market " << std::setw(7)
            << io::volatility(swaptionVols[k])
            << " (" << std::setw(7) << std::showpos
            << io::volatility(diff) << std::noshowpos << ")\n";
        }
    }

    void printExerciseProbabilities(std::ostream& out, const Swaption& swaption) {
        typedef std::map<std::string, boost::any> result_map;
        typedef std::map<Date, std::pair<double, double> > dated_prob_boundaries;
        const result_map all_results = swaption.additionalResults();

        result_map::const_iterator result_ptr = all_results.find("ExerciseProbabilityAndSwapBoundary");
        if (result_ptr != all_results.end()) {
            const boost::shared_ptr<dated_prob_boundaries> datedProbBoundaries =
                boost::any_cast<boost::shared_ptr<dated_prob_boundaries> > (result_ptr->second);
            out << "Exercise Probability {date, (prob, rate)}" << std::endl;
            for (
                dated_prob_boundaries::const_iterator ptr = datedProbBoundaries->begin();
                ptr != datedProbBoundaries->end();
                ++ptr
                ) {
                std::pair<double, double> result = ptr->second;
                out << "{" << ptr->first << ", (" << result.first << ", " << result.second << ")" << "} " << std::endl;
            }
            out << std::endl;
        }
    }

    typedef std::map<std::string, boost::any> result_map;

    void printGreekValues(
        std::ostream& out,
        const boost::shared_ptr<Instrument>& swaption,
        const ShiftedQuote& priceAndBump,
        const ShiftedParameterCalibrator& shiftedVols
    ) {
        result_map all_results = swaption->additionalResults();
        calculateDeltaAndGamma(all_results, priceAndBump, swaption);
        calculateHullWhiteVega(all_results, shiftedVols, swaption);
        result_map::const_iterator delta_ptr = all_results.find("delta");
        result_map::const_iterator gamma_ptr = all_results.find("gamma");
        result_map::const_iterator vega_ptr = all_results.find("vega");
        //result_map::const_iterator parallel_vega_ptr = all_results.find("parallelvega");

        QL_ASSERT(delta_ptr != all_results.end(), "Delta calculation not found.");
        QL_ASSERT(vega_ptr != all_results.end(), "Vega calculation not found.");
        QL_ASSERT(gamma_ptr != all_results.end(), "Gamma calculation not found.");

        auto delta = boost::any_cast<double>(delta_ptr->second);
        auto vega = boost::any_cast<Array>(vega_ptr->second);
        auto gamma = boost::any_cast<double>(gamma_ptr->second);

        out << "{delta: " << delta << ",  gamma: " << gamma << "}" << std::endl;
        out << "vega: " << vega << std::endl;
    }

      inline std::vector<boost::shared_ptr<AdditionalResultCalculator> >
    buildResultCalculators(
    ) {
        boost::shared_ptr<AdditionalResultCalculator> treeProbCalculator = boost::make_shared<TreeCumulativeProbabilityCalculator1D>();
        std::vector<boost::shared_ptr<AdditionalResultCalculator> > result;
        result.push_back(treeProbCalculator);

        return result;
    }
}

int main(int, char*[]) {

    try {
        boost::timer timer;
        std::cout << std::endl;

        Date todaysDate(15, February, 2002);
        Calendar calendar = TARGET();
        Date settlementDate(19, February, 2002);
        Settings::instance().evaluationDate() = todaysDate;

        // flat yield term structure implying 1x5 swap at 5%
        boost::shared_ptr<SimpleQuote> flatRate(new SimpleQuote(0.04875825));
        Handle<YieldTermStructure> rhTermStructure(
            boost::shared_ptr<FlatForward>(
                new FlatForward(settlementDate, Handle<Quote>(flatRate),
                    Actual365Fixed())));

        // Define the ATM/OTM/ITM swaps
        Frequency fixedLegFrequency = Annual;
        BusinessDayConvention fixedLegConvention = Unadjusted;
        BusinessDayConvention floatingLegConvention = ModifiedFollowing;
        DayCounter fixedLegDayCounter = Thirty360(Thirty360::European);
        Frequency floatingLegFrequency = Semiannual;
        VanillaSwap::Type type = VanillaSwap::Receiver;
        Rate dummyFixedRate = 0.03;
        boost::shared_ptr<IborIndex> indexSixMonths(new
            Euribor6M(rhTermStructure));

        Date startDate = calendar.advance(settlementDate, 1, Years,
            floatingLegConvention);
        Date maturity = calendar.advance(startDate, 5, Years,
            floatingLegConvention);
        Schedule fixedSchedule(startDate, maturity, Period(fixedLegFrequency),
            calendar, fixedLegConvention, fixedLegConvention,
            DateGeneration::Forward, false);
        Schedule floatSchedule(startDate, maturity, Period(floatingLegFrequency),
            calendar, floatingLegConvention, floatingLegConvention,
            DateGeneration::Forward, false);

        Real nominal = 1000.0;
        Spread spread = 0.0;

        // defining the swaptions to be used in model calibration
        std::vector<Period> swaptionMaturities;
        swaptionMaturities.push_back(Period(1, Years));
        swaptionMaturities.push_back(Period(2, Years));
        swaptionMaturities.push_back(Period(3, Years));
        swaptionMaturities.push_back(Period(4, Years));
        swaptionMaturities.push_back(Period(5, Years));

        std::vector<boost::shared_ptr<CalibrationHelper> > swaptionCalibrationHelpers;

        // List of times that have to be included in the timegrid
        std::list<Time> times;

        std::vector<boost::shared_ptr<SimpleQuote> > calibrationVols(numRows);
        std::vector<Rate> volatilities(numRows);
        for (Size i = 0; i < calibrationVols.size(); ++i) {
            calibrationVols[i] = boost::shared_ptr<SimpleQuote>(new SimpleQuote(swaptionVols[(i+1)*(numCols - 1)]));
            volatilities[i] = swaptionVols[(i+1)*(numCols - 1)];
        }

        for (Size i = 0; i<numRows; i++) {
            swaptionCalibrationHelpers.push_back(boost::shared_ptr<CalibrationHelper>(new
                SwaptionHelper(swaptionMaturities[i],
                    Period(swapLengths[i], Years),// 1x5, 2x4, 3x3, 4x2, 5x1
                    Handle<Quote>(calibrationVols[i]),
                    indexSixMonths,
                    indexSixMonths->tenor(),
                    indexSixMonths->dayCounter(),
                    indexSixMonths->dayCounter(),
                    rhTermStructure)));
            swaptionCalibrationHelpers.back()->addTimesTo(times);
        }

        // Building time-grid
        TimeGrid grid(times.begin(), times.end(), 30);


        // defining the models
        boost::shared_ptr<G2> modelG2(new G2(rhTermStructure));
        boost::shared_ptr<HullWhite> modelHW(new HullWhite(rhTermStructure));
        boost::shared_ptr<HullWhite> modelHW2(new HullWhite(rhTermStructure));
        boost::shared_ptr<BlackKarasinski> modelBK(
            new BlackKarasinski(rhTermStructure));

        // model calibrations
        EndCriteria endCriteria(400, 100, 1.0e-8, 1.0e-8, 1.0e-8);
        boost::shared_ptr<LevenbergMarquardt> optimizer(new LevenbergMarquardt);
        {
            std::cout << "Hull-White (analytic formulae) calibration" << std::endl;
            for (Size i = 0; i < swaptionCalibrationHelpers.size(); i++) {
                swaptionCalibrationHelpers[i]->setPricingEngine(boost::shared_ptr<PricingEngine>(
                    new JamshidianSwaptionEngine(modelHW)));
            }
            calibrateModel(modelHW, swaptionCalibrationHelpers, optimizer, endCriteria);
            std::cout << "calibrated to:\n"
            << "a = " << modelHW->params()[0] << ", "
            << "sigma = " << modelHW->params()[1]
            << std::endl << std::endl;
        }


        {
            std::cout << "G2 (analytic formulae) calibration" << std::endl;
            for (Size i = 0; i < swaptionCalibrationHelpers.size(); i++) {
                swaptionCalibrationHelpers[i]->setPricingEngine(boost::shared_ptr<PricingEngine>(new G2SwaptionEngine(modelG2, 6.0, 16)));
            }
            calibrateModel(modelG2, swaptionCalibrationHelpers, optimizer, endCriteria);
            std::cout << "calibrated to:\n"
            << "a     = " << modelG2->params()[0] << ", "
            << "sigma = " << modelG2->params()[1] << "\n"
            << "b     = " << modelG2->params()[2] << ", "
            << "eta   = " << modelG2->params()[3] << "\n"
            << "rho   = " << modelG2->params()[4]
            << std::endl << std::endl;
        }

        {//set up other result requests
            std::cout << "Hull-White (numerical) calibration" << std::endl;
            for (Size i = 0; i < swaptionCalibrationHelpers.size(); i++)
                swaptionCalibrationHelpers[i]->setPricingEngine(boost::shared_ptr<PricingEngine>(
                    new TreeSwaptionEngine(modelHW2, grid)));

            calibrateModel(modelHW2, swaptionCalibrationHelpers, optimizer, endCriteria);
            std::cout << "calibrated to:\n"
                << "a = " << modelHW2->params()[0] << ", "
                << "sigma = " << modelHW2->params()[1]
                << std::endl << std::endl;
        }

        //set up other result requests
        {
            //set up other result requests
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator;
            std::cout << "Black-Karasinski (numerical) calibration" << std::endl;
            for (Size i = 0; i < swaptionCalibrationHelpers.size(); i++)
                swaptionCalibrationHelpers[i]->setPricingEngine(boost::shared_ptr<PricingEngine>(
                    new TreeSwaptionEngine(modelBK, grid, Handle<YieldTermStructure>(), resultCalculator)));

            calibrateModel(modelBK, swaptionCalibrationHelpers, optimizer, endCriteria);
            std::cout << "calibrated to:\n"
                << "a = " << modelBK->params()[0] << ", "
                << "sigma = " << modelBK->params()[1]
                << std::endl << std::endl;
        }

        std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator;

        boost::shared_ptr<VanillaSwap> swap(new VanillaSwap(
            type, nominal,
            fixedSchedule, dummyFixedRate, fixedLegDayCounter,
            floatSchedule, indexSixMonths, spread,
            indexSixMonths->dayCounter()));
        swap->setPricingEngine(boost::shared_ptr<PricingEngine>(
            new DiscountingSwapEngine(rhTermStructure)));

        Rate fixedATMRate = swap->fairRate();
        Rate fixedOTMRate = fixedATMRate * 1.2;
        Rate fixedITMRate = fixedATMRate * 0.8;

        // ATM Bermudan swaption pricing
        std::cout << "Payer bermudan swaption "
        << "struck at " << io::rate(fixedATMRate)
        << " (ATM)" << std::endl;

        boost::shared_ptr<VanillaSwap> atmSwap(new VanillaSwap(
            type, nominal,
            fixedSchedule, fixedATMRate, fixedLegDayCounter,
            floatSchedule, indexSixMonths, spread,
            indexSixMonths->dayCounter()));
        boost::shared_ptr<VanillaSwap> otmSwap(new VanillaSwap(
            type, nominal,
            fixedSchedule, fixedOTMRate, fixedLegDayCounter,
            floatSchedule, indexSixMonths, spread,
            indexSixMonths->dayCounter()));
        boost::shared_ptr<VanillaSwap> itmSwap(new VanillaSwap(
            type, nominal,
            fixedSchedule, fixedITMRate, fixedLegDayCounter,
            floatSchedule, indexSixMonths, spread,
            indexSixMonths->dayCounter()));

        std::vector<Date> bermudanDates;
        const std::vector<boost::shared_ptr<CashFlow> >& leg =
            swap->fixedLeg();
        for (Size i = 0; i<leg.size(); i++) {
            boost::shared_ptr<Coupon> coupon =
                boost::dynamic_pointer_cast<Coupon>(leg[i]);
            bermudanDates.push_back(coupon->accrualStartDate());
        }

        boost::shared_ptr<Exercise> bermudanExercise(
            new BermudanExercise(bermudanDates));

        boost::shared_ptr<Swaption> bermudanSwaption = boost::make_shared<Swaption>(atmSwap, bermudanExercise);
        //Adjust to your own taste.
        const double deltaBump = 0.001;
        ShiftedQuote bumpAndPrice(deltaBump, flatRate);
        const double vegaBump = 0.01;
        boost::shared_ptr<PricingEngine> jamshidianEngine(
        new JamshidianSwaptionEngine(modelHW));

        ShiftedParameterCalibrator bumpAndVols(vegaBump, swaptionCalibrationHelpers, jamshidianEngine,
                                               calibrationVols, modelHW, optimizer, endCriteria);

        // Do the pricing for each model

        {// G2 price the European swaption here, it should switch to bermudan
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelG2, treeTimeStepCount)));
            std::cout << "G2 (tree):      " << bermudanSwaption->NPV() << std::endl;
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdG2SwaptionEngine(modelG2)));
            std::cout << "G2 (fdm) :      " << bermudanSwaption->NPV() << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "HW (tree):      " << bermudanSwaption->NPV() << std::endl;
            //printExerciseProbabilities(std::cout, *bermudanSwaption);
            printGreekValues(std::cout, bermudanSwaption, bumpAndPrice, bumpAndVols);

            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW)));
            std::cout << "HW (fdm) :      " << bermudanSwaption->NPV() << std::endl;
        }
        {
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW2, treeTimeStepCount)));
            std::cout << "HW (num, tree): " << bermudanSwaption->NPV() << std::endl;
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW2)));
            std::cout << "HW (num, fdm) : " << bermudanSwaption->NPV() << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();
            bermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelBK, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "BK:             " << bermudanSwaption->NPV() << std::endl;
            //printExerciseProbabilities(std::cout, *bermudanSwaption);
            //printGreekValues(std::cout, *bermudanSwaption);
        }

        // OTM Bermudan swaption pricing

        std::cout << "Payer bermudan swaption "
            << "struck at " << io::rate(fixedOTMRate)
            << " (OTM)" << std::endl;

        boost::shared_ptr<Swaption> otmBermudanSwaption = boost::make_shared<Swaption>(otmSwap, bermudanExercise);

        //Adjust to your own taste.
        ShiftedQuote otmBumpAndPrice(deltaBump, flatRate);
        // Do the pricing for each model
        {
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelG2, treeTimeStepCount)));
            std::cout << "G2 (tree):       " << otmBermudanSwaption->NPV()
                << std::endl;
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdG2SwaptionEngine(modelG2)));
            std::cout << "G2 (fdm) :       " << otmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();

            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "HW (tree):       " << otmBermudanSwaption->NPV()
                << std::endl;
            //printExerciseProbabilities(std::cout, *otmBermudanSwaption);
            printGreekValues(std::cout, otmBermudanSwaption, otmBumpAndPrice, bumpAndVols);
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW)));
            std::cout << "HW (fdm) :       " << otmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW2, treeTimeStepCount)));
            std::cout << "HW (num, tree):  " << otmBermudanSwaption->NPV()
                << std::endl;
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW2)));
            std::cout << "HW (num, fdm):   " << otmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();
            otmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelBK, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "BK:              " << otmBermudanSwaption->NPV()
                << std::endl;
            //printExerciseProbabilities(std::cout, *otmBermudanSwaption);
            //printGreekValues(std::cout, *otmBermudanSwaption);
        }

        // ITM Bermudan swaption pricing

        std::cout << "Payer bermudan swaption "
            << "struck at " << io::rate(fixedITMRate)
            << " (ITM)" << std::endl;

        boost::shared_ptr<Swaption> itmBermudanSwaption = boost::make_shared<Swaption>(itmSwap, bermudanExercise);

        //Adjust to your own taste.
        ShiftedQuote itmBumpAndPrice(deltaBump, flatRate);
        // Do the pricing for each model
        {
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelG2, treeTimeStepCount, rhTermStructure)));
            std::cout << "G2 (tree):       " << itmBermudanSwaption->NPV()
                << std::endl;
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdG2SwaptionEngine(modelG2)));
            std::cout << "G2 (fdm) :       " << itmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "HW (tree):       " << itmBermudanSwaption->NPV()
                << std::endl;
            //printExerciseProbabilities(std::cout, *itmBermudanSwaption);
            printGreekValues(std::cout, itmBermudanSwaption, itmBumpAndPrice, bumpAndVols);
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW)));
            std::cout << "HW (fdm) :       " << itmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelHW2, treeTimeStepCount, rhTermStructure)));
            std::cout << "HW (num, tree):  " << itmBermudanSwaption->NPV()
                << std::endl;
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new FdHullWhiteSwaptionEngine(modelHW2)));
            std::cout << "HW (num, fdm) :  " << itmBermudanSwaption->NPV()
                << std::endl;
        }
        {
            std::vector<boost::shared_ptr<AdditionalResultCalculator> > resultCalculator = buildResultCalculators();
            itmBermudanSwaption->setPricingEngine(boost::shared_ptr<PricingEngine>(
                new TreeSwaptionEngine(modelBK, treeTimeStepCount, rhTermStructure, resultCalculator)));
            std::cout << "BK:              " << itmBermudanSwaption->NPV()
                << std::endl;
            //printExerciseProbabilities(std::cout, *itmBermudanSwaption);
            //printGreekValues(std::cout, *itmBermudanSwaption);
        }
        double seconds = timer.elapsed();
        Integer hours = int(seconds / 3600);
        seconds -= hours * 3600;
        Integer minutes = int(seconds / 60);
        seconds -= minutes * 60;
        std::cout << " \nRun completed in ";
        if (hours > 0)
            std::cout << hours << " h ";
        if (hours > 0 || minutes > 0)
            std::cout << minutes << " m ";
        std::cout << std::fixed << std::setprecision(0)
            << seconds << " s\n" << std::endl;

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}

