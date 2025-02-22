//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   LinearSystemFactory.cc
 * \author Steven Hamilton
 * \brief  Construct EpetraCrsMatrix from ParameterList.
 */
//---------------------------------------------------------------------------//

#include <string>
#include <cmath>

#include "LinearSystemFactory.hh"

// Trilinos includes
#include "Teuchos_DefaultSerialComm.hpp"
#ifdef HAVE_MPI
#include "Teuchos_DefaultMpiComm.hpp"
#endif
#include "MatrixMarket_Tpetra.hpp"

namespace mcrex
{

//---------------------------------------------------------------------------//
/*!
 * \brief Build matrix from ParameterList.
 *
 *  \param pl ParameterList containing information about matrix construction.
 *
 *  Matrix construction is controlled by the following PL entries on the
 *  nested "Problem" sublist:
 *   - matrix_type(string): laplacian, convection-diffusion, matrix_market
 *                          (no default)
 *   - matrix_size(int):    N>0 (25) (only used if "matrix_type"="laplacian"
 *                          or "convection-diffusion")
 *   - matrix_file(string): valid matrix market filename
 *   - viscosity(SCALAR):   value for viscosity
 *                          (if "matrix_type"="convection-diffusion")
 *   - laplacian_shift(SCALAR): diagonal shift to apply to Laplacian matrix
 *                              (only applies if "matrix_type"="laplacian")
 */
//---------------------------------------------------------------------------//
Teuchos::RCP<LinearSystem>
LinearSystemFactory::buildLinearSystem( Teuchos::RCP<Teuchos::ParameterList> pl )
{
    Teuchos::RCP<Teuchos::ParameterList> mat_pl =
        Teuchos::sublist(pl,"Problem");

    TEUCHOS_ASSERT( mat_pl->isType<std::string>("matrix_type") );
    std::string matrix_type = mat_pl->get<std::string>("matrix_type");
    TEUCHOS_TEST_FOR_EXCEPT( matrix_type!="laplacian" &&
                             matrix_type!="convection-diffusion" &&
                             matrix_type!="matrix_market" );

    Teuchos::RCP<MATRIX> A = Teuchos::null;
    Teuchos::RCP<MV>     b = Teuchos::null;
    if( matrix_type == "laplacian" )
    {
        buildDiffusionSystem(mat_pl,A,b);
    }
    else if( matrix_type == "convection-diffusion" )
    {
        buildConvectionDiffusionSystem(mat_pl,A,b);
    }
    else if( matrix_type == "matrix_market" )
    {
        buildMatrixMarketSystem(mat_pl,A,b);
    }

    applyScaling(A,b,mat_pl);

    return Teuchos::rcp( new LinearSystem(A,b) );
}

//---------------------------------------------------------------------------//
// Build matrix and RHS for diffusion problem
//---------------------------------------------------------------------------//
void LinearSystemFactory::buildDiffusionSystem(
    Teuchos::RCP<Teuchos::ParameterList> pl,
    Teuchos::RCP<MATRIX>                 &A,
    Teuchos::RCP<MV>                     &b )
{
    GO N = pl->get("matrix_size",25);

    A = buildLaplacianMatrix(N,pl);

    // Sinusoidal source with homogeneous Dirichlet boundaries
    b = Teuchos::rcp( new MV(A->getDomainMap(),1) );
    const double pi = 4.0*std::atan(1.0);
    for( GO i=0; i<N; ++i )
    {
        if( A->getDomainMap()->isNodeLocalElement(i) )
        {
            double val = std::sin(pi*static_cast<double>(i)/
                                       static_cast<double>(N-1));
            //double val = std::exp(-100.0*std::abs(static_cast<double>(i-N/2)/
            //                                      static_cast<double>(N)));
            b->replaceLocalValue(i,0,val);
        }
    }
}

//---------------------------------------------------------------------------//
// Build matrix and RHS for diffusion problem
//---------------------------------------------------------------------------//
void LinearSystemFactory::buildConvectionDiffusionSystem(
    Teuchos::RCP<Teuchos::ParameterList> pl,
    Teuchos::RCP<MATRIX>                 &A,
    Teuchos::RCP<MV>                     &b )
{
    GO N = pl->get("matrix_size",25);
    SCALAR nu = pl->get("viscosity",1.0);
    Teuchos::RCP<MATRIX> Ad = buildLaplacianMatrix(N,pl);
    Teuchos::RCP<MATRIX> Ac = buildConvectionMatrix(N,pl);
    A = Teuchos::rcp_dynamic_cast<MATRIX>(
        Ad->add(1.0,*Ac,nu,Ad->getDomainMap(),Ad->getRangeMap(),pl),true);

    // Sinusoidal source with homogeneous Dirichlet boundaries
    b = Teuchos::rcp( new MV(A->getDomainMap(),1) );
    const double pi = 4.0*std::atan(1.0);
    for( LO i=0; i<N; ++i )
    {
        if( A->getDomainMap()->isNodeLocalElement(i) )
        {
            double val = 2*pi*static_cast<double>(i)/static_cast<double>(N-1);
            b->replaceLocalValue(i,0,std::sin(val));
        }
    }
}

//---------------------------------------------------------------------------//
// Build cell-based 1D finite difference Laplacian matrix on [0,1]
//---------------------------------------------------------------------------//
Teuchos::RCP<MATRIX>
LinearSystemFactory::buildLaplacianMatrix(
    int N, Teuchos::RCP<Teuchos::ParameterList> pl )
{
    SCALAR h = 1.0 / static_cast<SCALAR>(N);

    SCALAR laplacian_shift = pl->get("laplacian_shift",0.0);
    TEUCHOS_ASSERT( laplacian_shift >= 0.0 );

    // Build comm and map
    Teuchos::RCP<Teuchos::Comm<LO> > comm;
#ifdef HAVE_MPI
    int have_mpi;
    MPI_Initialized(&have_mpi);
    if( have_mpi )
    {
        comm = Teuchos::rcp( new Teuchos::MpiComm<LO>(MPI_COMM_WORLD) );
    }
    else
#endif
    {
        comm = Teuchos::rcp( new Teuchos::SerialComm<LO>() );
    }
    LO base=0;
    Teuchos::RCP<MAP> map( new MAP(N,base,comm) );

    LO numPerRow = 3;
    Teuchos::RCP<CRS_MATRIX> A =
        Teuchos::rcp( new CRS_MATRIX( map, numPerRow ) );

    LO numMyRows = map->getNodeNumElements();

    Teuchos::ArrayRCP<SCALAR> val(3);
    Teuchos::ArrayRCP<GO>     ind(3);
    for( LO i=0; i<numMyRows; ++i )
    {
        LO numThisRow;

        GO gid = map->getGlobalElement( i );
        if( gid == 0 )
        {
            // Dirichlet on left
            ind[0] = 0;
            val[0] = (3.0+laplacian_shift)/(h*h);
            ind[1] = 1;
            val[1] = -1.0/(h*h);
            numThisRow = 2;
        }
        else if( gid == N-1 )
        {
            // Dirichlet on right
            ind[0] = N-2;
            val[0] = -1.0/(h*h);
            ind[1] = N-1;
            val[1] = (3.0+laplacian_shift)/(h*h);
            numThisRow = 2;
        }
        else
        {
            ind[0] = gid-1;
            val[0] = -1.0/(h*h);
            ind[1] = gid;
            val[1] = (2.0+laplacian_shift)/(h*h);
            ind[2] = gid+1;
            val[2] = -1.0/(h*h);
            numThisRow = 3;
        }

        Teuchos::ArrayView<SCALAR> val_view = val(0,numThisRow);
        Teuchos::ArrayView<GO>     ind_view = ind(0,numThisRow);
        A->insertGlobalValues( gid, ind_view, val_view );
    }
    A->fillComplete();

    return A;
}

//---------------------------------------------------------------------------//
// Build cell-based 1D finite difference convection matrix on [0,1]
//---------------------------------------------------------------------------//
Teuchos::RCP<MATRIX>
LinearSystemFactory::buildConvectionMatrix(
    int N, Teuchos::RCP<Teuchos::ParameterList> pl )
{
    SCALAR h = 1.0 / static_cast<SCALAR>(N);

    // Build comm and map
    Teuchos::RCP<Teuchos::Comm<LO> > comm;
#ifdef HAVE_MPI
    int have_mpi;
    MPI_Initialized(&have_mpi);
    if( have_mpi )
    {
        comm = Teuchos::rcp( new Teuchos::MpiComm<LO>(MPI_COMM_WORLD) );
    }
    else
#endif
    {
        comm = Teuchos::rcp( new Teuchos::SerialComm<LO>() );
    }
    LO base=0;
    Teuchos::RCP<MAP> map( new MAP(N,base,comm) );

    LO numPerRow = 3;
    Teuchos::RCP<CRS_MATRIX> A =
        Teuchos::rcp( new CRS_MATRIX( map, numPerRow ) );

    LO numMyRows = map->getNodeNumElements();

    Teuchos::ArrayRCP<SCALAR> val(2);
    Teuchos::ArrayRCP<GO>     ind(2);
    for( LO i=0; i<numMyRows; ++i )
    {
        LO numThisRow;

        GO gid = map->getGlobalElement( i );
        if( gid == 0 )
        {
            // Dirichlet on left
            ind[0] = 0;
            val[0] = 0.5/h;
            ind[1] = 1;
            val[1] = 0.5/h;
            numThisRow = 2;
        }
        else if( gid == N-1 )
        {
            // Dirichlet on right
            ind[0] = N-2;
            val[0] = 0.5/h;
            ind[1] = N-1;
            val[1] = 0.5/h;
            numThisRow = 2;
        }
        else
        {
            ind[0] = gid-1;
            val[0] = -0.5/h;
            ind[1] = gid+1;
            val[1] = 0.5/h;
            numThisRow = 2;
        }

        Teuchos::ArrayView<SCALAR> val_view = val(0,numThisRow);
        Teuchos::ArrayView<GO>     ind_view = ind(0,numThisRow);
        A->insertGlobalValues( gid, ind_view, val_view );
    }
    A->fillComplete();

    return A;
}

//---------------------------------------------------------------------------//
/*!
 * \brief Read matrix from Matrix Market file.
 */
//---------------------------------------------------------------------------//
void LinearSystemFactory::buildMatrixMarketSystem(
        Teuchos::RCP<Teuchos::ParameterList> pl,
        Teuchos::RCP<MATRIX>                 &A,
        Teuchos::RCP<MV>                     &b )
{
    TEUCHOS_ASSERT( pl->isType<std::string>("matrix_filename") );

    std::string filename = pl->get<std::string>("matrix_filename");

    Teuchos::RCP<NODE> node = KokkosClassic::Details::getNode<NODE>();
    Teuchos::RCP<Teuchos::Comm<LO> > comm;
#ifdef HAVE_MPI
    int have_mpi;
    MPI_Initialized(&have_mpi);
    if( have_mpi )
    {
        comm = Teuchos::rcp( new Teuchos::MpiComm<LO>(MPI_COMM_WORLD) );
    }
    else
#endif
    {
        comm = Teuchos::rcp( new Teuchos::SerialComm<LO>() );
    }

    A = Tpetra::MatrixMarket::Reader<CRS_MATRIX>::readSparseFile(
        filename,comm,node);

    Teuchos::RCP<const MAP> map = A->getDomainMap();
    if( pl->isType<std::string>("rhs_filename") )
    {
        std::string rhs_file = pl->get<std::string>("rhs_filename");
        b = Tpetra::MatrixMarket::Reader<CRS_MATRIX>::readVectorFile(
            rhs_file,comm,node,map);
    }
    else
    {
        // Just make RHS constant
        b = Teuchos::rcp( new MV(map,1) );
        b->putScalar(1.0);
    }

    // If an initial guess is specified, compute the initial residual and
    // use that as the RHS
    if( pl->isType<std::string>("init_guess_filename") )
    {
        std::string x0_file = pl->get<std::string>("init_guess_filename");
        Teuchos::RCP<MV> x0 =
            Tpetra::MatrixMarket::Reader<CRS_MATRIX>::readVectorFile(
                x0_file,comm,node,map);
        A->apply(*x0,*b,Teuchos::NO_TRANS,-1.0,1.0);
    }
}

//---------------------------------------------------------------------------//
/*!
 * \brief Apply specified scaling to matrix
 */
//---------------------------------------------------------------------------//
void LinearSystemFactory::applyScaling( Teuchos::RCP<MATRIX>                 A,
                                        Teuchos::RCP<MV>                     b,
                                        Teuchos::RCP<Teuchos::ParameterList> pl )
{
    VECTOR diag(A->getDomainMap());
    A->getLocalDiagCopy(diag);

    std::string scale_type = pl->get<std::string>("scaling_type","diagonal");
    TEUCHOS_ASSERT( scale_type=="diagonal" ||
                    scale_type=="row"      ||
                    scale_type=="column"   ||
                    scale_type=="sign"     ||
                    scale_type=="none" );

    // Temporarily disable row and column scaling
    TEUCHOS_ASSERT( scale_type!="row" );
    TEUCHOS_ASSERT( scale_type!="column" );

    if( scale_type == "diagonal" )
    {
        Teuchos::ArrayRCP<SCALAR> diag_data = diag.getDataNonConst();
        for( LO i=0; i<diag_data.size(); ++i )
        {
            if( diag_data[i] != 0.0 )
            {
                diag_data[i] = 1.0 / diag_data[i];
            }
            else
            {
                diag_data[i] = 1.0;
            }
        }
        A->leftScale(diag);
        Teuchos::ArrayRCP<SCALAR> b_data = b->getDataNonConst(0);
        for( LO i=0; i<b_data.size(); ++i )
        {
            b_data[i] = b_data[i] * diag_data[i];
            TEUCHOS_ASSERT( !SCALAR_TRAITS::isnaninf(b_data[i]) );
        }
    }
    if( scale_type == "row" )
    {
        /*
        VECTOR inv_row_sums(A->getDomainMap());
        A->invRowSums(inv_row_sums);

        // Modify scaling by sign of diagonal
        for( int i=0; i<inv_row_sums.getLocalLength(); ++i )
        {
            if( diag[i] < 0.0 )
            {
                inv_row_sums[i] *= -1.0;
            }
        }

        A->leftScale(inv_row_sums);
        for( size_t i=0; i<b->getLocalLength(); ++i )
        {
            b->replaceLocalValue(i,0,b->getData(0)[i]*inv_row_sums.getData()[i]);
        }
        */
    }
    else if( scale_type == "column" )
    {
        /*
        VECTOR inv_col_sums(A->getDomainMap());
        A->invColSums(inv_col_sums);

        // Modify scaling by sign of diagonal
        for( int i=0; i<inv_col_sums.getLocalLength(); ++i )
        {
            if( diag[i] < 0.0 )
            {
                inv_col_sums[i] *= -1.0;
            }
        }

         A->rightScale(inv_col_sums);
        */
    }
    else if( scale_type == "sign" )
    {
        // Scale each row by sign of diagonal
        VECTOR diag_sign(A->getDomainMap());
        for( size_t i=0; i<diag_sign.getLocalLength(); ++i )
        {
            diag_sign.replaceLocalValue(i,diag.getData(0)[i] > 0.0 ? 1.0 : -1.0);
        }

        A->leftScale(diag_sign);
        for( size_t i=0; i<b->getLocalLength(); ++i )
        {
            b->replaceLocalValue(i,0,b->getData(0)[i]*diag_sign.getData()[i]);
        }
    }

}

} // namespace mcrex

