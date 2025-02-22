//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   SyntheticAcceleration.cc
 * \author Steven Hamilton
 * \brief  Perform Adjoint Monte Carlo on linear system
 */
//---------------------------------------------------------------------------//

#include <iterator>
#include <string>

#include "Teuchos_ArrayRCP.hpp"

#include "SyntheticAcceleration.hh"
#include "LinearSolverFactory.hh"

namespace mcrex
{

//---------------------------------------------------------------------------//
/*!
 * \brief Constructor
 *
 * \param A  Problem matrix
 * \param pl ParameterList
 *
 * Behavior is controlled by following PL entries on the nested
 * "SyntheticAcceleration"
 * sublist:
 *  - max_iterations(int)             : >0 (1000)
 *  - damping(SCALAR)                 : >0.0 (1.0)
 *  - tolerance(MAGNITUDE)            : >0.0 (1.0e-6)
 *  - divergence_tolerance(MAGNITUDE) : >0.0 (1.0e4),
 *                                      residual norm to declare failure
 *  - verbosity(string)               : "none", ("low"), "medium", "high"
 */
//---------------------------------------------------------------------------//
SyntheticAcceleration::SyntheticAcceleration(Teuchos::RCP<const MATRIX> A,
           Teuchos::RCP<Teuchos::ParameterList> pl )
  : MCREX_Solver(A,pl)
{
    // Get SyntheticAcceleration pl
    Teuchos::RCP<Teuchos::ParameterList> solver_pl =
        Teuchos::sublist(pl,"Synthetic Acceleration");

    d_damping = solver_pl->get("damping_factor",1.0);

    d_divergence_tol = solver_pl->get("divergence_tolerance",1.0e4);

    // Override settings if present on local list
    this->setParameters(solver_pl);

    // Build preconditioner, default to monte_carlo (MCSA)
    std::string prec_type = pl->get("preconditioner","monte_carlo");
    d_preconditioner = LinearSolverFactory::buildSolver(prec_type,A,pl);

    b_label = "SyntheticAcceleration";
}

//---------------------------------------------------------------------------//
// PRIVATE MEMBER FUNCTIONS
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
/*!
 * \brief Perform SyntheticAcceleration iterations.
 */
//---------------------------------------------------------------------------//
void SyntheticAcceleration::applyImpl(const MV &x, MV &y) const
{
    // For now we only support operating on a single vector
    TEUCHOS_TEST_FOR_EXCEPT( x.getNumVectors() != 1 );

    // Compute initial residual
    MV r(y);
    r.update(1.0,x,0.0);

    MV Pr(y);

    Teuchos::ArrayRCP<MAGNITUDE> r_norm(1);
    r.norm2(r_norm());
    if( r_norm[0] == 0.0 )
    {
        if( b_verbosity >= LOW )
        {
            std::cout << "Input vector has zero norm."
                << "  Setting output vector to zero without iterating."
                << std::endl;
        }
        return;
    }

    MAGNITUDE r0_norm = r_norm[0];

    b_num_iters = 0;
    while( true )
    {
        b_num_iters++;

        // y = y + r
        y.update(1.0,r,1.0);

        // Compute residual r = x - A*y
        b_A->apply(y,r);
        r.update(1.0,x,-1.0);

        // Precondition residual
        if( d_preconditioner != Teuchos::null )
        {
            d_preconditioner->apply(r,Pr);
        }
        else
        {
            Pr.update(1.0,r,0.0);
        }

        // Update y: y = y + Pr
        y.update(d_damping,Pr,1.0);

        // Compute residual r = x - A*y
        b_A->apply(y,r);
        r.update(1.0,x,-1.0);

        r.norm2(r_norm());

        if( b_verbosity >= HIGH )
        {
            std::cout << "Relative residual norm at iteration " << b_num_iters
                << " is " << r_norm[0]/r0_norm << std::endl;
        }

        // Check for convergence
        if( r_norm[0]/r0_norm < b_tolerance )
        {
            if( b_verbosity >= LOW )
            {
                std::cout << "SyntheticAcceleration converged after "
                    << b_num_iters << " iterations." << std::endl;
            }
            break;
        }

        // Check for max iterations
        if( b_num_iters >= b_max_iterations )
        {
            if( b_verbosity >= LOW )
            {
                std::cout << "SyntheticAcceleration reached maximum iteration "
                    << "count with relative residual norm of "
                    << r_norm[0]/r0_norm << std::endl;
            }
            b_num_iters = -1;
            break;
        }

        // Check for divergence
        if( r_norm[0]/r0_norm > d_divergence_tol)
        {
            if( b_verbosity >= LOW )
            {
                std::cout << "SyntheticAcceleration diverged after "
                    << b_num_iters << " iterations." << std::endl;
            }
            b_num_iters = -b_num_iters;
            break;
        }
    }
}

} // namespace mcrex

