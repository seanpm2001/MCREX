//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   Polynomial.cc
 * \author Steven Hamilton
 * \brief  Polynomial class definitions.
 */
//---------------------------------------------------------------------------//

#include "Polynomial.hh"
#include "MCREX_Typedefs.hh"


namespace mcrex
{

//---------------------------------------------------------------------------//
/*!
 * \brief Constructor.
 */
//---------------------------------------------------------------------------//
Polynomial::Polynomial(Teuchos::RCP<const MATRIX> A,
                 Teuchos::RCP<Teuchos::ParameterList> pl)
  : b_A(A)
  , b_pl(pl)
{
    TEUCHOS_ASSERT( b_pl != Teuchos::null );

    b_poly_pl = Teuchos::sublist(b_pl,"Polynomial");

    b_m = b_poly_pl->get<int>("polynomial_order",1);

    // Set verbosity for polynomial construction
    std::string verb = b_poly_pl->get("verbosity","none");
    if( verb == "high" )
        b_verbosity = HIGH;
    else if( verb == "medium" )
        b_verbosity = MEDIUM;
    else if( verb == "low" )
        b_verbosity = LOW;
    else
        b_verbosity = NONE;
}

//---------------------------------------------------------------------------//
// PUBLIC MEMBER FUNCTIONS
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
/*!
 * \brief Access polynomial coefficients.
 *
 * Each polynomial type constructs a set of coefficients \f$ c_i \f$ such
 * that \f$ p(x) = \sum_{i=0}^m c_i b_\mathrm{native}(x)^i \f$, where
 * \f$ b_\mathrm{native}(x) = \alpha + \beta x \f$ is a linear basis that is
 * chosen by the particular polynomial type.  The calling function
 * (or the user) will typically want to work in a prescribed ``target''
 * basis.  This function returns the coefficients with respect to that
 * target basis.
 */
//---------------------------------------------------------------------------//
Teuchos::ArrayRCP<const SCALAR>
Polynomial::getCoeffs(const PolynomialBasis &target) const
{
    TEUCHOS_ASSERT( b_native_basis != Teuchos::null );
    TEUCHOS_ASSERT( b_coeffs != Teuchos::null );
    TEUCHOS_ASSERT( b_coeffs.size() == (b_m+1) );

    // Convert coefficients from native basis to target
    Teuchos::ArrayRCP<SCALAR> target_coeffs;
    target_coeffs = target.transformBasis(b_coeffs,*b_native_basis);
    TEUCHOS_ASSERT( target_coeffs.size() == (b_m+1) );

    if( b_verbosity >= MEDIUM )
    {
        std::cout << "Polynomial Coefficients: " << std::endl;
        for( int i=0; i<b_m+1; ++i )
        {
            std::cout << i << " " << b_coeffs[i] << std::endl;
        }
    }

    return target_coeffs;
}

} // namespace mcrex

