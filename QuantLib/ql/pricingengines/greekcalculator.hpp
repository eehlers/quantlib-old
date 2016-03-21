#ifndef QUANTLIB_GREEKCALCULATOR_H
#define QUANTLIB_GREEKCALCULATOR_H

#include <ql/pricingengines/additionalresultcalculators.hpp>
#include <ql/quote.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/instruments/swaption.hpp>
#include <ql/quotes/simplequote.hpp>

namespace QuantLib {

    struct ShiftedQuote{

        ShiftedQuote(double shift, const boost::shared_ptr<SimpleQuote>& quote): shift_(shift), quote_(quote) {}

        double shift_;
        boost::shared_ptr<SimpleQuote> quote_;
    };

    void calculateDeltaAndGamma(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedQuote& shiftedQuote,
        const boost::shared_ptr<Instrument>& instrument
    ) ;

    struct ShiftedParameterCalibrator {
        ShiftedParameterCalibrator(
            const double shift,
            const std::vector<boost::shared_ptr<CalibrationHelper> > swaptionCalibrationHelpers,
            const std::vector<boost::shared_ptr<SimpleQuote> > volQuotes,
            const boost::shared_ptr<ShortRateModel>& model,
            const boost::shared_ptr<OptimizationMethod>& optimizer,
            const EndCriteria& endCriteria
        ): shift_(shift), swaptionCalibrationHelpers_(swaptionCalibrationHelpers),
           volQuotes_(volQuotes), model_(model), optimizer_(optimizer), endCriteria_(endCriteria) {}

        double shift_;
        std::vector<boost::shared_ptr<CalibrationHelper> > swaptionCalibrationHelpers_;
        std::vector<boost::shared_ptr<SimpleQuote> > volQuotes_;
        boost::shared_ptr<ShortRateModel> model_;
        boost::shared_ptr<OptimizationMethod> optimizer_;
        EndCriteria endCriteria_;
    };

    void calculateHullWhiteVega(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedParameterCalibrator& shiftedVols,
        const boost::shared_ptr<Instrument>& instrument
    );

    typedef std::map<std::string, ShiftedQuote> LabelledQuotes;
}


#endif //QUANTLIB_GREEKCALCULATOR_H
