/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2000, 2001, 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2008 StatPro Italia srl

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

/*! \file multipleregimeinterpolation.hpp
    \brief linear interpolation between discrete points
*/

#ifndef quantlib_multiple_regime_interpolation_hpp
#define quantlib_multiple_regime_interpolation_hpp

#include <ql/math/interpolation.hpp>
#include <vector>

namespace QuantLib {

    namespace detail {
        typedef std::vector<boost::shared_ptr<Interpolation> > interp_list;
        typedef interp_list::iterator it;
        typedef interp_list::const_iterator it2;

        template <class I1, class I2>
        class MultipleRegimeInterpolationImpl
            : public Interpolation::templateImpl<I1,I2> {
          public:
            MultipleRegimeInterpolationImpl(const I1& xBegin, const I1& xEnd,
                                    const I2& yBegin,
                std::vector<boost::shared_ptr<Interpolation> > L_)
            : Interpolation::templateImpl<I1,I2>(xBegin, xEnd, yBegin),
                L(L_) {}
            void update() {
                for (it i = L.begin(); i != L.end(); i++) {
                    (*i)->update();
                }
            }
            Real value(Real x) const {
                return getInterp(x)->operator()(x);
            }
            Real primitive(Real x) const {
                return getInterp(x)->primitive(x);
            }
            Real derivative(Real x) const {
                return getInterp(x)->derivative(x);
            }
            Real secondDerivative(Real x) const {
                return getInterp(x)->secondDerivative(x);
            }
          private:
            std::vector<boost::shared_ptr<Interpolation> > L;
            boost::shared_ptr<Interpolation> getInterp(Real x) const {
                for (it2 i = L.begin(); i != L.end(); i++) {
                    if ((*i)->isInRange(x))
                        return *i;
                }
                QL_FAIL("interpolation range is ["
                       << this->xMin() << ", " << this->xMax()
                       << "]: extrapolation at " << x << " not allowed");
            }
        };

    }

    //! %Linear interpolation between discrete points
    class MultipleRegimeInterpolation : public Interpolation {
      public:
        /*! \pre the \f$ x \f$ values must be sorted. */
        template <class I1, class I2>
        MultipleRegimeInterpolation(const I1& xBegin, const I1& xEnd,
            const I2& yBegin,
            std::vector<boost::shared_ptr<Interpolation> > L) {
            impl_ = boost::shared_ptr<Interpolation::Impl>(new
                detail::MultipleRegimeInterpolationImpl<I1,I2>(xBegin, xEnd,
                                                       yBegin, L));
            impl_->update();
        }
    };

    //! %Linear-interpolation factory and traits
//    class Linear {
//      public:
//        template <class I1, class I2>
//        Interpolation interpolate(const I1& xBegin, const I1& xEnd,
//                                  const I2& yBegin) const {
//            return LinearInterpolation(xBegin, xEnd, yBegin);
//        }
//        static const bool global = false;
//        static const Size requiredPoints = 2;
//    };

}

#endif
