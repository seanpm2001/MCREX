##---------------------------------------------------------------------------##
# mc_data.py
# Data structure functions for Monte Carlo calculations
##---------------------------------------------------------------------------##
import numpy

##---------------------------------------------------------------------------##
## Make an iteration matrix H = I-A for a given operator for the adjoint
##---------------------------------------------------------------------------##
def makeIterationMatrix( A ):
    grid_size = len(A)
    H = numpy.zeros((grid_size,grid_size))
    for i in xrange(grid_size):
        for j in xrange(grid_size):
            H[i][j] = -A[i][j]
            if i == j:
                H[i][j] = H[i][j] + 1.0
    return H

##---------------------------------------------------------------------------##
## Make an adjoint probability matrix for a given iteration matrix.
##---------------------------------------------------------------------------##
def makeProbabilityMatrix( H ):
    grid_size = len(H)
    P = numpy.zeros((grid_size,grid_size))
    for i in xrange(grid_size):
        row_sum = 0
        for j in xrange(grid_size):
            row_sum = row_sum + abs(H[j][i])
        p_sum = 0
        for j in xrange(grid_size):
            P[i][j] = abs(H[j][i])/row_sum
            p_sum = p_sum + P[i][j]
        for j in xrange(grid_size):
            P[i][j] = P[i][j] / p_sum
    return P

##---------------------------------------------------------------------------##
## Make a cumulative distribution matrix for a given probability matrix.
##---------------------------------------------------------------------------##
def makeCDFMatrix( P ):
    grid_size = len(P)
    C = numpy.zeros((grid_size,grid_size))
    for i in xrange(grid_size):
        C[i][0] = P[i][0]
        for j in xrange(1,grid_size):
            C[i][j] = C[i][j-1] + P[i][j]
    return C

##---------------------------------------------------------------------------##
## Make an adjoint weight matrix from an iteration matrix and an
## adjoint probability matrix.
## ---------------------------------------------------------------------------##
def makeWeightMatrix( H, P ):
    grid_size = len(H)
    W = numpy.zeros((grid_size,grid_size))
    for i in xrange(grid_size):
        for j in xrange(grid_size):
            if P[i][j] > 0.0:
                W[i][j] = H[j][i] / P[i][j]
    return W

##---------------------------------------------------------------------------##
## Make H, C, and W for a Monte Carlo problem.
##---------------------------------------------------------------------------##
def makeMonteCarloHCW( A ):
    H = makeIterationMatrix( A )
    P = makeProbabilityMatrix( H )
    C = makeCDFMatrix( P )
    W = makeWeightMatrix( H, P )
    return (H, C, W)

##---------------------------------------------------------------------------##
## Given a forcing term create the source cdf and starting weight.
##---------------------------------------------------------------------------##
def makeSourceCDF( b ):
    size = len(b)
    source_c = numpy.zeros( size )
    source_c[0] = abs(b[0])
    for i in xrange(1,size):
        source_c[i] = source_c[i-1] + abs(b[i])
    starting_weight = source_c[size-1]
    for i in xrange(size):
        source_c[i] = source_c[i] / starting_weight
    return (source_c, starting_weight)

##---------------------------------------------------------------------------##
## Given a forcing term, create the probability vector and weight vector.
##---------------------------------------------------------------------------##
def makeSourcePW( b ):
    size = len(b)
    p = numpy.zeros(size)
    w = numpy.zeros(size)
    b_norm = numpy.linalg.norm(b,1)
    for i in xrange(size):
        p[i] = abs(b[i])/b_norm
        w[i] = b[i] / p[i]
    return p, w, b_norm

##---------------------------------------------------------------------------##
## Given a source probability vector, build a source CDF
##---------------------------------------------------------------------------##
def makeSourceC( p ):
    size = len(p)
    psum = sum(p)
    c = numpy.zeros(size)
    c[0] = p[0]/psum
    for i in xrange(1,size):
        c[i] = p[i]/psum + c[i-1]
    return c
    
##---------------------------------------------------------------------------##
## end mc_data.py
##---------------------------------------------------------------------------##


