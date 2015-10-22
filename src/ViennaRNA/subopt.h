/* subopt.h */
#ifndef VIENNA_RNA_PACKAGE_SUBOPT_H
#define VIENNA_RNA_PACKAGE_SUBOPT_H

#include <ViennaRNA/data_structures.h>
#include <ViennaRNA/params.h>


/**
 *  \addtogroup subopt_fold Enumerating Suboptimal Structures
 *  \ingroup folding_routines
 *  @{
 *    \file subopt.h
 *    \brief RNAsubopt and density of states declarations
 *
 *  @}
 */

/**
 *  @brief Maximum density of states discretization for subopt
 */
#define MAXDOS                1000

/**
 *  \addtogroup subopt_wuchty
 *  @{
 *
 *  @}
 */

/**
 *  \brief Returns list of subopt structures or writes to fp
 * 
 *  This function produces <b>all</b> suboptimal secondary structures within
 *  'delta' * 0.01 kcal/mol of the optimum. The results are either
 *  directly written to a 'fp' (if 'fp' is not NULL), or
 *  (fp==NULL) returned in a #SOLUTION * list terminated
 *  by an entry were the 'structure' pointer is NULL.
 *
 *  \ingroup subopt_wuchty
 *
 *  \param  seq
 *  \param  structure
 *  \param  delta
 *  \param  fp
 *  \return
 */
SOLUTION *subopt (char *seq,
                  char *structure,
                  int delta,
                  FILE *fp);

/**
 *  \brief Returns list of subopt structures or writes to fp
 * 
 *  \ingroup subopt_wuchty
 */
SOLUTION *subopt_par( char *seq,
                      char *structure,
                      vrna_param_t *parameters,
                      int delta,
                      int is_constrained,
                      int is_circular,
                      FILE *fp);

/**
 *  \brief Returns list of circular subopt structures or writes to fp
 * 
 *  This function is similar to subopt() but calculates secondary structures
 *  assuming the RNA sequence to be circular instead of linear
 * 
 *  \ingroup subopt_wuchty
 *
 *  \param  seq
 *  \param  sequence
 *  \param  delta
 *  \param  fp
 *  \return
 */
SOLUTION *subopt_circ ( char *seq,
                        char *sequence,
                        int delta,
                        FILE *fp);

SOLUTION *vrna_subopt(vrna_fold_compound *vc,
                      int delta,
                      FILE *fp);

/**
 *  \brief Sort output by energy
 * 
 *  \ingroup subopt_wuchty
 *
 */
extern  int     subopt_sorted;


/**
 *  \brief printing threshold for use with logML
 * 
 *  \ingroup subopt_wuchty
 *
 */
extern  double  print_energy;

/**
 *  \addtogroup dos
 *  @{
 */

/**
 *  \brief The Density of States
 *
 *  This array contains the density of states for an RNA sequences after a call to subopt_par(),
 *  subopt() or subopt_circ().
 *
 *  \pre  Call one of the functions subopt_par(), subopt() or subopt_circ() prior accessing the contents
 *        of this array
 *  \see  subopt_par(), subopt(), subopt_circ()
 *
 */
extern  int     density_of_states[MAXDOS+1];

/** @} */ /* End of group dos */

#endif
