//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   LinearSystemFactory.hh
 * \author Steven Hamilton
 * \brief  Construct EpetraCrsMatrix from ParameterList.
 */
//---------------------------------------------------------------------------//

#ifndef MCREX_LinearSystemFactory_hh
#define MCREX_LinearSystemFactory_hh

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"

#include "LinearSystem.hh"
#include "MCREX_Typedefs.hh"

namespace mcrex
{

//---------------------------------------------------------------------------//
/*!
 * \class LinearSystemFactory
 * \brief Load/construct matrix.
 *
 * This class provides a simple interface to build a linear system.  Several
 * simple matrices can be constructed from the provided ParameterList or
 * a matrix can be loaded from a Matrix Market file.  A right hand side for
 * the linear system will also be constructed, either by generating a physical
 * source term where appropriate or by assigning a dummy vector if no
 * meaningful vector is available.
 */
//---------------------------------------------------------------------------//
class LinearSystemFactory
{
  private:

    // Pure static, disallow construction
    LinearSystemFactory(){};

  public:

    static Teuchos::RCP<LinearSystem> buildLinearSystem(
        Teuchos::RCP<Teuchos::ParameterList> pl);

  private:

    static void buildDiffusionSystem(
        Teuchos::RCP<Teuchos::ParameterList> pl,
        Teuchos::RCP<MATRIX> &A,
        Teuchos::RCP<MV>     &b );

    static void buildConvectionDiffusionSystem(
        Teuchos::RCP<Teuchos::ParameterList> pl,
        Teuchos::RCP<MATRIX> &A,
        Teuchos::RCP<MV>     &b );

    static Teuchos::RCP<MATRIX> buildLaplacianMatrix(
        int N, Teuchos::RCP<Teuchos::ParameterList> pl);

    static Teuchos::RCP<MATRIX> buildConvectionMatrix(
        int N, Teuchos::RCP<Teuchos::ParameterList> pl);

    static void buildMatrixMarketSystem(
        Teuchos::RCP<Teuchos::ParameterList> pl,
        Teuchos::RCP<MATRIX> &A,
        Teuchos::RCP<MV>     &b );

    static void applyScaling( Teuchos::RCP<MATRIX>                 A,
                              Teuchos::RCP<MV>                     b,
                              Teuchos::RCP<Teuchos::ParameterList> pl);
};

}

#endif // MCREX_LinearSystemFactory_hh

