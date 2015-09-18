/*
 * Author: Ali Ghayoor
 * at Psychiatry Imaging Lab,
 * University of Iowa Health Care 2011
 */

#include "itkVariableSizeMatrix.h"
#include "itkMahalanobisDistanceMetric.h"
#include "itkEuclideanDistanceMetric.h"

#include "vnl/vnl_vector.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_matrix_inverse.h"

int main( int argc, char * argv[] )
{

  // create a sample test matrix
  // compute mean
  // compute covariance
  // compute MD and ED
  // create Dist
  // return isPure!

  vnl_matrix<double> measurementMatrix(2,8);
  measurementMatrix.fill(1);

  // 1-compute mean
  vnl_vector<double> mean( measurementMatrix.rows() );
  for( unsigned int c = 0; c != measurementMatrix.rows(); ++c )
  {
  mean[c] = measurementMatrix.get_row(c).mean();
  }






  return EXIT_SUCCESS;
}
