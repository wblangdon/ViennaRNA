// The contents of this file are in the public domain. See LICENSE_FOR_EXAMPLE_PROGRAMS.txt
/*
 *
 *  This is an example illustrating the use the general purpose non-linear
 *  optimization routines from the dlib C++ Library.
 *
 *  The library provides implementations of many popular algorithms such as L-BFGS
 *  and BOBYQA.  These algorithms allow you to find the minimum or maximum of a
 *  function of many input variables.  This example walks though a few of the ways
 *  you might put these routines to use.
 *
 */


#include <iostream>
#include <dlib/optimization.h>
#include <dlib/global_optimization.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "ViennaRNA/utils/basic.h"
#ifdef __cplusplus
}
#endif

#include "wrap_dlib.h"

using namespace std;
using namespace dlib;

// ----------------------------------------------------------------------------------------

// In dlib, most of the general purpose solvers optimize functions that take a
// column vector as input and return a double.  So here we make a typedef for a
// variable length column vector of doubles.  This is the type we will use to
// represent the input to our objective functions which we will be minimizing.
typedef matrix<double, 0, 1> column_vector;

// ----------------------------------------------------------------------------------------
// Below we create a few functions.  When you get down into main() you will see that
// we can use the optimization algorithms to find the minimums of these functions.
// ----------------------------------------------------------------------------------------

PRIVATE double
rosen(const column_vector& m)
/*
 *  This function computes what is known as Rosenbrock's function.  It is
 *  a function of two input variables and has a global minimum at (1,1).
 *  So when we use this function to test out the optimization algorithms
 *  we will see that the minimum found is indeed at the point (1,1).
 */
{
  const double  x = m(0);
  const double  y = m(1);

  // compute Rosenbrock's function and return the result
  return 100.0 * pow(y - x * x, 2) + pow(1 - x, 2);
}


// This is a helper function used while optimizing the rosen() function.
PRIVATE const column_vector
rosen_derivative(const column_vector& m)
/*!
 *  ensures
 *      - returns the gradient vector for the rosen function
 * !*/
{
  const double  x = m(0);
  const double  y = m(1);

  // make us a column vector of length 2
  column_vector res(2);

  // now compute the gradient vector
  res(0)  = -400 * x * (y - x * x) - 2 * (1 - x); // derivative of rosen() with respect to x
  res(1)  = 200 * (y - x * x);                    // derivative of rosen() with respect to y
  return res;
}


// This function computes the Hessian matrix for the rosen() fuction.  This is
// the matrix of second derivatives.
PRIVATE matrix<double>
rosen_hessian(const column_vector& m)
{
  const double    x = m(0);
  const double    y = m(1);

  matrix<double>  res(2, 2);

  // now compute the second derivatives
  res(0, 0) = 1200 * x * x - 400 * y + 2; // second derivative with respect to x
  res(1, 0) = res(0, 1) = -400 * x;       // derivative with respect to x and y
  res(1, 1) = 200;                        // second derivative with respect to y
  return res;
}


// ----------------------------------------------------------------------------------------

class rosen_model
{
/*!
 *  This object is a "function model" which can be used with the
 *  find_min_trust_region() routine.
 * !*/

public:
typedef ::column_vector column_vector;
typedef matrix<double> general_matrix;

double
operator()(const column_vector& x) const
{
  return rosen(x);
}


void
get_derivative_and_hessian(const column_vector& x,
                           column_vector&       der,
                           general_matrix&      hess) const
{
  der   = rosen_derivative(x);
  hess  = rosen_hessian(x);
}
};

// ----------------------------------------------------------------------------------------

/*
 *  function to minimize to obtain equlibrium concentrations
 *  of multistrand systems. We use the transformation
 *
 *  L_a = lambda_a + ln Z_a
 *
 *  such that h(L) reads
 *
 *  h(L) = -\sum_a (c_a L_a - exp(L_a)) + sum_k K_k exp(sum_b L_b A_{b,k}
 *
 *  with total concentration c_a of strand a, equilibrium constant
 *  K_k of strand k, and membership matrix A[b][k] denoting the number
 *  of strands b in complex k
 *
 *  Note, here we minimize h(L) due to implementation issues whereas
 *  in our publication we've written h'(L) = -h(L) to effectively
 *  maximize the function instead.
 */
PRIVATE double
h(const column_vector&  L,
  const double          *eq_constants,
  const double          *concentration_strands_tot,
  const unsigned int    **A,
  size_t                strands,
  size_t                complexes)
{
  double h, hh, *K;

  K = (double *)vrna_alloc(sizeof(double) * complexes);
  h = 0.;

  for (size_t k = 0; k < complexes; k++) {
    for (size_t a = 0; a < strands; a++)
      K[k] += L(a) *
              (double)A[a][k];

    K[k] = eq_constants[k] *
           exp(K[k]);
  }

  for (size_t a = 0; a < strands; a++)
    h -= concentration_strands_tot[a] *
         L(a) -
         exp(L(a));

  for (size_t k = 0; k < complexes; k++)
    h += K[k];

  free(K);

  return h;
}


/*
 *  Get gradient of h(L)
 */
PRIVATE const column_vector
h_derivative(const column_vector& L,
             const double         *eq_constants,
             const double         *concentration_strands_tot,
             const unsigned int   **A,
             size_t               strands,
             size_t               complexes)
{
  double        gg, *K;
  column_vector g(strands);

  K = (double *)vrna_alloc(sizeof(double) * complexes);

  for (size_t k = 0; k < complexes; k++) {
    for (size_t a = 0; a < strands; a++)
      K[k] += L(a) *
              (double)A[a][k];

    K[k] = eq_constants[k] *
           exp(K[k]);
  }

  for (size_t a = 0; a < strands; a++) {
    g(a) = -concentration_strands_tot[a] +
           exp(L(a));

    for (size_t k = 0; k < complexes; k++)
      g(a) += (double)A[a][k] *
              K[k];
  }

  free(K);

  return g;
}


/*
 *  Get Hessian of h(L)
 */
PRIVATE matrix<double>
h_hessian(const column_vector&  L,
          const double          *eq_constants,
          const double          **delta,
          const unsigned int    **A,
          size_t                strands,
          size_t                complexes)
{
  double                  hh, *K;

  PRIVATE matrix<double>  H(strands, strands);

  K = (double *)vrna_alloc(sizeof(double) * complexes);

  for (size_t k = 0; k < complexes; k++) {
    for (size_t a = 0; a < strands; a++)
      K[k] += L(a) *
              (double)A[a][k];

    K[k] = eq_constants[k] *
           exp(K[k]);
  }

  for (size_t a = 0; a < strands; a++) {
    for (size_t b = 0; b < strands; b++) {
      H(a, b) = //delta[a][b] *
                exp(L(a));

      for (size_t k = 0; k < complexes; k++)
        H(a, b) += (double)(A[a][k] * A[b][k]) *
                   K[k];
    }
  }

  free(K);

  return H;
}


class h_model
{
/*!
 *  This object is a "function model" which can be used with the
 *  find_min_trust_region() routine.
 * !*/

public:
typedef ::column_vector column_vector;
typedef matrix<double> general_matrix;

const double *eq_constants;
const double *concentration_strands_tot;
const double **delta;
const unsigned int **A;
size_t strands;
size_t complexes;

double
operator()(const column_vector& x) const
{
  return h(x, eq_constants, concentration_strands_tot, A, strands, complexes);
}


void
init(const double       *eq_constants,
     const double       *concentration_strands_tot,
     const unsigned int **A,
     const double       **delta,
     size_t             strands,
     size_t             complexes)
{
  this->eq_constants              = eq_constants;
  this->concentration_strands_tot = concentration_strands_tot;
  this->A                         = A;
  this->delta                     = delta;
  this->strands                   = strands;
  this->complexes                 = complexes;
}


void
get_derivative_and_hessian(const column_vector& x,
                           column_vector&       der,
                           general_matrix&      hess) const
{
  der   = h_derivative(x, eq_constants, concentration_strands_tot, A, strands, complexes);
  hess  = h_hessian(x, eq_constants, delta, A, strands, complexes);
}
};


/*
 *  Get concentrations of single strands from
 *  a given vector L (that minimizes h(L))
 */
PRIVATE double *
conc_single_strands(const column_vector&  L,
                    size_t                strands)
{
  double *c = (double *)vrna_alloc(sizeof(double) * strands);

  for (size_t a = 0; a < strands; a++)
    c[a] = exp(L(a));

  return c;
}


/*
 *  Get concentrations of complexes from
 *  a given vector L (that minimizes h(L))
 */
PRIVATE double *
conc_complexes(const column_vector& L,
               const double         *eq_const,
               const unsigned int   **A,
               size_t               strands,
               size_t               complexes)
{
  double *c, *c_free;

  c       = (double *)vrna_alloc(sizeof(double) * complexes);
  c_free  = conc_single_strands(L, strands);

  for (size_t k = 0; k < complexes; k++) {
    c[k] = eq_const[k];

    for (size_t a = 0; a < strands; a++)
      c[k] *= pow(c_free[a], (double)A[a][k]);
  }

  free(c_free);

  return c;
}


double *
vrna_equilibrium_conc(const double        *eq_constants,
                      double              *concentration_strands,
                      const unsigned int  **A,
                      size_t              num_strands,
                      size_t              num_complexes)
{
  double        *r = NULL;

  column_vector starting_point;
  double        **delta;

  h_model       h;

  h.init(eq_constants,
         concentration_strands,
         A,
         (const double **)delta,
         num_strands,
         num_complexes);

  starting_point.set_size(num_strands);

  for (size_t a = 0; a < num_strands; a++)
    starting_point(a) = 0.;

  find_min_trust_region(objective_delta_stop_strategy(1e-32),
                        h,
                        starting_point,
                        1   // initial trust region radius
                        );

  double *conc_monomers = conc_single_strands(starting_point, num_strands);

  for (size_t a = 0; a < num_strands; a++)
    concentration_strands[a] = conc_monomers[a];

  r = conc_complexes(starting_point, eq_constants, A, num_strands, num_complexes);

  free(conc_monomers);

  return r;
}
