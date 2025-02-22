
#include <iomanip>
#include <chrono>

#include "LinearSystem.hh"
#include "LinearSystemFactory.hh"
#include "LinearSolverFactory.hh"

#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"

#include "MCREX_Typedefs.hh"

using namespace mcrex;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

int main( int argc, char *argv[] )
{
    int my_rank=0;
#ifdef HAVE_MPI
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
#endif

    high_resolution_clock::time_point time_start = high_resolution_clock::now();

    // Read ParameterList from file
    Teuchos::RCP<Teuchos::ParameterList> pl;
    TEUCHOS_ASSERT( argc > 1 );
    pl = Teuchos::getParametersFromXmlFile(argv[1]);
    TEUCHOS_ASSERT( pl != Teuchos::null );

    Teuchos::RCP<LinearSystem> system =
        mcrex::LinearSystemFactory::buildLinearSystem(pl);
    Teuchos::RCP<const MATRIX> myA = system->getMatrix();
    Teuchos::RCP<const MV> b = system->getRhs();

    Teuchos::RCP<MV> x( new MV(myA->getDomainMap(),1) );

    high_resolution_clock::time_point time_end = high_resolution_clock::now();
    if( my_rank == 0 )
    {
        std::cout << "Matrix loading took "
            << duration_cast<milliseconds>(time_end-time_start).count()
            << " milliseconds" << std::endl;
    }

    time_start = high_resolution_clock::now();

    std::string solver_type = pl->get("solver_type","mcsa");
    Teuchos::RCP<mcrex::MCREX_Solver> solver =
        mcrex::LinearSolverFactory::buildSolver(solver_type,myA,pl);

    time_end = high_resolution_clock::now();
    if( my_rank == 0 )
    {
        std::cout << "Solver setup took "
            << duration_cast<milliseconds>(time_end-time_start).count()
            << " milliseconds" << std::endl;
    }

    time_start = high_resolution_clock::now();
    solver->apply(*b,*x);
    time_end = high_resolution_clock::now();
    if( my_rank == 0 )
    {
        std::cout << "Solve took "
            << duration_cast<milliseconds>(time_end-time_start).count()
            << " milliseconds" << std::endl;
    }

    // Compute final residual
    Teuchos::RCP<MV> r( new MV(myA->getDomainMap(),1) );
    myA->apply(*x,*r);
    r->update(1.0,*b,-1.0);
    Teuchos::ArrayRCP<SCALAR> res_norm(1), b_norm(1);
    r->norm2(res_norm());
    b->norm2(b_norm());
    if( my_rank == 0 )
    {
        std::cout << "Final relative residual norm: "
                  << res_norm[0]/b_norm[0] << std::endl;
    }

#ifdef HAVE_MPI
    MPI_Finalize();
#endif

    return 0;
}

