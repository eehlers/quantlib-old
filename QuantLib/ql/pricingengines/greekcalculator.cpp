//
// Created by Etuka Onono on 07/02/2016.
//

#include <ql/pricingengines/greekcalculator.hpp>
#include <ql/discretizedasset.hpp>
#include <iostream>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>


namespace QuantLib {

    void calculateDeltaAndGamma(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedQuote& shiftedQuote,
        const boost::shared_ptr<Instrument>& instrument
    ) {
        double pv = instrument->NPV();
        //delta

        double rate = shiftedQuote.quote_->value();

        double rateUp = rate + shiftedQuote.shift_;

        shiftedQuote.quote_->setValue(rateUp);
        double pvUp = instrument->NPV();

        double rateDown = rate - shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateDown);
        double pvDown = instrument->NPV();

        double delta = (pvUp - pvDown) / (2.*shiftedQuote.shift_);
        additionalResults["delta"] = delta;

        //gamma
        rateUp = rateUp + shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateUp);
        pvUp = instrument->NPV();

        rateDown = rateDown - shiftedQuote.shift_;
        shiftedQuote.quote_->setValue(rateDown);
        pvDown = instrument->NPV();

        double gamma = (pvUp + 2.* pv- pvDown) / (4. * shiftedQuote.shift_ * shiftedQuote.shift_);

        additionalResults["gamma"] = gamma;
        //tidy up by setting things as they were.
        shiftedQuote.quote_->setValue(rate);
    }

    void calculateHullWhiteVega(
        std::map<std::string, boost::any>& additionalResults,
        const ShiftedParameterCalibrator& shiftedVols,
        const boost::shared_ptr<Instrument>& instrument
    ) {
        Array vega(shiftedVols.volQuotes_.size());
        for(size_t i = 0; i < shiftedVols.volQuotes_.size(); ++i) {
            SwaptionHelper& helper = *(boost::dynamic_pointer_cast<SwaptionHelper>(shiftedVols.helpers_[i]));
            helper.setPricingEngine(shiftedVols.engine_);

            boost::shared_ptr<SimpleQuote> volQuote = shiftedVols.volQuotes_[i];
            double vol = volQuote->value();
            double volUp = vol + shiftedVols.shift_ * vol;
            volQuote->setValue(volUp);

            shiftedVols.model_->calibrate(shiftedVols.helpers_,
                                          *shiftedVols.optimizer_, shiftedVols.endCriteria_);
            shiftedVols.model_->update();

            double pvUp = instrument->NPV();

            double volDown = vol - shiftedVols.shift_ * vol;
            volQuote->setValue(volDown);

            shiftedVols.model_->calibrate(shiftedVols.helpers_,
                                          *shiftedVols.optimizer_, shiftedVols.endCriteria_);
            shiftedVols.model_->update();

            double pvDown = instrument->NPV();

            vega[i] = (pvUp - pvDown) / (2. * shiftedVols.shift_);

            //tidy up by setting vol to original value
            volQuote->setValue(vol);
            shiftedVols.model_->calibrate(shiftedVols.helpers_,
                                          *shiftedVols.optimizer_, shiftedVols.endCriteria_);
            shiftedVols.model_->update();
        }

        additionalResults["vega"] = vega;
    }

}