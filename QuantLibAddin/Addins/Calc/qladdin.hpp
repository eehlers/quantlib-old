/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*	
 Copyright (C) 2004, 2005, 2006, 2008 Eric Ehlers
 Copyright (C) 2009 Roland Lichters

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

/*
    Based on the "Simple" and "Complete" Calc add-in example in C++.
    Eric's addin code incorporated step by step from QuantLibAddin/Addins/Calc.

    http://wiki.services.openoffice.org/wiki/Calc/Add-In/Simple_Calc_Add-In
    http://wiki.services.openoffice.org/wiki/CompleteAddIn
    http://quantlib.org/quantlibaddin
*/

#ifndef quantlib_calcaddins_hpp
#define quantlib_calcaddins_hpp

#include <cppuhelper/implbase4.hxx> //4-parameter template will be used
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implementationentry.hxx>

#include <uno/lbnames.h>

#include <com/sun/star/sheet/XAddIn.hpp>
#include <com/sun/star/lang/XServiceName.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cstdlib>
#include <cmath>

/* 
   Header generated by idlc/cppumaker from calcaddins.idl

   Its location on the file system is $OO_SDK_OUT/inc/SAMPLE_NAME/ModuleName
   where 
   - OO_SDK_OUT is an environment variable set when the SDK is installed
   - SAMPLE_NAME as defined in the Makefile, here: CalcAddinCpp
   - ModuleName as defined in the idl file, here: quantlib

   Directory $OO_SDK_OUT/inc/SAMPLE_NAME is added to the include path so that 
   the inlude below works.
*/
#include <quantlib/XCalcAddins.hpp>

// QuantLib
#include <ql/quantlib.hpp>
// ObjectHandler
#include <oh/objecthandler.hpp>
// QuantLibAddin
#include <qlo/qladdin.hpp>
#include <qldefs.hpp>
// QuantLibAddin/Addins/Calc files copied to this directory and maybe modified
#include <conversions.hpp>
#include <init.hpp>

#define _serviceName_ "quantlib.CalcAddins"
#define _implName_ "quantlib.CalcAddins_impl.CalcAddins"
#define _AddserviceName_ "com.sun.star.sheet.AddIn"

using namespace ::rtl;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

//4-parameter template
class CalcAddins_impl 
    : public ::cppu::WeakImplHelper4< ::quantlib::XCalcAddins, sheet::XAddIn,
                                      lang::XServiceName, lang::XServiceInfo> {
    //Locale
    lang::Locale locale;
       
    std::map< STRING, STRING > funcMap;                 // function names
    std::map< STRING, STRING > funcDesc;                // function descriptions
    std::map< STRING, std::vector < STRING > > argName; // parameter names
    std::map< STRING, std::vector < STRING > > argDesc; // parameter description
public:
    //XCalcAddins
    /**************************************************************************
     * Start declaring methods here ...
     * Eric has put these into separate header files that are included at the
     * bottom of the class decleration with #include <<Addins/Calc/qlo_all.hpp>
     *************************************************************************/
    // basic examples
//     long SAL_CALL methodOne() throw (RuntimeException);;
//     long SAL_CALL methodTwo(long dummy) throw (RuntimeException);
//     double SAL_CALL methodThree(long dummy, double m) throw (RuntimeException);
//     double SAL_CALL methodFour(double m) throw (RuntimeException);
//     Sequence<Sequence<long> > 
//     methodFive(const Sequence<Sequence<long> > &aValList)
//         throw (RuntimeException);

    #include "qlo_all.hpp"

    /*************************************************************************
     * ... and stop declaring methods here.
     *************************************************************************/

    //XAddIn
    OUString SAL_CALL getProgrammaticFuntionName(const OUString& aDisplayName)
        throw (RuntimeException);
    OUString SAL_CALL getDisplayFunctionName(const OUString& aProgrammaticName) 
        throw (RuntimeException);
    OUString SAL_CALL getFunctionDescription(const OUString& aProgrammaticName) 
        throw (RuntimeException);
    OUString SAL_CALL getDisplayArgumentName(const OUString& aProgrammaticName,
                                             ::sal_Int32 nArgument) 
        throw (RuntimeException);
    OUString SAL_CALL getArgumentDescription(const OUString& aProgrammaticName,
                                             ::sal_Int32 nArgument) 
        throw (RuntimeException);
    OUString SAL_CALL getProgrammaticCategoryName(
                                             const OUString& aProgrammaticName) 
        throw (RuntimeException);
    OUString SAL_CALL getDisplayCategoryName(const OUString& aProgrammaticName)
        throw (RuntimeException);

    //XServiceName
    OUString SAL_CALL getServiceName() throw (RuntimeException);

    //XServiceInfo
    OUString SAL_CALL getImplementationName() throw (RuntimeException);
    ::sal_Bool SAL_CALL supportsService(const OUString& ServiceName) 
        throw (RuntimeException);
    Sequence< OUString > SAL_CALL getSupportedServiceNames() 
        throw (RuntimeException);

    //XLocalizable
    void SAL_CALL setLocale(const lang::Locale& eLocale) 
        throw (RuntimeException);
    lang::Locale SAL_CALL getLocale() throw (RuntimeException);

    /**************************************************************************
     * RL: Declare constructor and desctructor here, following Eric's example.
     *     This is to initialize QuantLibAddin's repository and set up
     *     descriptions for the function wizard.
     *     Implemented in funcdef.cpp
     *************************************************************************/
    CalcAddins_impl() throw ();
    virtual ~CalcAddins_impl() {};

};

#endif
